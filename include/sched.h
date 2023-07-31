/**
 * @file sched.h
 * @brief 调度相关
 * @date 2023-07-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _SCHED_H_
#define _SCHED_H_

#include "types.h"

typedef u64 __CPU_BITTYPE;
#define CPU_SETSIZE 1024
#define __CPU_BITS 64

#define SCHED_OTHER 0
#define SCHED_NORMAL 0 // 和 SCHED_OTHER 等价
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3
#define SCHED_IDLE 5
#define SCHED_DEADLINE 6

#define PRI_MIN -100
#define PRI_MAX 39
// 闭区间

typedef struct
{
    __CPU_BITTYPE __bits[CPU_SETSIZE / __CPU_BITS];
} cpu_set_t;

typedef struct
{
    int schedPriority;
} sched_param;

/* Clears set, so that it contains no CPUs. */
void CPU_ZERO(cpu_set_t *set);
/* Add CPU cpu to set. */
void CPU_SET(int cpu, cpu_set_t *set);
/* Remove CPU cpu from set. */
void CPU_CLR(int cpu, cpu_set_t *set);
/* Test to see if CPU cpu is a member of set. */
int CPU_ISSET(int cpu, cpu_set_t *set);
/* Return the number of CPUs in set. */
int CPU_COUNT(cpu_set_t *set);

void CPU_AND(cpu_set_t *destset,
             cpu_set_t *srcset1, cpu_set_t *srcset2);
void CPU_OR(cpu_set_t *destset,
            cpu_set_t *srcset1, cpu_set_t *srcset2);
void CPU_XOR(cpu_set_t *destset,
             cpu_set_t *srcset1, cpu_set_t *srcset2);
int CPU_EQUAL(cpu_set_t *set1, cpu_set_t *set2);

int sched_setscheduler(u64 pid, int policy, sched_param *param);
int sched_getscheduler(u64 pid);
int sched_getparam(u64 pid, sched_param *param);
int sched_setaffinity(u32 pid, u64 cpusetsize, cpu_set_t *mask);
int sched_getaffinity(u32 pid, u64 cpusetsize, cpu_set_t *mask);

/* 还没有设置 clone, fork 时 cpuset 的继承情况 */

#endif