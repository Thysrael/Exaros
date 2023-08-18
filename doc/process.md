# 进程和线程

## 进程控制块

进程控制块中存放着进程的基本信息，包括进程的基本信息，进程的页表等。

```c
// 进程控制块
typedef struct Process
{
    // Trapframe trapframe; // 进程异常时保存寄存器的地方
    struct ProcessTime processTime;
    CpuTimes cpuTime;
    ProcessListEntry link;
    u64 *pgdir;    // 进程页表地址
    u32 processId; // 进程 id
    u32 parentId;  // 父进程 id
    u32 priority;            // 优先级
    enum ProcessState state; // 进程状态
    Spinlock lock;
    DirMeta *cwd;        // 进程所在的路径
    File *ofile[NOFILE]; // 进程打开的文件
    u32 retValue; // 进程返回值
    u64 brkHeapTop;
    u64 mmapHeapTop;
    u64 shmHeapTop;
    DirMeta *execFile;
    int threadCount;
    struct ResourceLimit fileDescription;
    u32 pgid;
    u32 sid;
    SegmentMap *segmentMapHead; // 记录着这个进程的所有 segment 映射信息
    int ktime;
    int utime;
} Process;
```

## 线程控制块


线程的控制块需要保存的基本信息： 

* 寄存器上下文 trapframe
* 线程属于的进程
* 与调度相关的信息

```c
typedef struct Thread
{
    Trapframe trapframe;
    u64 awakeTime;
    u32 threadId;
    ThreadListEntry link;         // free thread
    ThreadListEntry scheduleLink; // yield(old)
    ThreadListEntry priSchedLink; // yield(with priority)
    enum ProcessState state;
    struct Spinlock lock;
    u64 channel; // wait Object
    u64 currentKernelSp;
    int reason;
    u32 retValue;
    u64 clearChildTid;
    Process *process;
    cpu_set_t cpuset;                 // CPU 亲和集
    int schedPolicy;                  // 调度策略
    sched_param schedParam;           // 调度参数
    bool killed;                      // 信号 SIGKILL
    SignalSet blocked;                // 屏蔽的信号集合（事实上是阻塞）
    u64 setAlarm;
    SignalContextList pendingSignal;  // 未决 = 仍然未处理的信号，最近的一条在最后面
    SignalContextList handlingSignal; // 正在处理的信号（同样的信号将被忽略），最近的一条在最前面
} Thread;
```


## 进程管理初始化

```c
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
```

初始化分为一下步骤：

1. 初始化进程列表
2. 初始化进程调度列表
3. 分别初始化进程控制块，为每个进程控制块设置内核页表地址（kernelSatp），和进程初始状态，并把进程控制块插入空闲进程列表
4. 将 `sscrach` 寄存器设置为当前 hart 的 `hartTrapFrame` 地址

## 用户进程的内存空间

![](img/process-0.png)

## 创建


函数 `processCreatePriority()`根据传入的 `binary` 创建新的进程，其中 `binary` 为进程的二进制文件。

- 申请进程控制块
- 设置进程优先级
- 调用 `loadElf(...)` 将 `binary` 中的段填入刚申请的进程。
- 设置起始执行的位置 `p->trapframe.epc = entryPoint;` 
- 将进程插入调度队列


```c
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
}
```


## 销毁

首先调用 `processFree()`，该函数做以下操作：

- 释放进程所持有的资源，包括页表、数据页、打开的文件
- 唤醒因为 `wait()` 函数而等待的父进程
- 将进程状态设置为 `ZOMBIE`

重新设置内核栈。

调度下一个进程。


## 运行线程

`ThreadRun(Thread * th)` 运行  `th` 线程。

- 如果之前该 cpu 正在运行其它进程，则将 `TRAMPOLINE` 中保存的寄存器信息拷贝到刚才运行的进程控制块中
- `th->state = RUNNING;`
- 接下来判断该线程之前让出 cpu 使用权的原因，若 `th->reason == 1` 表示该线程主动让出（sleep）。
  - 将当前进程的进程控制块的  Trapframe 拷贝到 trampoline 中的 Trapframe
  - 调用 `sleepRec()`，该函数从进程的内核态栈中读读取上下文并恢复执行
- 若 `p->reason == 0` ，则说明该线程之前不是通过 sleep 让出 cpu 的，
  - 将当前进程的进程控制块的  Trapframe 拷贝到 trampoline 中的 Trapframe
  - 设置内核栈
  - 调用 `userTrapReturn()` 函数恢复上下文。该函数的详细信息参见 **Trap** 部分。

## 进程调度

`yield` 函数的功能是让当前线程让出 CPU，然后切换并运行另一个可以运行的线程。

```c
/**
 * @brief
 * 1. 将当前进程改为 RUNNABLE，并插入调度列表末尾
 * 2. 交替访问两个调度队列，直到找到一个 RUNNABLE 的进程
 */
void yield()
{
    // printk("yield\n");
    int hartId = getTp();
    int count = processTimeCount[hartId];
    int point = processBelongList[hartId];
    Process *process = currentProcess[hartId];

    // printk("0: %d, 1:%d\n", processes[0].state, processes[1].state);

    if (process && process->state == RUNNING)
    {
        process->state = RUNNABLE;
    }
    // 防止死锁，假如只有一个进程而这个进程被 sleep 了，在这里应该接受外部中断
    intr_on();

    while ((count == 0) || !process || (process->state != RUNNABLE) || (process->awakeTime > r_time()))
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
        // printk("finding a process to yield... %d, %d, %d\n", count, process->state, (int)intr_get());
    }

    // 在这里关掉中断，不然 sleep 到一半的时候被打断
    intr_off();

    CNX_DEBUG("currentKernelSp: %lx \n", process->currentKernelSp);
    count--;
    processTimeCount[hartId] = count;
    processBelongList[hartId] = point;
    CNX_DEBUG("hartID %d yield process %lx\n", hartId, process->processId);

    // syscall_watetime 的范围值设置为 0
    if (process->awakeTime > 0)
    {
        getHartTrapFrame()->a0 = 0;
        process->awakeTime = 0;
    }
    processRun(process);
}
```

## Sleep/Wakeup 的实现

### Sleep

线程在内核态进入睡眠，需要先将进程控制块的状态设置为 `SLEEP`，`reason` 设置为 `1` 用来区分通过 sleep 让出 CPU 的进程和通过其他方式陷入内核的线程。

最后将此时的寄存器状态保存在栈中，将当前的 sp 指针值存入 `proceee->currentKernelSp`, 然后调用 `SleepSave` 将寄存器保存在栈中。

```c
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
```

### Wakeup

wakeup 的作用是唤醒某个等待队列中的全部进程，实现方法是遍历进程列表中的所有进程，找到当前等待队列中的进程并强其状态设置为 `RUNNABLE`。

```c

/**
 * @brief 唤醒在等待队列 channel 中的所有进程
 * 其实并没有队列，channel 只是一个整数，记录在 Process 结构体中
 *
 * @param channel
 */
void wakeup(void *channel)
{
    for (int i = 0; i < PROCESS_TOTAL_NUMBER; i++)
    {
        // if (&processes[i] != myProcess())
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
```

被唤醒后的进程可以被 yield 函数调度，在重新运行进程的时候会通过 `process->reason` 判断这个进程是不是因为 `sleep` 让出 CPU 的，如果不是，则从 `process->trapframe` 中恢复寄存器，如果是，则从栈中恢复寄存器。