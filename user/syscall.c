#ifndef _USER_SYSCALL_C_
#define _USER_SYSCALL_C_

#include "syscall_ids.h"
#include "syscall.h"
#include "unistd.h"

void exit(int code)
{
    syscall(SYS_exit, code);
}

ssize_t read(int fd, void *buf, size_t len)
{
    return syscall(SYS_read, fd, buf, len);
}

ssize_t write(int fd, const void *buf, size_t len)
{
    return syscall(SYS_write, fd, buf, len);
}

int brk(void *addr)
{
    return syscall(SYS_brk, addr);
}

pid_t fork()
{
    return syscall(SYS_clone, SIGCHLD, 0);
}

char *getcwd(char *buf, size_t size)
{
    return (char *)syscall(SYS_getcwd, buf, size);
}

int chdir(const char *path)
{
    return syscall(SYS_chdir, path);
}

int open(const char *path, int flags)
{
    return syscall(SYS_openat, AT_FDCWD, path, flags, O_RDWR);
}

int dup2(int old, int new)
{
    return syscall(SYS_dup3, old, new, 0);
}

int execve(const char *name, char *const argv[], char *const argp[])
{
    return syscall(SYS_execve, name, argv, argp);
}

int pipe(int fd[2])
{
    return syscall(SYS_pipe2, fd, 0);
}

int waitpid(int pid, int *code, int options)
{
    return syscall(SYS_wait4, pid, code, options, 0);
}

int wait(int *code)
{
    return waitpid((int)-1, code, 0);
}

int close(int fd)
{
    return syscall(SYS_close, fd);
}

#endif