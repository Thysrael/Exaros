/**
 * @file process.c
 * @brief 进程管理
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 */

#include <process.h>
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
#include <thread.h>
#include <fs.h>
#include <futex.h>

Process processes[PROCESS_TOTAL_NUMBER];
ProcessList freeProcesses, usedProcesses;
// ProcessList scheduleList[2];
// Process *currentProcess[CORE_NUM] = {0};
static int processTimeCount[CORE_NUM] = {0};
static int processBelongList[CORE_NUM] = {0};
struct Spinlock freeProcessesLock, processIdLock, waitLock, currentProcessLock;

/**
 * @brief 获取当前这个核正在运行的进程
 *
 * @return Process*
 */
Process *myProcess()
{
    if (myThread() == NULL)
    {
        return NULL;
    }
    return myThread()->process;
}

/**
 * @brief 初始化进程管理
 *
 */
extern u64 kernelPageDirectory[];
void processInit()
{
    printk("ProcessInit start...\n");

    LIST_INIT(&freeProcesses);
    LIST_INIT(&usedProcesses);

    int i;
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--)
    {
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);
    }
    threadInit();
    writeSscratch((u64)getHartTrapFrame());
    printk("ProcessInit end!!!\n");
}

u32 generateProcessId(Process *p)
{
    static u32 nextId = 0;
    // 高位是按顺序分配的编码，低位是进程控制块的序号
    u32 processId = ((++nextId) << (1 + LOG_PROCESS_NUM)) | (u32)(p - processes);
    return processId;
}

extern Thread *currentThread[CORE_NUM];

/**
 * @brief 终止进程 p，修改 sp 指针为内核栈地址，然后重新调度进程
 *
 * @param p
 */
void processDestory(Process *p)
{
    processFree(p);
    int hartId = getTp();

    extern Thread threads[PROCESS_TOTAL_NUMBER];

    // destory 掉这个进程的线程
    for (int i = 0; i < PROCESS_TOTAL_NUMBER; i++)
    {
        if (threads[i].process == p)
        {
            threadDestroy(&threads[i]);
        }
    }

    if (currentThread[hartId]->process == p)
    {
        currentThread[hartId] = NULL;
        extern char kernelStack[];
        u64 sp = (u64)kernelStack + (hartId + 1) * KERNEL_STACK_SIZE;
        asm volatile("ld sp, 0(%0)"
                     :
                     : "r"(&sp)
                     : "memory");
        yield();
    }
}

/**
 * @brief 释放进程控制块
 *
 * @param p
 */
void processFree(Process *p)
{
    pgdirFree(p->pgdir);
    p->state = ZOMBIE; // 进程已结束，但是未被父进程获取返回值

    // todo 在这里释放掉进程占用的文件资源
    for (int fd = 0; fd < NOFILE; fd++)
    {
        if (p->ofile[fd])
        {
            struct File *f = p->ofile[fd];
            fileclose(f);
            p->ofile[fd] = 0;
        }
    }
    processSegmentMapFree(p);
    if (p->parentId > 0)
    {
        Process *parentProcess;
        int r = pid2Process(p->parentId, &parentProcess, 0);
        // if (r < 0) {
        //     panic("Can't get parent process, current process is %x, parent is %x\n", p->id, p->parentId);
        // }
        // printf("[Free] process %x wake up %x\n", p->id, parentProcess);
        // The parent process may die before the child process

        if (r == 0)
        {
            wakeup(parentProcess);
        }
    }
}

/**
 * @brief 获取 processId 对应的进程控制块并放入 process 中
 * 如果 checkPerm 需要检查当前进程是不是要获取的进程或者要获取进程的父进程
 *
 * @param processId
 * @param process
 * @param checkPerm
 * @return int
 */
int pid2Process(u32 processId, Process **process, int checkPerm)
{
    struct Process *p;

    if (processId == 0)
    {
        *process = myProcess();
        return 0;
    }

    p = processes + PROCESS_OFFSET(processId);
    // p 进程 processId 的 Process 结构体

    if (p->state == UNUSED || p->processId != processId)
    {
        *process = NULL;
        return -INVALID_PROCESS_STATUS;
    }

    if (checkPerm)
    {
        if (p != myProcess() && p->parentId != myProcess()->processId)
        {
            *process = NULL;
            return -INVALID_PERM;
        }
    }

    *process = p;
    return 0;
}

extern void userTrap();

/**
 * @brief 为进程申请一页内存作为页表，成功返回 0
 *
 * @param page
 * @return int
 */
int allocPgdir(Page **page)
{
    int r;
    if ((r = pageAlloc(page)) < 0)
        return r;
    (*page)->ref++;
    return 0;
}

void pgdirFree(u64 *pgdir)
{
    // printf("jaoeifherigh   %lx\n", (u64)pgdir);
    u64 i, j, k;
    u64 *pageTable;
    for (i = 0; i < PTE2PT; i++)
    {
        if (!(pgdir[i] & PTE_VALID_BIT))
            continue;
        pageTable = pgdir + i;
        u64 *pa = (u64 *)PTE2PA(*pageTable);
        for (j = 0; j < PTE2PT; j++)
        {
            if (!(pa[j] & PTE_VALID_BIT))
                continue;
            pageTable = (u64 *)pa + j;
            u64 *pa2 = (u64 *)PTE2PA(*pageTable);
            for (k = 0; k < PTE2PT; k++)
            {
                if (!(pa2[k] & PTE_VALID_BIT))
                    continue;
                u64 addr = (i << 30) | (j << 21) | (k << 12);
                pageRemove(pgdir, addr);
            }
            pa2[j] = 0;
            paDecreaseRef((u64)pa2);
        }
        paDecreaseRef((u64)pa);
    }
    paDecreaseRef((u64)pgdir);
}

extern FileSystem *rootFileSystem;
extern FileSystem fileSystem[32];
/**
 * @brief 为进程申请页表，并且建立 trampoline 和 trapframe 的映射
 * 为进程的内核栈建立映射，这样进程在陷入内核的时候才不会报错
 *
 * @param p
 * @return int
 */
int setupProcess(Process *p)
{
    Page *page;

    // 申请页表
    int r = allocPgdir(&page);
    if (r)
    {
        panic("allock pgdir failed...\n");
        return r;
    }

    p->pgdir = (u64 *)page2PA(page);
    p->state = UNUSED;
    p->retValue = 0;
    p->parentId = 0;
    p->mmapHeapTop = USER_MMAP_HEAP_BOTTOM;
    p->brkHeapTop = USER_BRK_HEAP_BOTTOM;
    p->shmHeapTop = USER_SHM_HEAP_BOTTOM;
    p->fileDescription.hard = p->fileDescription.soft = NOFILE;

    p->cwd = &(rootFileSystem->root);

    // printk("p-> cwd: %lx, %lx\n", p->cwd, fileSystem);
    // 设置内核栈，就是进程在进入内核 trap 的时候使用的栈
    // 为每个进程开一页的内核栈
    // r = pageAlloc(&page);
    // printk("mmapva: %lx\n", getProcessTopSp(p) - PAGE_SIZE);

    // 其实这里最多可以申请 9 页（因为留了 10 页）
    // pageMap(kernelPageDirectory, getProcessTopSp(p) - PAGE_SIZE, page2PA(page),
    //         PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);

    if (pageAlloc(&page) < 0)
    {
        return -1;
    }
    pageInsert(kernelPageDirectory, KERNEL_PROCESS_SIGNAL_BASE + (u64)(p - processes) * PAGE_SIZE, page,
               PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);

    extern char trampoline[];
    extern char trapframe[];
    extern char signalTrampoline[];
    // 尝试更严格的权限管理
    // pageMap(p->pgdir, TRAMPOLINE, ((u64)trampoline), PTE_EXECUTE_BIT);
    // pageMap(p->pgdir, TRAPFRAME, ((u64)trapframe), PTE_READ_BIT | PTE_WRITE_BIT);
    pageMap(p->pgdir, TRAMPOLINE, ((u64)trampoline), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    pageMap(p->pgdir, TRAPFRAME, ((u64)trapframe), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    pageMap(p->pgdir, SIGNAL_TRAMPOLINE, ((u64)signalTrampoline), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT | PTE_USER_BIT);
    return 0;
}

/**
 * @brief 申请进程控制块并初始化
 * 包括申请页表，初始化页表映射（setupProcess)，初始化 trapframe
 *
 * @param new
 * @param parentId
 * @return int
 */
int processAlloc(Process **new, u64 parentId)
{
    int r;
    Process *p;

    if (LIST_EMPTY(&freeProcesses))
    {
        *new = NULL;
        return -NO_FREE_PROCESS;
    }
    p = LIST_FIRST(&freeProcesses);
    LIST_REMOVE(p, link);
    LIST_INSERT_HEAD(&usedProcesses, p, link);

    if ((r = setupProcess(p)) < 0)
    {
        return r;
    }

    p->processId = generateProcessId(p);
    p->pgid = p->processId;
    p->sid = p->processId;
    p->state = RUNNABLE;
    p->parentId = parentId;

    *new = p;
    return 0;
}

extern struct ThreadList scheduleList[2];

/**
 * @brief 给定一段二进制可执行文件，创建一个进程
 *
 * @param binary 二进制地址
 * @param size 二进制大小
 * @param priority 进程优先级
 */
void processCreatePriority(u8 *binary, u32 size, u32 priority)
{
    Thread *th;
    int r = mainThreadAlloc(&th, 0);
    if (r)
        return;
    Process *p = th->process;
    p->priority = priority;
    u64 entryPoint; // 入口地址
    if (loadElf(binary, size, &entryPoint, p) != 0)
    {
        panic("failed to load elf\n");
    }
    th->trapframe.epc = entryPoint;

    // 向栈压入参数
    Page *page;
    if (pageAlloc(&page))
    {
        panic("allock stack error\n");
    }
    u64 sp = th->trapframe.sp;
    pageInsert(p->pgdir, sp - PAGE_SIZE, page,
               PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT);

    char str[] = "create by kernel";
    sp -= strlen(str) + 1;
    sp -= sp % 16;
    if (copyout(p->pgdir, sp, str, strlen(str) + 1) < 0) // sizeof(char) = 1
    {
        panic("copyout error");
    }

    printk("str = 0x%lx\n", sp);
    u64 ustack[3] = {1, sp, 0};
    sp -= 3 * sizeof(u64);
    sp -= sp % 16;
    if (copyout(p->pgdir, sp, (char *)ustack, 2 * sizeof(u64)))
    {
        panic("copyout error");
    }
    th->trapframe.sp = sp;

    LIST_INSERT_TAIL(&scheduleList[0], th, scheduleLink);
    printk("created a process: %x, epc: %lx\n", p->processId, th->trapframe.epc);
}

/**
 * @brief 当前核的内核栈栈顶地址
 *
 * @return u64
 */
u64 getHartKernelTopSp()
{
    extern char kernelStack[];
    return (u64)kernelStack + KERNEL_STACK_SIZE * (getTp() + 1);
}

/**
 * @brief
 * 1. 将当前进程改为 RUNNABLE，并插入调度列表末尾
 * 2. 交替访问两个调度队列，直到找到一个 RURNNABLE 的进程
 */
void yield()
{
    // printk("yield\n");
    int hartId = getTp();
    int count = processTimeCount[hartId];
    int point = processBelongList[hartId];
    Thread *th = myThread();

    // printk("0: %d, 1:%d\n", processes[0].state, processes[1].state);

    if (th && th->state == RUNNING)
    {
        // 这一句没必要，因为在 processrun 里面是要拷贝的
        // memmove(&process->trapframe, getHartTrapFrame(), sizeof(Trapframe));
        th->state = RUNNABLE;
    }

    // 关闭时钟中断
    writeSie(readSie() & (~SIE_STIE));
    // 防止死锁，假如只有一个进程而这个进程被 sleep 了，在这里应该接受外部中断
    intr_on();

    while ((count == 0) || !th || (th->state != RUNNABLE) || (th->awakeTime > r_time()))
    {
        if (th)
            LIST_INSERT_TAIL(&scheduleList[point ^ 1], th, scheduleLink);
        if (LIST_EMPTY(&scheduleList[point]))
            point ^= 1;
        if (!(LIST_EMPTY(&scheduleList[point])))
        {
            th = LIST_FIRST(&scheduleList[point]);
            // printk(" %d ", process->processId);
            LIST_REMOVE(th, scheduleLink);
            count = th->process->priority;
        }
        CNX_DEBUG("finding a process to yield... %d, %d, %d\n", point, th->state, (int)intr_get());
    }

    // 在这里关掉中断，不然 sleep 到一半的时候被打断
    intr_off();

    writeSie(readSie() | SIE_STIE);

    // CNX_DEBUG("currentKernelSp: %lx \n", process->currentKernelSp);
    count--;
    processTimeCount[hartId] = count;
    processBelongList[hartId] = point;
    printk("hartID %d yield thread %lx, %lx\n", hartId, th->threadId, th->trapframe.epc);

    // syscall_watetime 的范围值设置为 0
    if (th->awakeTime > 0)
    {
        getHartTrapFrame()->a0 = 0;
        th->awakeTime = 0;
    }

    futexClear(th);
    threadRun(th);
}

// /**
//  * @brief 获取的内核栈地址
//  *
//  * @param p
//  * @return u64
//  */
// u64 getProcessTopSp(Process *p)
// {
//     return KERNEL_PROCESS_SP_TOP - (u64)(p - processes) * 10 * PAGE_SIZE;
// }

/**
 * @brief 等待进程 targetProcessId 改变状态，
 *
 */
int wait(int targetProcessId, u64 addr)
{
    Process *p = myProcess();
    int haveChildProcess, pid;

    acquireLock(&waitLock);

    while (true)
    {
        haveChildProcess = 0;
        Process *np = NULL;
        LIST_FOREACH(np, &usedProcesses, link)
        {
            acquireLock(&np->lock);
            if (np->parentId == p->processId)
            {
                haveChildProcess = 1;
                if ((targetProcessId == -1 || np->processId == targetProcessId) && np->state == ZOMBIE)
                {
                    pid = np->processId;
                    if (addr != 0 && copyout(p->pgdir, addr, (char *)&np->retValue, sizeof(np->retValue)) < 0)
                    {
                        releaseLock(&np->lock);
                        releaseLock(&waitLock);
                        return -1;
                    }
                    acquireLock(&freeProcessesLock);
                    // updateAncestorsCpuTime(np);
                    np->state = UNUSED;
                    LIST_REMOVE(np, link);                      // 从 used 列表中删除 p
                    LIST_INSERT_HEAD(&freeProcesses, np, link); // test pipe
                    // printf("[Process Free] Free an process %d\n", (u32)(np - processes));
                    releaseLock(&freeProcessesLock);
                    releaseLock(&np->lock);
                    releaseLock(&waitLock);
                    return pid;
                }
            }
            releaseLock(&np->lock);
        }

        if (!haveChildProcess)
        {
            releaseLock(&waitLock);
            return -1;
        }

        // printf("[WAIT]porcess id %x wait for %x\n", p->id, p);
        sleep(p, &waitLock);
    }
}

#define SIGCHLD 17
/**
 * @brief 功能：创建一个子进程；
 *
 * @param flags 创建的标志，如SIGCHLD；
 * @param stack 指定新进程的栈，可为0；
 * @param ptid 父线程ID；
 * @param tls TLS线程本地存储描述符；
 * @param ctid 子线程ID；
 * 成功则返回子进程的线程ID，失败返回-1；
 */
// void processFork(u64 flags, u64 stack, u64 ptid, u64 tls, u64 ctid)
int processFork(u32 flags, u64 stackVa, u64 ptid, u64 tls, u64 ctid)
{
    Thread *th;
    Trapframe *trapframe = getHartTrapFrame();
    Process *process, *myprocess = myProcess();
    int r = mainThreadAlloc(&th, myprocess->processId);
    if (r < 0)
    {
        return r;
    }
    process = th->process;
    process->cwd = myprocess->cwd;

    for (SegmentMap *psm = myprocess->segmentMapHead; psm; psm = psm->next)
    {
        SegmentMap *new = segmentMapAlloc();
        *new = *psm;
        segmentMapAppend(process, new);
    }

    for (int i = 0; i < process->fileDescription.hard; i++)
    {
        if (myprocess->ofile[i])
        {
            filedup(myprocess->ofile[i]);
            process->ofile[i] = myprocess->ofile[i];
        }
    }

    process->priority = myprocess->priority;
    process->brkHeapTop = myprocess->brkHeapTop;
    process->mmapHeapTop = myprocess->mmapHeapTop;
    process->shmHeapTop = myprocess->shmHeapTop;

    memmove(&th->trapframe, trapframe, sizeof(Trapframe));
    th->trapframe.a0 = 0;
    th->trapframe.kernelSp = getThreadTopSp(th);

    // if (stackVa != 0)
    // {
    //     th->trapframe.sp = stackVa;
    // }
    // if (ptid != NULL)
    // {
    //     copyout(myprocess->pgdir, ptid, (char *)&myprocess->processId, sizeof(u32));
    // }

    // if (ctid != NULL)
    // {
    //     copyout(myprocess->pgdir, ctid, (char *)&process->processId, sizeof(u32));
    // }

    u64 i,
        j, k;
    for (i = 0; i < 512; i++)
    {
        if (!(myprocess->pgdir[i] & PTE_VALID_BIT))
        {
            continue;
        }
        u64 *pa = (u64 *)PTE2PA(myprocess->pgdir[i]);
        for (j = 0; j < 512; j++)
        {
            if (!(pa[j] & PTE_VALID_BIT))
            {
                continue;
            }
            u64 *pa2 = (u64 *)PTE2PA(pa[j]);
            for (k = 0; k < 512; k++)
            {
                if (!(pa2[k] & PTE_VALID_BIT))
                {
                    continue;
                }
                u64 va = (i << 30) + (j << 21) + (k << 12);
                if (va == TRAMPOLINE || va == TRAPFRAME)
                {
                    continue;
                }
                if (pa2[k] & PTE_WRITE_BIT)
                {
                    pa2[k] |= PTE_COW_BIT;
                    pa2[k] &= ~PTE_WRITE_BIT;
                }
                pageMap(process->pgdir, va, PTE2PA(pa2[k]), PTE2PERM(pa2[k]));
            }
        }
    }

    LIST_INSERT_TAIL(&scheduleList[0], th, scheduleLink);

    return process->processId;
}

int threadFork(u64 stackVa, u64 ptid, u64 tls, u64 ctid)
{
    Thread *thread;
    Process *current = myProcess();
    int r = threadAlloc(&thread, current, stackVa);
    if (r < 0)
    {
        return r;
    }
    Trapframe *trapframe = getHartTrapFrame();
    memmove(&thread->trapframe, trapframe, sizeof(Trapframe));
    thread->trapframe.a0 = 0;
    thread->trapframe.tp = tls;
    thread->trapframe.kernelSp = getThreadTopSp(thread);
    thread->trapframe.sp = stackVa;
    if (ptid != 0)
    {
        copyout(current->pgdir, ptid, (char *)&thread->threadId, sizeof(u32));
    }
    thread->clearChildTid = ctid;

    // acquireLock(&scheduleListLock);
    LIST_INSERT_TAIL(&scheduleList[0], thread, scheduleLink);
    // releaseLock(&scheduleListLock);
    return thread->threadId;
}

/**
 * @brief 功能：创建一个子进程；
 *
 * @param flags 创建的标志，如SIGCHLD；
 * @param stack 指定新进程的栈，可为0；
 * @param ptid 父线程ID；
 * @param tls TLS线程本地存储描述符；
 * @param ctid 子线程ID；
 * 成功则返回子进程的线程ID，失败返回-1；
 */
int clone(u32 flags, u64 stackVa, u64 ptid, u64 tls, u64 ctid)
{
    if (flags & CLONE_VM)
    {
        return threadFork(stackVa, ptid, tls, ctid);
    }
    else
    {
        return processFork(flags, stackVa, ptid, tls, ctid);
    }
}

void kernelProcessCpuTimeEnd()
{
    Process *p = myProcess();
    p->processTime.lastKernelTime = r_time();
}
