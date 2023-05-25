/**
 * @file process.h
 * @brief 进程
 * @date 2023-04-13
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _PROCESS_H
#define _PROCESS_H

#include <types.h>
#include <yield.h>
#include <queue.h>
#include <memory.h>
#include <string.h>
#include <dirmeta.h>
#include <file.h>
#include <lock.h>

#define NOFILE 1024 // Number of fds that a process can open
#define LOG_PROCESS_NUM 10
#define PROCESS_TOTAL_NUMBER (1 << LOG_PROCESS_NUM)

typedef LIST_HEAD(ProcessList, Process) ProcessList;
#define PROCESS_OFFSET(processId) ((processId) & (PROCESS_TOTAL_NUMBER - 1))

/**
 * @brief 创建进程, x 是要创建的进程名字，y 是优先级
 *
 */
#define PROCESS_CREATE_PRIORITY(x, y)                                \
    {                                                                \
        extern u8 binary##x##Start[];                                \
        extern int binary##x##Size;                                  \
        processCreatePriority(binary##x##Start, binary##x##Size, y); \
    }

// 进程状态
enum ProcessState
{
    UNUSED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

struct ProcessTime
{
    long lastUserTime;
    long lastKernelTime;
};

/**
 * @brief 保存进程寄存器和其他信息
 *
 */
typedef struct Trapframe
{
    u64 kernelSatp;
    u64 kernelSp;
    u64 trapHandler; // trap handler address
    u64 epc;         // error pc
    u64 kernelHartId;
    u64 ra;
    u64 sp;
    u64 gp;
    u64 tp;
    u64 t0;
    u64 t1;
    u64 t2;
    u64 s0;
    u64 s1;
    u64 a0;
    u64 a1;
    u64 a2;
    u64 a3;
    u64 a4;
    u64 a5;
    u64 a6;
    u64 a7;
    u64 s2;
    u64 s3;
    u64 s4;
    u64 s5;
    u64 s6;
    u64 s7;
    u64 s8;
    u64 s9;
    u64 s10;
    u64 s11;
    u64 t3;
    u64 t4;
    u64 t5;
    u64 t6;
    u64 ft0;
    u64 ft1;
    u64 ft2;
    u64 ft3;
    u64 ft4;
    u64 ft5;
    u64 ft6;
    u64 ft7;
    u64 fs0;
    u64 fs1;
    u64 fa0;
    u64 fa1;
    u64 fa2;
    u64 fa3;
    u64 fa4;
    u64 fa5;
    u64 fa6;
    u64 fa7;
    u64 fs2;
    u64 fs3;
    u64 fs4;
    u64 fs5;
    u64 fs6;
    u64 fs7;
    u64 fs8;
    u64 fs9;
    u64 fs10;
    u64 fs11;
    u64 ft8;
    u64 ft9;
    u64 ft10;
    u64 ft11;
} Trapframe;

typedef LIST_ENTRY(ProcessListEntry, Process) ProcessListEntry;

// 进程控制块
typedef struct Process
{
    Trapframe trapframe; // 进程异常时保存寄存器的地方
    struct ProcessTime processTime;
    // CpuTimes cpuTime;
    ProcessListEntry link;
    ProcessListEntry scheduleLink;
    // u64 awakeTime;
    u64 *pgdir;    // 进程页表地址
    u32 processId; // 进程 id
    u32 parentId;  // 父进程 id
    // LIST_ENTRY(Process) scheduleLink;
    u32 priority;            // 优先级
    enum ProcessState state; // 进程状态
    Spinlock lock;
    DirMeta *cwd;            // 进程所在的路径
    File *ofile[NOFILE];     // 进程打开的文件
    u64 channel;             // 等待队列
    u64 currentKernelSp;
    int reason;
    u32 retValue; // 进程返回值
    u64 brkHeapTop;
    u64 mmapHeapTop;
    DirMeta *execFile;
    // SignalSet blocked;
    // SignalSet pending;
    // u64 setChildTid;
    // u64 clearChildTid;
    // int threadCount;
    // struct ResourceLimit fileDescription;
    // ProcessSegmentMap *segmentMapHead;
} Process;

// functions
Process *myProcess();
void processInit();
u32 generateProcessId(Process *p);
void processDestory(Process *p);
void processFree(Process *p);
int pid2Process(u32 processId, Process **process, int checkPerm);
int allocPgdir(Page **page);
void pgdirFree(u64 *pgdir);
int setupProcess(Process *p);
int processAlloc(Process **new, u64 parentId);
void processCreatePriority(u8 *binary, u32 size, u32 priority);
u64 getHartKernelTopSp();
void processRun(Process *p);
void yield();
u64 getProcessTopSp(Process *p);
void sleep(void *channel, Spinlock *lk);
void wakeup(void *channel);
void processFork(u64 flags, u64 stack, u64 ptid, u64 tls, u64 ctid);

u64 exec(char *path, char **argv);

#endif