#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "file.h"

int main(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        write(stdout, argv[i], strlen(argv[i]));
        if (i + 1 < argc)
        {
            write(stdout, " ", 1);
        }
        else
        {
            write(stdout, "\n", 1);
        }
    }
    return 0;
}