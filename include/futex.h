#ifndef _FUTEX_H_
#define _FUTEX_H_

#include "types.h"
#include "thread.h"

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_REQUEUE 3

#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_COUNT 64

u64 futex(u64 addr, int futex_op, u32 val, TimeSpec *ts);
void futexWait(u64 addr, Thread *thread, TimeSpec *ts);
void futexWake(u64 addr, int n);
void futexRequeue(u64 addr, int n, u64 newAddr);
void futexClear(Thread *thread);

#endif