/**
 * @file cat.c
 * @brief 用户进程 cat
 * @date 2023-05-25
 *
 * @copyright Copyright (c) 2023
 */

#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

char buf[512];

void cat(int fd)
{
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        if (write(stdout, buf, n) != n)
        {
            write(stderr, "cat: write error\n", 18);
            exit(1);
        }
    }
    if (n < 0)
    {
        write(stderr, "cat: read error\n", 17);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    int fd, i;

    if (argc <= 1)
    {
        cat(0);
        exit(0);
    }
    for (i = 1; i < argc; i++)
    {
        if ((fd = open(argv[i], 0)) < 0)
        {
            printf("cat: cannot open %s\n", argv[i]);
            exit(1);
        }
        cat(fd);
        close(fd);
    }
    return 0;
}