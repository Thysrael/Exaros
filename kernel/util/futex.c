#include "futex.h"
#include "types.h"

typedef struct FutexQueue
{
    u64 addr;
    Thread *thread;
    u8 valid;
} FutexQueue;

FutexQueue futexQueue[FUTEX_COUNT];

void futexWait(u64 addr, Thread *th, TimeSpec *ts)
{
    for (int i = 0; i < FUTEX_COUNT; i++)
    {
        if (!futexQueue[i].valid)
        {
            futexQueue[i].valid = true;
            futexQueue[i].addr = addr;
            futexQueue[i].thread = th;
            if (ts)
            {
                th->awakeTime = ts->second * 1000000 + ts->microSecond;
            }
            else
            {
                if (th->state == RUNNABLE)
                    LIST_REMOVE(th, priSchedLink);
                th->state = SLEEPING;
            }
            // yield();
            callYield();
            // not reach here!!!
        }
    }
    panic("No futex Resource!\n");
}

extern struct ThreadList priSchedList[140];
void futexWake(u64 addr, int n)
{
    for (int i = 0; i < FUTEX_COUNT && n; i++)
    {
        if (futexQueue[i].valid && futexQueue[i].addr == addr)
        {
            if (futexQueue[i].thread->state != RUNNABLE)
            {
                futexQueue[i].thread->state = RUNNABLE;
                int pri = 99 - futexQueue[i].thread->schedParam.schedPriority;
                LIST_INSERT_TAIL(&priSchedList[pri], futexQueue[i].thread, priSchedLink);
            }
            futexQueue[i].thread->trapframe.a0 = 0; // set next yield accept!
            futexQueue[i].valid = false;
            n--;
        }
    }
    // yield();
}

void futexRequeue(u64 addr, int n, u64 newAddr)
{
    for (int i = 0; i < FUTEX_COUNT && n; i++)
    {
        if (futexQueue[i].valid && futexQueue[i].addr == addr)
        {
            if (futexQueue[i].thread->state != RUNNABLE)
            {
                futexQueue[i].thread->state = RUNNABLE;
                int pri = 99 - futexQueue[i].thread->schedParam.schedPriority;
                LIST_INSERT_TAIL(&priSchedList[pri], futexQueue[i].thread, priSchedLink);
            }
            futexQueue[i].thread->trapframe.a0 = 0; // set next yield accept!
            futexQueue[i].valid = false;
            n--;
        }
    }
    for (int i = 0; i < FUTEX_COUNT; i++)
    {
        if (futexQueue[i].valid && futexQueue[i].addr == addr)
        {
            futexQueue[i].addr = newAddr;
        }
    }
}

void futexClear(Thread *thread)
{
    for (int i = 0; i < FUTEX_COUNT; i++)
    {
        if (futexQueue[i].valid && futexQueue[i].thread == thread)
        {
            futexQueue[i].valid = false;
        }
    }
}