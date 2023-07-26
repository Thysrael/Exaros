/**
 * @file sysarg.h
 * @brief 用来处理系统调用的时候的参数
 * @date 2023-05-11
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _SYSARG_H_
#define _SYSARG_H_

#include "types.h"

int argfd(int n, int *pfd, File **pf);
int argint(int, int *);
int argstr(int, char *, int);
int argaddr(int, u64 *);
int fetchstr(u64, char *, int);
int fetchaddr(u64, u64 *);
int copyInstr(u64 *pagetable, char *dst, u64 srcva, u64 max);
int fdalloc(File *f);
#endif