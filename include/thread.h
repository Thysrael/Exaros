#ifndef _THREAD_H_
#define _THREAD_H_

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
#include <fs.h>

typedef LIST_ENTRY(ThreadListEntry, Thread) ThreadListEntry;

typedef struct Thread
{
    Trapframe trapframe;
    ThreadListEntry link;
    u64 awakeTime;
    u32 threadId;
    ThreadListEntry scheduleLink;
    enum ProcessState state;
    struct Spinlock lock;
    u64 channel; // wait Object
    u64 currentKernelSp;
    int reason;
    u32 retValue;
    // SignalSet blocked;
    // SignalSet pending;
    // SignalSet processing;
    // u64 setChildTid;
    // u64 clearChildTid;
    Process *process;
    // u64 robustHeadPointer;
    // bool killed;
    // struct SignalContextList waitingSignal;
} Thread;

LIST_HEAD(ThreadList, Thread);

Thread *myThread(); // Get current running thread in this hart
void threadFree(Thread *th);
u64 getThreadTopSp(Thread *th);

void threadInit();
int mainThreadAlloc(Thread **new, u64 parentId);
int threadAlloc(Thread **new, Process *process, u64 userSp);
int tid2Thread(u32 threadId, struct Thread **thread, int checkPerm);
void threadRun(Thread *thread);
void threadDestroy(Thread *thread);
void threadSetup(Thread *th);

#endif