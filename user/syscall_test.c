#include "unistd.h"
#include "stdio.h"

char *syscallList[] = {"sh"};

char *argv[] = {NULL};

char *argp[] = {NULL};

// char *syscallList[] = {"sh"};
// char *argv[] = {"ls", "arg1", "arg2", 0};

int main()
{
    putchar('#');
    for (int i = 0; i < sizeof(syscallList) / sizeof(char *); i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            putchar('p');
            putchar('i');
            putchar('d');
            putchar('0');
            putchar('\n');
            execve(syscallList[i], argv, argp);
        }
        else
        {
            wait(0);
        }
    }
    return 0;
}
