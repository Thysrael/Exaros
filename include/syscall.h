#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYSCALL_PUTCHAR 4

extern void (*syscallVector[])(void);

#endif