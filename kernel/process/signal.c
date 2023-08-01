#include "lock.h"
#include "error.h"
#include "driver.h"
#include "thread.h"
#include "process.h"
#include "signal.h"

int signalSetEmpty(SignalSet *set)
{
    set->signal = 0;
    return 0;
}

int signalSetFill(SignalSet *set)
{
    set->signal = 0xffffffffffffffff;
    return 0;
}

int signalSetAdd(SignalSet *set, int signal)
{
    if (signal < 1 || signal > SIGNAL_COUNT) { return -1; }
    set->signal |= 1ULL << (signal - 1);
    return 0;
}

int signalSetDel(SignalSet *set, int signal)
{
    if (signal < 1 || signal > SIGNAL_COUNT) { return -1; }
    set->signal &= ~(1ULL << (signal - 1));
    return 0;
}

int signalSetOr(SignalSet *set, SignalSet *set1)
{
    set->signal |= set1->signal;
    return 0;
}

bool signalSetIsMember(SignalSet *set, int signal)
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
        return 0;
    }
    printk("there's no signal context left!\n");
    *signalContext = NULL;
    return -1;
}

void signalContextFree(SignalContext *sc)
{
    LIST_REMOVE(sc, link);
    LIST_INSERT_HEAD(&freeSignalContextList, sc, link);
}

SignalContext *getFirstPendingSignal(Thread *thread)
{
    SignalContext *sc = NULL;
    LIST_FOREACH(sc, &thread->pendingSignal, link)
    {
        if (signalSetIsMember(&thread->blocked, sc->signal)) { continue; }
        return sc;
    }
    return NULL;
}

extern Process processes[];
SignalAction *getSignalHandler(Process *p)
{
    return (SignalAction *)(KERNEL_PROCESS_SIGNAL_BASE
                            + (u64)(p - processes) * PAGE_SIZE);
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

void handleSignal()
{
    Thread *thread = myThread();
    if (thread->killed) { threadDestroy(thread); } // SIGKILL to here
    SignalContext *sc;
    while (1)
    {
        sc = getFirstPendingSignal(thread);
        if (sc == NULL) { break; }
        SignalAction *sa = getSignalHandler(thread->process) + (sc->signal - 1);
        if (sa->sa_handler == NULL)
        {
            SignalActionDefault(thread, sc->signal);
            continue;
        }
        Trapframe *tf = getHartTrapFrame();
        bcopy(tf, &sc->contextRecover, sizeof(Trapframe));
        sc->blockedRecover = thread->blocked;        // 原有 block
        signalSetAdd(&thread->blocked, sc->signal);  // block 自己
        signalSetOr(&thread->blocked, &sa->sa_mask); // block sa 中规定的信号
        // printk("sa->flag %x\n", sa->sa_flags);       // TODO sa->flag;
        tf->sp = ALIGN_DOWN(tf->sp, PAGE_SIZE);
        tf->ra = SIGNAL_TRAMPOLINE;
        tf->epc = (u64)sa->sa_handler;
        // printk("sa->sa_handler 0x%lx\n", sa->sa_handler);
        if (sa->sa_flags & 0x0004)
        {
            sa->sa_handler = NULL;
            signalSetEmpty(&sa->sa_mask);
        }
        LIST_REMOVE(sc, link);                               // 从 pendingSignal 取出
        LIST_INSERT_HEAD(&thread->handlingSignal, sc, link); // 插入 handlingSignal
    }
}

void rt_sigreturn()
{
    Thread *thread = myThread();
    SignalContext *sc = LIST_FIRST(&thread->handlingSignal);
    Trapframe *tf = getHartTrapFrame();

    bcopy(&sc->contextRecover, tf, sizeof(Trapframe));
    thread->blocked = sc->blockedRecover;

    LIST_REMOVE(sc, link);
    LIST_INSERT_HEAD(&freeSignalContextList, sc, link);
}

int rt_sigaction(int sig, u64 act, u64 oldAction)
{
    Thread *th = myThread();
    if (sig < 1 || sig > SIGNAL_COUNT) { return -1; }
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

// TODO
int rt_sigtimedwait(SignalSet *which, SignalInfo *info, TimeSpec *ts)
{
    // printk("rt_sigtimedwait TODO!");
    Thread *thread = myThread();
    SignalContext *sc = getFirstPendingSignal(thread);
    return sc == NULL ? 0 : sc->signal;
}

extern struct ThreadList priSchedList[140];
int threadSignalSend(Thread *thread, int sig)
{
    SignalContext *sc;
    int r = signalContextAlloc(&sc);
    if (r < 0) { panic(""); }

    sc->signal = sig;
    if (sig == SIGKILL) // SIGKILL 直接修改 thread 状态
    {
        if (thread->state != RUNNABLE)
        {
            thread->state = RUNNABLE;
            int pri = 99 - thread->schedParam.schedPriority;
            LIST_INSERT_TAIL(&priSchedList[pri], thread, priSchedLink);
        }

        thread->killed = true;
    }
    // 最近的信号在最后面，从而保证先插入旧的到 handling
    LIST_INSERT_TAIL(&thread->pendingSignal, sc, link);
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
    return ret == -ESRCH ? 0 : ret;
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
    ret = tid2Thread(tid, &thread, 0);
    if (ret < 0) { return ret; }
    ret = threadSignalSend(thread, sig);
    return ret;
}