#ifndef _USER_SYSCALL_C_
#define _USER_SYSCALL_C_

#include "../include/syscall.h"
#include "lib/syscall.h"
#include <unistd.h>

void exit(int code)
{
    syscall(SYSCALL_EXIT, code);
}

ssize_t read(int fd, void *buf, size_t len)
{
    return (ssize_t)syscall(SYSCALL_READ, fd, buf, len);
}

ssize_t write(int fd, const void *buf, size_t len)
{
    return (ssize_t)syscall(SYSCALL_WRITE, fd, buf, len);
}

int brk(void *addr)
{
    return (int)syscall(SYSCALL_BRK, addr);
}

void *sbrk(intptr_t increment)
{
    return (void *)syscall(SYSCALL_SBRK, increment);
}

#endif