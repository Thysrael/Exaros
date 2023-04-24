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
#include <driver.h>
#include <mem_layout.h>
#include <error.h>
#include <yield.h>
#include <trap.h>
#include <elf.h>

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

    printk("processes address:%lx\n", (u64)(&processes));
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

void processFree(Process *p)
{
    // todo
    // pgdirFree(p->pgdir);
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

int allocPgdir(Page **page)
{
    int r = pageAlloc(page);
    if (r)
        return r;
    (*page)->ref++;
    return 0;
}

/**
 * @brief 为进程申请页表，并且建立 trampoline 和 trapframe 的映射
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

u64 getHartKernelTopSp()
{
    extern char kernelStack[];
    return (u64)kernelStack + KERNEL_STACK_SIZE * (getTp() + 1);
}

void processRun(Process *p)
{
    Trapframe *trapframe = getHartTrapFrame();
    if (currentProcess[getTp()])
    {
        bcopy(trapframe, &currentProcess[getTp()]->trapframe, sizeof(Trapframe));
    }
    p->state = RUNNING;

    printk("%lx\n", trapframe);
    int hartid = getTp();
    currentProcess[hartid] = p;

    // 切换页表
    // 拷贝 trapframe

    printk("%lx\n", trapframe);
    printk("address: %lx\n", (u64) & (currentProcess[getTp()]->trapframe));
    bcopy(&(currentProcess[hartid]->trapframe), trapframe, sizeof(Trapframe));

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
    printk("yield\n");
    int hartId = getTp();
    int count = processTimeCount[hartId];
    int point = processBelongList[hartId];
    Process *process = currentProcess[hartId];

    if (process && process->state == RUNNING)
    {
        bcopy(getHartTrapFrame(), &process->trapframe, sizeof(Trapframe));
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
            count = 1;
        }
        printk("finding a process to yield...\n");
    }
    count--;
    processTimeCount[hartId] = count;
    processBelongList[hartId] = point;
    printk("hartID %d yield process %lx\n", hartId, process->processId);
    processRun(process);
}

u64 getProcessTopSp(Process *p)
{
    return KERNEL_PROCESS_SP_TOP - (u64)(p - processes) * 10 * PAGE_SIZE;
}