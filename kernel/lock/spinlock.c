#include <lock.h>

// void initLock

void acquireLock(Spinlock *lock)
{
    interruptPush();
    if (holding(lock))
    {
        panic("You have acquire the lock! The lock is %s\n", lock->name);
    }

    // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
    //   a5 = 1
    //   s1 = &lk->locked
    //   amoswap.w.aq a5, a5, (s1)
    while (__sync_lock_test_and_set(&lock->locked, 1) != 0)
        ;

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen strictly after the lock is acquired.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    lock->hart = myHart();
}

void releaseLock(Spinlock *lock)
{
    if (!holding(lock))
    {
        panic("You have release the lock! The lock is %s\n", lock->name);
    }

    lock->hart = 0;

    __sync_synchronize();

    // Release the lock, equivalent to lk->locked = 0.
    // This code doesn't use a C assignment, since the C standard
    // implies that an assignment might be implemented with
    // multiple store instructions.
    // On RISC-V, sync_lock_release turns into an atomic swap:
    //   s1 = &lk->locked
    //   amoswap.w zero, zero, (s1)
    __sync_lock_release(&lock->locked);
    interruptPop();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int holding(Spinlock *lk)
{
    int r;
    r = (lk->locked && lk->hart == myHart());
    return r;
}