#include "unistd.h"
#include "stdio.h"

char *test[] = {
    "print_test"};

char *argv[] = {NULL};

char *argp[] = {NULL};

int main()
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    for (int i = 0; i < sizeof(test) / sizeof(char *); i++)
    {
        int pid = fork();
        if (pid == 0) { execve(test[i], argv, argp); }
        else { wait(0); }
    }
    return 0;
}
