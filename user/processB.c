#include "stdio.h"

int main(int argc, char **argv)
{
    for (int i = 1; i <= 100; ++i)
    {
        putchar('@');
    }
    putchar('\n');
    while (1)
        ;
    return 0;
}