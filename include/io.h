#ifndef _IO_H_
#define _IO_H_

#include "types.h"
#include "lock.h"

#define IO_BUFFER 65536

struct AsynInput
{
    u8 buffer[IO_BUFFER];
    struct Spinlock lock;
    u64 head, tail;
};

void asynInputInit();
void asynInput();
bool hasChar();

#endif