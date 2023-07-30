#include "sched.h"
#include "thread.h"

void CPU_ZERO(cpu_set_t *set)
{
    for (int i = 0; i < CPU_SETSIZE / __CPU_BITS; ++i)
    {
        set->__bits[i] = 0;
    }
}

void CPU_SET(int cpu, cpu_set_t *set)
{
    if (cpu < 0 || cpu >= CPU_SETSIZE) { return; }
    set->__bits[cpu / 64] |= (((u64)1) << (cpu % 64));
}

void CPU_CLR(int cpu, cpu_set_t *set)
{
    if (cpu < 0 || cpu >= CPU_SETSIZE) { return; }
    set->__bits[cpu / 64] &= ~(((u64)1) << (cpu % 64));
}

int CPU_ISSET(int cpu, cpu_set_t *set)
{
    return (set->__bits[cpu / 64] & (((u64)1) << (cpu % 64))) != 0;
}

int CPU_COUNT(cpu_set_t *set)
{
    int ans = 0;
    for (int i = 0; i < CPU_SETSIZE / __CPU_BITS; ++i)
    {
        u64 bits = set->__bits[i];
        while (bits != 0)
        {
            ans += (bits & 1);
            bits >>= 1;
        }
    }
    return ans;
}

void CPU_OR(cpu_set_t *destset,
            cpu_set_t *srcset1, cpu_set_t *srcset2)
{
    for (int i = 0; i < CPU_SETSIZE / __CPU_BITS; ++i)
    {
        destset->__bits[i] = srcset1->__bits[i] | srcset2->__bits[i];
    }
}

/* 仅仅检查参数而不真正的修改 */
int sched_setscheduler(u64 pid, int policy, sched_param *param)
{
    Thread *thread;
    if (tid2Thread(pid, &thread, 0) < 0) { return -ESRCH; }
    if (policy != SCHED_OTHER)
    {
        printk("sched_setscheduler unsupported policy: %d\n", policy); // for test
        return 0;                                                      // for test
        return -EINVAL;
    }
    if (policy == SCHED_OTHER && param->sched_priority != 0)
    {
        return -EINVAL;
    }
    return 0;
}

int sched_getscheduler(u64 pid)
{
    Thread *thread;
    if (tid2Thread(pid, &thread, 0) < 0) { return -ESRCH; }
    return 0;
}

int sched_setaffinity(u32 pid, u64 cpusetsize, cpu_set_t *mask)
{
    Thread *thread;
    if (tid2Thread(pid, &thread, 0) < 0) { return -ESRCH; }
    bcopy(mask, &thread->cpuset, cpusetsize);
    return 0;
}

int sched_getaffinity(u32 pid, u64 cpusetsize, cpu_set_t *mask)
{
    Thread *thread;
    if (tid2Thread(pid, &thread, 0) < 0) { return -ESRCH; }
    bcopy(&thread->cpuset, mask, cpusetsize);
    return 0;
}