#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "file.h"

void ls(char *path)
{
    static char buf[1024];
    int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd == -1)
    {
        printf("open error\n");
        return;
    }
    printf("file type  d_reclen  d_off   d_name\n");
    for (;;)
    {
        int nread = getdents(fd, (struct linux_dirent64 *)buf, 1024);
        if (nread == -1)
            printf("getdents error");

        if (nread == 0)
            break;

        for (long bpos = 0; bpos < nread;)
        {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + bpos);
            unsigned char d_type = d->d_type;
            printf("%-12s", (d_type == DT_REG) ? "regular" : (d_type == DT_DIR) ? "directory" :
                                                         (d_type == DT_FIFO)    ? "FIFO" :
                                                         (d_type == DT_SOCK)    ? "socket" :
                                                         (d_type == DT_LNK)     ? "symlink" :
                                                         (d_type == DT_BLK)     ? "block dev" :
                                                         (d_type == DT_CHR)     ? "char dev" :
                                                                                  "???");
            printf("%-10d%-8d", d->d_reclen, (int)d->d_off, d->d_name);
            switch (d_type)
            {
            case DT_REG:
                printf("\033[0;32m%-8s\033[0m\n", d->d_name);
                break;
            case DT_DIR:
                printf("\033[0;34m%-8s\033[0m\n", d->d_name);
                break;
            case DT_LNK:
                printf("\033[0;36m%-8s\033[0m\n", d->d_name);
                break;
            case DT_BLK:
            case DT_CHR:
                printf("\033[0;33m%-8s\033[0m\n", d->d_name);
                break;
            default:
                printf("\033[0;31m%-8s\033[0m\n", d->d_name);
                break;
            }
            bpos += d->d_reclen;
        }
    }
}

int main(int argc, char **argv)
{
    // dev(1, O_RDWR);
    // dup(0);
    // dup(0);
    // for (int i = 0; i < argc; ++i)
    // {
    //     printf("%s ", argv[i]);
    // }
    // printf("\n");
    // 如果没有参数
    if (argc < 2)
    {
        ls(".");
    }
    else
    {
        for (int i = 1; i < argc; i++)
            ls(argv[i]);
    }

    return 0;
}
