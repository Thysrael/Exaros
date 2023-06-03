#include "unistd.h"
#include "stdio.h"

int main(int argc, char **argv)
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    for (int i = 1; i <= 100; ++i)
    {
        putchar('@');
    }
    putchar('\n');
    while (1)
        ;
    return 0;
}