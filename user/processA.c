#include <print.h>

int userMain(int argc, char **argv)
{
    for (int i = 1; i <= 20; ++i)
    {
        putchar('a' + i);
    }

    while (1)
        ;
    return 0;
}