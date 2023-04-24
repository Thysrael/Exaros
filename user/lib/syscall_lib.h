#ifndef _USER_SYSCALLLIB_H_
#define _USER_SYSCALLLIB_H_

#include "../../include/syscall.h"
#include "../../include/types.h"
#include "syscall.h"

static inline int exit(int ec)
{
    return msyscall(SYSCALL_EXIT, (u64)ec, 0, 0, 0, 0, 0);
}

#endif