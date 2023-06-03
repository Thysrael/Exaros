#include "unistd.h"
#include "stdio.h"

char *syscallList[] = {
    "brk",
    "chdir",
    "clone",
    "close",
    "dup",
    "dup2",
    "execve",
    "exit",
    "fork",
    "fstat",
    "getcwd",
    "getdents",
    "getpid",
    "getppid",
    "gettimeofday",
    "mkdir_",
    "mmap",
    "mount",
    "munmap",
    "open",
    "openat",
    "pipe",
    "read",
    "sleep",
    "test_echo",
    "times",
    "umount",
    "uname",
    "unlink",
    "wait",
    "waitpid",
    "write",
    "yield",
    // "sh",
};

char *argv[] = {NULL};

char *argp[] = {NULL};

// char *syscallList[] = {"sh"};
// char *argv[] = {"ls", "arg1", "arg2", 0};

int main()
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    // printf("hello, test.\n");
    // write(1, "*", 1);
    for (int i = 0; i < sizeof(syscallList) / sizeof(char *); i++)
    {
        // printf("test bin: %s\n", syscallList[i]);
        // execve("pipe", argv, argp);

        int pid = fork();
        if (pid == 0)
        {
            // printf("%d\n", i);
            // printf("pid0\n");
            execve(syscallList[i], argv, argp);
        }
        else
        {
            wait(0);
        }
    }
    return 0;
}
