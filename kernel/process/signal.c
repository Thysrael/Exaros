#include "lock.h"
#include "error.h"
#include "driver.h"
#include "thread.h"
#include "process.h"
#include "signal.h"

int signalEmptySet(SignalSet *set)
{
    set->signal = 0;
    return 0;
}

int signalFillset(SignalSet *set)
{
    set->signal = 0xffffffffffffffff;
    return 0;
}

int signalAddSet(SignalSet *set, int signal)
{
    if (signal < 1 || signal > SIGNAL_COUNT) { return -1; }
    set->signal |= 1ULL << (signal - 1);
    return 0;
}

int signalDelSet(SignalSet *set, int signal)
{
    if (signal < 1 || signal > SIGNAL_COUNT) { return -1; }
    set->signal &= ~(1ULL << (signal - 1));
    return 0;
}

bool signalIsMember(SignalSet *set, int signal)
{
    return (set->signal & (1ULL << (signal - 1))) != 0;
}

SignalContext signalContext[SIGNAL_CONTEXT_COUNT];
SignalContextList freeSignalContextList;
struct Spinlock signalContextListLock;

/**
 * @brief 初始化 Free Signal Context
 *
 */
void signalInit()
{
    initLock(&signalContextListLock, "signalContextLock");
    for (int i = 0; i < SIGNAL_CONTEXT_COUNT; ++i)
    {
        LIST_INSERT_HEAD(&freeSignalContextList, &signalContext[i], link);
    }
}

/**
 * @brief 释放 Signal Context
 *
 * @param sc
 */
void signalContextFree(SignalContext *sc)
{
    LIST_INSERT_HEAD(&freeSignalContextList, sc, link);
}

/**
 * @brief 申请 Signal Context
 *
 * @param signalContext
 * @return int
 */
int signalContextAlloc(SignalContext **signalContext)
{
    SignalContext *sc;
    if ((sc = LIST_FIRST(&freeSignalContextList)) != NULL)
    {
        *signalContext = sc;
        LIST_REMOVE(sc, link);
        sc->start = false;
        return 0;
    }
    printk("there's no signal context left!\n");
    *signalContext = NULL;
    return -1;
}

// SignalContext *getFirstSignalContext(Thread *thread)
// {
//     SignalContext *sc = NULL;
//     int signal;
//     // 按照 ID 一个个进行处理
//     for (signal = 0; signal < SIGNAL_COUNT; ++signal)
//     {
//         if (signalIsMember(&thread->pending, signal) && !signalIsMember(&thread->blocked, signal)) { break; }
//     }
//     if (signal == SIGNAL_COUNT) { return sc; }
//     signal++;
//     LIST_FOREACH(sc, &thread->waitingSignal, link)
//     {
//         if (sc->signal != signal) { continue; }
//         break;
//     }
//     return sc;
// }

SignalContext *getFirstSignalContext(Thread *thread)
{
    SignalContext *sc = NULL;
    LIST_FOREACH(sc, &thread->waitingSignal, link)
    {
        if (!sc->start && (signalIsMember(&thread->blocked, sc->signal) || signalIsMember(&thread->processing, sc->signal)))
        {
            continue;
        }
        break;
    }
    return sc;
}

SignalContext *getHandlingSignal(Thread *thread)
{
    SignalContext *sc = NULL;
    // acquireLock(&thread->lock);
    LIST_FOREACH(sc, &thread->waitingSignal, link)
    {
        if (sc->start)
        {
            break;
        }
    }
    // releaseLock(&thread->lock);
    assert(sc != NULL);
    return sc;
}

bool hasKillSignal(Thread *thread)
{
    SignalContext *sc = NULL;
    // acquireLock(&thread->lock);
    bool find = false;
    LIST_FOREACH(sc, &thread->waitingSignal, link)
    {
        if (sc->signal == SIGKILL)
        {
            find = true;
            break;
        }
    }
    // releaseLock(&thread->lock);
    return find;
}

void initFrame(SignalContext *sc, Thread *thread)
{
    Trapframe *tf = getHartTrapFrame();
    u64 sp = ALIGN_DOWN(tf->sp - PAGE_SIZE, PAGE_SIZE);
    Page *page;
    int r;
    if ((r = pageAlloc(&page)) < 0) { panic(""); }
    pageInsert(myProcess()->pgdir, sp - PAGE_SIZE, page, PTE_USER_BIT | PTE_READ_BIT | PTE_WRITE_BIT);
    u32 pageTop = PAGE_SIZE;
    tf->sp = pageTop + sp - PAGE_SIZE;
    tf->ra = SINGNAL_TRAMPOLINE;
}

void signalFinish(Thread *thread, SignalContext *sc)
{
    LIST_REMOVE(sc, link);
    signalContextFree(sc);
}

extern Process processes[];
SignalAction *getSignalHandler(Process *p)
{
    return (SignalAction *)(KERNEL_PROCESS_SIGNAL_BASE + (u64)(p - processes) * PAGE_SIZE);
}

void SignalActionDefault(Thread *thread, int signal)
{
    printk("GET SIGNAL: %d\n", signal);
    // switch (signal)
    // {
    // case SIGTSTP:
    // case SIGTTIN:
    // case SIGTTOU:
    // case SIGSTOP:
    // case SIGQUIT:
    // case SIGILL:
    // case SIGTRAP:
    // case SIGABRT:
    // case SIGFPE:
    // case SIGSEGV:
    // case SIGBUS:
    // case SIGSYS:
    // case SIGXCPU:
    // case SIGXFSZ:
    // }
}

void handleSignal(Thread *thread)
{
    SignalContext *sc;
    while (1)
    {
        if (thread->killed)
        {
            threadDestroy(thread);
        }
        sc = getFirstSignalContext(thread);
        if (sc == NULL)
        {
            return;
        }
        if (sc->start)
        {
            return;
        }
        SignalAction *sa = getSignalHandler(thread->process) + (sc->signal - 1);
        if (sa->handler == NULL)
        {
            SignalActionDefault(thread, sc->signal);
            signalFinish(thread, sc);
            continue;
        }
        sc->start = true;
        signalAddSet(&thread->processing, sc->signal);
        Trapframe *tf = getHartTrapFrame();
        bcopy(tf, &sc->contextRecover, sizeof(Trapframe));
        struct pthread self;
        copyin(thread->process->pgdir, (char *)&self, thread->trapframe.tp - sizeof(struct pthread), sizeof(struct pthread));
        initFrame(sc, thread);
        tf->epc = (u64)sa->handler;
        return;
    }
}

int rt_sigaction(int sig, u64 act, u64 oldAction)
{
    Thread *th = myThread();
    if (sig < 1 || sig > SIGNAL_COUNT)
    {
        return -1;
    }
    SignalAction *k = getSignalHandler(th->process) + (sig - 1);
    if (oldAction)
    {
        copyout(myProcess()->pgdir, oldAction, (char *)k, sizeof(SignalAction));
    }
    if (act)
    {
        copyin(myProcess()->pgdir, (char *)k, act, sizeof(SignalAction));
    }
    return 0;
}

int rt_sigprocmask(int how, SignalSet *set, SignalSet *oldset, int sigsetsize)
{
    Thread *th = myThread();
    // sigsetsize 为 sizeof(SignalSet) = 8
    oldset->signal = th->blocked.signal;
    switch (how)
    {
    case SIG_BLOCK:
        th->blocked.signal |= set->signal;
        return 0;
    case SIG_UNBLOCK:
        th->blocked.signal &= ~(set->signal);
        return 0;
    case SIG_SETMASK:
        th->blocked.signal = set->signal;
        return 0;
    default:
        return -1;
    }
}

// void handleSignal(Thread *thread)
// {
//     SignalContext *sc;
//     while (1)
//     {
//         if (thread->killed) { threadDestroy(thread); }
//         sc = getFirstSignalContext(thread);
//         if (sc == NULL) { return; }
//         SignalAction *sa = getSignalHandler(thread->process) + (sc->signal - 1);
//         // 如果没有注册对应函数，调用默认函数
//         if (sa->handler == NULL)
//         {
//             SignalActionDefault(thread, sc->signal);
//             continue;
//         }

//         sc->start = true;
//         // signalAddSet(sc->signal, &thread->processing);// 忽略相同信号
//         Trapframe *tf = getHartTrapFrame();
//         // 拷贝 Trapframe
//         bcopy(tf, &sc->contextRecover, sizeof(Trapframe));
//         // struct pthread self;
//         // copyin(thread->process->pgdir, (char *)&self, thread->trapframe.tp - sizeof(struct pthread), sizeof(struct pthread));
//         initFrame(sc, thread);
//         tf->epc = (u64)sa->handler;
//         return;
//     }
// }

int threadSignalSend(Thread *thread, int sig)
{
    SignalContext *sc;
    int r = signalContextAlloc(&sc);
    if (r < 0) { panic(""); }

    sc->signal = sig;
    // TODO
    if (sig == SIGKILL) // SIGKILL 直接修改 thread 状态
    {
        thread->state = RUNNABLE;
        thread->killed = true;
    }
    LIST_INSERT_HEAD(&thread->waitingSignal, sc, link);
    return 0;
}

/**
 * @brief 向进程发送信号
 * 随机挑选一个线程组中一个进程
 *
 * @param pid
 * @param sig
 * @return int
 */
int kill(int pid, int sig)
{
    extern struct ThreadList usedThreads;
    int ret = -ESRCH;
    Thread *thread = NULL;
    LIST_FOREACH(thread, &usedThreads, link)
    {
        if (pid == thread->process->processId)
        {
            ret = threadSignalSend(thread, sig);
            break;
        }
    }
    return ret;
}

/**
 * @brief 选择进程中特定线程发送信号
 *
 * @param tgid
 * @param tid
 * @param sig
 * @return int
 */
int tgkill(int tgid, int tid, int sig)
{
    extern struct ThreadList usedThreads;
    int ret = -ESRCH;
    Thread *thread = NULL;
    LIST_FOREACH(thread, &usedThreads, link)
    {
        if (tgid == thread->process->processId && tid == thread->threadId)
        {
            ret = threadSignalSend(thread, sig);
            break;
        }
    }
    return ret;
}

/**
 * @brief 给特定线程发送信号
 * 注意可能会由于 threadId 的回收导致删除错误
 *
 * @param tid
 * @param sig
 * @return int
 */
int tkill(int tid, int sig)
{
    extern struct ThreadList usedThreads;
    int ret = -ESRCH;
    Thread *thread = NULL;
    LIST_FOREACH(thread, &usedThreads, link)
    {
        if (tid == thread->threadId)
        {
            ret = threadSignalSend(thread, sig);
            break;
        }
    }
    return ret;
}

int rt_sigtimedwait(SignalSet *which, SignalInfo *info, TimeSpec *ts)
{
    Thread *thread = myThread();
    SignalContext *sc = getFirstSignalContext(thread);
    return sc == NULL ? 0 : sc->signal;
}

void sigreturn()
{
    Trapframe *tf = getHartTrapFrame();
    Thread *thread = myThread();
    SignalContext *sc = getHandlingSignal(thread);
    bcopy(&sc->contextRecover, tf, sizeof(Trapframe));
    signalDelSet(&thread->processing, sc->signal);
    signalFinish(thread, sc);
}