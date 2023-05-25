#include "unistd.h"

char *syscallList[] = {"getpid", "getppid", "dup", "exit", "yield", "pipe", "times", "gettimeofday", "sleep", "dup2",
                       "getcwd", "open", "read", "write", "close", "execve", "chdir", "waitpid", "brk", "wait", "fork", "mkdir_",
                       "openat", "fstat", "mmap", "munmap", "clone", "mount", "umount", "unlink", "getdents", "uname", "sh"};

char *argv[] = {NULL};

char *argp[] = {NULL};

// char *syscallList[] = {"sh"};
// char *argv[] = {"ls", "arg1", "arg2", 0};

int main()
{
    for (int i = 0; i < sizeof(syscallList) / sizeof(char *); i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            execve(syscallList[i], argv, argp);
        }
        else
        {
            wait(0);
        }
    }
    return 0;
}
