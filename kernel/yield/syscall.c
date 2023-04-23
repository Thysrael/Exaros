#include <../../include/syscall.h>
#include <trap.h>
#include <driver.h>

void syscallPutchar()
{
    putchar(getHartTrapFrame()->a0);
};

void (*syscallVector[])(void) = {
    [0] syscallPutchar,
    [1] syscallPutchar,
    [2] syscallPutchar,
    [3] syscallPutchar,
    [SYSCALL_PUTCHAR] syscallPutchar,
};
