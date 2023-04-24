#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYSCALL_PUTCHAR 4
#define SYSCALL_EXIT 93

extern void (*syscallVector[])(void);

#endif