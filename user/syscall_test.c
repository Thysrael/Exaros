#include "unistd.h"
#include "stdio.h"

char *syscallList[] = {"sh"};

char *argv[] = {NULL};

char *argp[] = {NULL};

// char *syscallList[] = {"sh"};
// char *argv[] = {"ls", "arg1", "arg2", 0};

int main()
{
    printf("hello, test.\n");
    for (int i = 0; i < sizeof(syscallList) / sizeof(char *); i++)
    {
        printf("test bin: %s\n", syscallList[i]);
        execve(syscallList[i], argv, argp);
        // int pid = fork();
        // if (pid == 0)
        // {
        //     printf("%d\n", i);
        //     printf("pid0\n");
        //     // execve(syscallList[i], argv, argp);
        // }
        // else
        // {
        //     wait(0);
        // }
    }
    return 0;
}
