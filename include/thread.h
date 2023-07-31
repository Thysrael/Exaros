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
#include <signal.h>
#include <sched.h>

typedef LIST_ENTRY(ThreadListEntry, Thread) ThreadListEntry;

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

#define LOCALE_NAME_MAX 23

struct __locale_map
{
    const void *map;
    u64 map_size;
    char name[LOCALE_NAME_MAX + 1];
    const struct __locale_map *next;
};

struct __locale_struct
{
    const struct __locale_map *cat[6];
};

struct pthread
{
    /* Part 1 -- these fields may be external or
     * internal (accessed via asm) ABI. Do not change. */
    struct pthread *self;
    u64 *dtv;
    struct pthread *prev, *next; /* non-ABI */
    u64 sysinfo;
    u64 canary, canary2;

    /* Part 2 -- implementation details, non-ABI. */
    int tid;
    int errno_val;
    volatile int detach_state;
    volatile int cancel;
    volatile unsigned char canceldisable, cancelasync;
    unsigned char tsd_used : 1;
    unsigned char dlerror_flag : 1;
    unsigned char *map_base;
    u64 map_size;
    void *stack;
    u64 stack_size;
    u64 guard_size;
    void *result;
    struct __ptcb *cancelbuf;
    void **tsd;
    struct
    {
        volatile void *volatile head;
        long off;
        volatile void *volatile pending;
    } robust_list;
    volatile int timer_id;
    struct __locale_struct *locale;
    volatile int killlock[1];
    char *dlerror_buf;
    void *stdio_locks;

    /* Part 3 -- the positions of these fields relative to
     * the end of the structure is external and internal ABI. */
    u64 canary_at_end;
    u64 *dtv_copy;
};

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