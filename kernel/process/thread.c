#include <process.h>
#include <thread.h>
#include <riscv.h>
#include <memory.h>
#include <queue.h>
#include <string.h>
#include <driver.h>
#include <mem_layout.h>
#include <error.h>
#include <yield.h>
#include <trap.h>
#include <elf.h>
#include <debug.h>
#include <lock.h>
#include <error.h>
#include <fs.h>
#include <signal.h>
#include <futex.h>

Thread threads[PROCESS_TOTAL_NUMBER];

struct ThreadList freeThreads, usedThreads;
struct ThreadList scheduleList[2];
struct ThreadList priSchedList[140];
Thread *currentThread[CORE_NUM] = {0};
struct Spinlock threadListLock, scheduleListLock, threadIdLock;

Thread *myThread()
{
    interruptPush();
    if (currentThread[getTp()] == NULL)
    {
        return NULL;
    }
    Thread *t = currentThread[getTp()];
    interruptPop();
    return t;
}

void threadFree(Thread *th)
{
    Process *p = th->process;
    while (!LIST_EMPTY(&th->pendingSignal))
    {
        signalContextFree(LIST_FIRST(&th->pendingSignal));
    }
    while (!LIST_EMPTY(&th->handlingSignal))
    {
        signalContextFree(LIST_FIRST(&th->handlingSignal));
    }
    // releaseLock(&th->lock);
    // acquireLock(&p->lock);

    if (th->clearChildTid)
    {
        int val = 0;
        copyout(p->pgdir, th->clearChildTid, (char *)&val, sizeof(int));
        futexWake(th->clearChildTid, 1);
    }

    p->threadCount--;
    if (!p->threadCount)
    {
        p->retValue = th->retValue;
        processFree(p);
    }
    if (th->state == RUNNABLE) { LIST_REMOVE(th, priSchedLink); }
    th->state = UNUSED;
    LIST_REMOVE(th, link);
    LIST_INSERT_HEAD(&freeThreads, th, link); // test pipe
}

/**
 * @brief 获取线程的内核栈地址
 *
 * @param p
 * @return u64
 */
u64 getThreadTopSp(Thread *th)
{
    return KERNEL_PROCESS_SP_TOP - (u64)(th - threads) * 10 * PAGE_SIZE;
}

extern u64 kernelPageDirectory[];

void threadInit()
{
    initLock(&threadListLock, "threadList");
    initLock(&scheduleListLock, "scheduleList");
    initLock(&threadIdLock, "threadId");

    LIST_INIT(&freeThreads);
    LIST_INIT(&usedThreads);
    LIST_INIT(&scheduleList[0]);
    LIST_INIT(&scheduleList[1]);
    for (int pri = 0; pri <= PRI_MAX - PRI_MIN; ++pri)
    {
        LIST_INIT(&priSchedList[pri]);
    }

    int i;
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--)
    {
        threads[i].state = UNUSED;
        threads[i].trapframe.kernelSatp = MAKE_SATP(SV39, PA2PPN(kernelPageDirectory));
        LIST_INSERT_HEAD(&freeThreads, &threads[i], link);
    }
}

u32 generateThreadId(Thread *th)
{
    static u32 nextId = 0;
    // 高位是按顺序分配的编码，低位是进程控制块的序号
    u32 threadId = ((++nextId) << (1 + LOG_PROCESS_NUM)) | (u32)(th - threads);
    printk("generate thread id 0x%lx\n", threadId);
    return threadId;
}

/**
 * @brief 申请主线程（包含申请它对应的进程）
 *
 * @param new
 * @param parentId
 * @return int
 */
int mainThreadAlloc(Thread **new, u64 parentId)
{
    int r;
    Thread *th;
    // acquireLock(&threadListLock);
    if (LIST_EMPTY(&freeThreads))
    {
        // releaseLock(&threadListLock);
        // panic("there's no freeThreads");
        *new = NULL;
        return -ESRCH;
    }
    th = LIST_FIRST(&freeThreads);
    LIST_REMOVE(th, link);
    LIST_INSERT_HEAD(&usedThreads, th, link);
    // releaseLock(&threadListLock);

    threadSetup(th);
    th->threadId = generateThreadId(th);
    th->state = RUNNABLE;
    th->trapframe.kernelSp = getThreadTopSp(th);
    th->trapframe.sp = USER_STACK_TOP - 24; // argc = 0, argv = 0, envp = 0
    Process *process;
    r = processAlloc(&process, parentId);
    if (r < 0)
    {
        *new = NULL;
        return r;
    }
    // acquireLock(&process->lock);
    th->process = process;
    process->threadCount++;
    // releaseLock(&process->lock);
    *new = th;
    return 0;
}

int threadAlloc(Thread **new, Process *process, u64 userSp)
{
    Thread *th;
    if (LIST_EMPTY(&freeThreads))
    {
        // panic("there's no freeThreads");
        *new = NULL;
        return -ESRCH;
    }
    th = LIST_FIRST(&freeThreads);
    LIST_REMOVE(th, link);
    LIST_INSERT_HEAD(&usedThreads, th, link);

    threadSetup(th);
    th->threadId = generateThreadId(th);
    th->state = RUNNABLE;
    th->trapframe.kernelSp = getThreadTopSp(th);
    th->trapframe.sp = userSp;

    th->process = process;
    process->threadCount++;

    *new = th;
    return 0;
}

int tid2Thread(u32 threadId, struct Thread **thread, int checkPerm)
{
    struct Thread *th;
    int hartId = getTp();

    if (threadId == 0)
    {
        *thread = currentThread[hartId];
        return 0;
    }

    th = threads + PROCESS_OFFSET(threadId);

    if (th->state == UNUSED || th->threadId != threadId)
    {
        *thread = NULL;
        return -ESRCH;
    }

    if (checkPerm)
    {
        if (th != currentThread[hartId] && th->process->parentId != myProcess()->processId)
        {
            *thread = NULL;
            return -EPERM;
        }
    }

    *thread = th;
    return 0;
}

extern FileSystem *rootFileSystem;
extern FileSystem fileSystem[32];
void sleepRec();

/**
 * @brief 运行线程 th
 *
 * @param th
 */
void threadRun(Thread *th)
{
    static int first = 1;
    Trapframe *trapframe = getHartTrapFrame();
    // printk("now tid: 0x%lx\n", th->threadId);
    // 保存当前进程的 trapfreme 到进程结构体中
    if (currentThread[getTp()])
    {
        memmove(&(currentThread[getTp()]->trapframe), trapframe,
                sizeof(Trapframe));
    }

    th->state = RUNNING;
    int hartid = getTp();
    currentThread[hartid] = th;

    if (th->reason)
    {
        th->reason = 0;
        memmove(trapframe, &currentThread[getTp()]->trapframe, sizeof(Trapframe));
        asm volatile("ld sp, 0(%0)"
                     :
                     : "r"(&th->currentKernelSp));
        sleepRec();
    }
    else
    {
        // 在此处初始化文件系统，是因为初始化需要 sleep，而 sleep 需要考虑进程号

        if (first)
        {
            first = 0;
            // 把 hartTrapframe 中的东西拷贝到 hartTrapframe 中
            // 是因为 process run最开始如果是 (currentProcess[getTp()]) ，就要把 harttrapframe 拷贝到 process->tf
            // 但是第一个进程在 run 的时候 harttf 里面根本没有东西，就会导致用  0 覆盖
            memmove(trapframe, &(currentThread[hartid]->trapframe), sizeof(Trapframe));
            initRootFileSystem();
            th->process->cwd = &(rootFileSystem->root);
        }

        // 切换页表
        // 拷贝进程的 trapframe 到 hart 对应的 trapframe

        memmove(trapframe, &(currentThread[getTp()]->trapframe), sizeof(Trapframe));
        u64 sp = getHartKernelTopSp(th);
        asm volatile("ld sp, 0(%0)"
                     :
                     : "r"(&sp)
                     : "memory");
        userTrapReturn();
    }
}

void threadDestroy(Thread *th)
{
    threadFree(th);
    int hartId = getTp();
    if (currentThread[hartId] == th)
    {
        currentThread[hartId] = NULL;
        extern char kernelStack[];
        u64 sp = (u64)kernelStack + (hartId + 1) * KERNEL_STACK_SIZE;
        asm volatile("ld sp, 0(%0)"
                     :
                     : "r"(&sp)
                     : "memory");
        callYield();
    }
}

void threadSetup(Thread *th)
{
    th->channel = 0;
    th->retValue = 0;
    th->state = UNUSED;
    th->reason = 0;
    th->awakeTime = 0;
    CPU_ZERO(&th->cpuset);
    CPU_SET(0, &th->cpuset);
    th->schedPolicy = SCHED_OTHER;
    th->schedParam.schedPriority = 0;
    th->killed = false;
    signalSetEmpty(&th->blocked);
    LIST_INIT(&th->pendingSignal);
    LIST_INIT(&th->handlingSignal);

    th->clearChildTid = 0;
    Page *page;

    /* 申线程内核栈 */
    /* 仅申请了3个 Page */
    if (pageAlloc(&page) < 0)
    {
        panic("");
    }
    pageInsert(kernelPageDirectory, getThreadTopSp(th) - PAGE_SIZE, (page), PTE_READ_BIT | PTE_WRITE_BIT);
    if (pageAlloc(&page) < 0)
    {
        panic("");
    }
    pageInsert(kernelPageDirectory, getThreadTopSp(th) - PAGE_SIZE * 2, (page), PTE_READ_BIT | PTE_WRITE_BIT);
    if (pageAlloc(&page) < 0)
    {
        panic("");
    }
    pageInsert(kernelPageDirectory, getThreadTopSp(th) - PAGE_SIZE * 3, (page), PTE_READ_BIT | PTE_WRITE_BIT);
}

void sleepSave();

// 因为让一个进程进入 sleep 状态是一个原子过程，因此要加锁
/**
 * @brief 当前进程进入 sleep 状态，
 *
 * @param channel
 * @param lk
 */
void sleep(void *channel, Spinlock *lk)
{
    Thread *th = myThread();

    acquireLock(&(th->lock));
    releaseLock(lk);
    th->channel = (u64)channel;
    th->state = SLEEPING; // 必然是 running -> sleeping
    th->reason = 1;
    releaseLock(&(th->lock));

    if (th->killed) { threadDestroy(th); }

    asm volatile("sd sp, 0(%0)"
                 :
                 : "r"(&th->currentKernelSp));

    // 保存寄存器
    sleepSave();

    acquireLock(&th->lock);
    th->channel = 0;
    releaseLock(&th->lock);

    if (th->killed) { threadDestroy(th); }

    acquireLock(lk);
}

/**
 * @brief 唤醒在等待队列 channel 中的所有线程
 * 其实并没有队列，channel 只是一个整数，记录在 Process 结构体中
 *
 * @param channel
 */
void wakeup(void *channel)
{
    // 遍历所有使用过的进程，找到 process->channel = channel 的
    Thread *th = NULL;
    LIST_FOREACH(th, &usedThreads, link)
    {
        acquireLock(&th->lock);
        if (th->state == SLEEPING && th->channel == (u64)channel)
        {
            th->state = RUNNABLE;
            int pri = 99 - th->schedParam.schedPriority;
            LIST_INSERT_TAIL(&priSchedList[pri], th, priSchedLink);
        }
        releaseLock(&th->lock);
    }
}
