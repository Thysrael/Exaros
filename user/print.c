#include "lib/print.h"
#include "lib/syscall.h"
#include "../include/syscall.h"

void putchar(u8 c)
{
    msyscall(SYSCALL_PUTCHAR, c, 0, 0, 0, 0, 0);
}
