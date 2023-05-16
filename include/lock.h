#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#include "types.h"

typedef struct Spinlock
{
    u8 locked; // 在锁可以被获得时值为0，而当锁已经被获得时值为非零

    int times;
    // For debugging:
    char *name;        // Name of lock.
    struct Hart *hart; // The cpu holding the lock.
} Spinlock;

// spinlock.c
void acquireLock(Spinlock *lock);
void releaseLock(Spinlock *lock);
int holding(Spinlock *lk);

// interrupt.c
void interruptPush(void);
void interruptPop(void);

#endif