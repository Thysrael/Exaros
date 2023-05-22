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
#include <lock.h>

Process processes[PROCESS_TOTAL_NUMBER];
ProcessList freeProcesses, usedProcesses;
ProcessList scheduleList[2];
Process *currentProcess[CORE_NUM] = {0};
static int processTimeCount[CORE_NUM] = {0};
static int processBelongList[CORE_NUM] = {0};

/**
 * @brief 获取当前这个核正在运行的进程
 *
 * @return Process*
 */
Process *myProcess()
{
    int hartId = getTp();
    if (currentProcess[hartId] == 0)
    {
        panic("failed to get cuffent process num\n");
    }
    Process *ret = currentProcess[hartId];
    return ret;
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
    LIST_INIT(&scheduleList[0]);
    LIST_INIT(&scheduleList[1]);
    int i;
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--)
    {
        processes[i].state = UNUSED;
        processes[i].trapframe.kernelSatp = MAKE_SATP(SV39, PA2PPN(kernelPageDirectory));
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);
    }
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

/**
 * @brief 终止进程 p，修改 sp 指针为内核栈地址，然后重新调度进程
 *
 * @param p
 */
void processDestory(Process *p)
{
    processFree(p);
    int hartId = getTp();
    if (currentProcess[hartId] == p)
    {
        currentProcess[hartId] = NULL;
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

    // todo 写好文件系统之后要在这里释放掉进程占用的文件资源

    // if (p->parentId > 0)
    // {
    //     Process *parentProcess;
    //     pid2Process(p->parentId, &parentProcess, 0);
    //     // if (r == 0)
    //     // {
    //     //     wakeup(parentProcess);
    //     // }
    // }
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
    int hartId = getTp();

    if (processId == 0)
    {
        *process = currentProcess[hartId];
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
        if (p != currentProcess[hartId] && p->parentId != currentProcess[hartId]->processId)
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
    int r = pageAlloc(page);
    if (r)
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

    // 设置内核栈，就是进程在进入内核 trap 的时候使用的栈
    // 为每个进程开一页的内核栈
    r = pageAlloc(&page);
    kernelPageMap(kernelPageDirectory, getProcessTopSp(p) - PAGE_SIZE, page2PA(page),
                  PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);

    extern char trampoline[];
    extern char trapframe[];
    kernelPageMap(p->pgdir, TRAMPOLINE, ((u64)trampoline), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    kernelPageMap(p->pgdir, TRAPFRAME, ((u64)trapframe), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
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
    if ((r = setupProcess(p)) < 0)
    {
        return r;
    }

    p->processId = generateProcessId(p);
    p->state = RUNNABLE;
    p->parentId = parentId;
    p->trapframe.kernelSp = getProcessTopSp(p);
    p->trapframe.sp = USER_STACK_TOP;

    *new = p;
    return 0;
}

/**
 * @brief 给定一段二进制可执行文件，创建一个进程
 *
 * @param binary 二进制地址
 * @param size 二进制大小
 * @param priority 进程优先级
 */
void processCreatePriority(u8 *binary, u32 size, u32 priority)
{
    Process *p;
    int r = processAlloc(&p, 0);
    if (r)
        return;
    p->priority = priority;
    u64 entryPoint; // 入口地址
    if (loadElf(binary, size, &entryPoint, p) != 0)
    {
        panic("failed to load elf\n");
    }
    p->trapframe.epc = entryPoint;

    LIST_INSERT_TAIL(&scheduleList[0], p, scheduleLink);

    printk("created a process.\n");
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
 * @brief 运行进程 p
 *
 * @param p
 */
void processRun(Process *p)
{
    Trapframe *trapframe = getHartTrapFrame();

    // 保存当前进程的 trapfreme 到进程结构体中
    if (currentProcess[getTp()])
    {
        memmove(&currentProcess[getTp()]->trapframe, trapframe, sizeof(Trapframe));
    }
    p->state = RUNNING;

    int hartid = getTp();
    currentProcess[hartid] = p;

    // 切换页表
    // 拷贝进程的 trapframe 到 hart 对应的 trapframe
    memmove(trapframe, &(currentProcess[hartid]->trapframe), sizeof(Trapframe));

    u64 sp = getHartKernelTopSp(p);
    asm volatile("ld sp, 0(%0)"
                 :
                 : "r"(&sp)
                 : "memory");
    userTrapReturn();
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
    Process *process = currentProcess[hartId];

    if (process && process->state == RUNNING)
    {
        memmove(&process->trapframe, getHartTrapFrame(), sizeof(Trapframe));
        process->state = RUNNABLE;
    }

    while ((count == 0) || !process || (process->state != RUNNABLE))
    {
        if (process)
            LIST_INSERT_TAIL(&scheduleList[point ^ 1], process, scheduleLink);
        if (LIST_EMPTY(&scheduleList[point]))
            point ^= 1;
        if (!(LIST_EMPTY(&scheduleList[point])))
        {
            process = LIST_FIRST(&scheduleList[point]);
            LIST_REMOVE(process, scheduleLink);
            count = process->priority;
        }
        // printk("finding a process to yield...\n");
    }
    count--;
    processTimeCount[hartId] = count;
    processBelongList[hartId] = point;
    // printk("hartID %d yield process %lx\n", hartId, process->processId);
    processRun(process);
}

/**
 * @brief 获取进程的内核栈地址
 *
 * @param p
 * @return u64
 */
u64 getProcessTopSp(Process *p)
{
    return KERNEL_PROCESS_SP_TOP - (u64)(p - processes) * 10 * PAGE_SIZE;
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
    Process *p = myProcess();
    acquireLock(&(p->lock));
    releaseLock(lk);

    p->channel = (u64)channel;
    p->state = SLEEPING;
    p->reason = 1;
    releaseLock(&(p->lock));

    asm volatile("sd sp, 0(%0)"
                 :
                 : "r"(&p->currentKernelSp));

    // 保存寄存器
    sleepSave();

    acquireLock(&p->lock);
    p->channel = 0;
    releaseLock(&p->lock);
    acquireLock(lk);
}
/**
 * @brief 唤醒在等待队列 channel 中的所有进程
 * 其实并没有队列，channel 只是一个整数，记录在 Process 结构体中
 *
 * @param channel
 */
void wakeup(void *channel)
{
    // 遍历所有进程，找到 process->channel = channel 的
    // 这个是不是可以改进？
    for (int i = 0; i < PROCESS_TOTAL_NUMBER; i++)
    {
        if (&processes[i] != myProcess())
        {
            acquireLock(&processes[i].lock);
            if (processes[i].state == SLEEPING && processes[i].channel == (u64)channel)
            {
                processes[i].state = RUNNABLE;
            }
            releaseLock(&processes[i].lock);
        }
    }
}

/**
 * @brief
 *
 */
// void wait(int targetProcessId, u64 addr)
// {
// }

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
void processFork(u64 flags, u64 stack, u64 ptid, u64 tls, u64 ctid)
{
}