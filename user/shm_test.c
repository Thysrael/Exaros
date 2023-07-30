#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

// static int fd[2];

// void init()
// {
//     if (pipe(fd) < 0)
//     {
//         printf("pipe error");
//     }
// }

// void wait_pipe()
// {
//     char c;
//     if (read(fd[0], &c, 1) < 0)
//     {
//         printf("wait pipe error");
//     }
// }

// void notify_pipe()
// {
//     char c = 'c';
//     if (write(fd[1], &c, 1) != 1)
//     {
//         printf("notify pipe error");
//     }
// }

// void destory_pipe()
// {
//     close(fd[0]);
//     close(fd[1]);
// }

int select(long long a5);

int main(int argc, char **argv)
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    int shmid;
    shmid = shmget(0, 1024, 0);
    printf("shmid is %d\n", shmid);
    int *pi = (int *)shmat(shmid, 0, 0);
    int pid = fork();
    if (pid == 0)
    {
        printf("this is child\n");

        *pi = 1123;
        select(0);
    }
    else
    {
        printf("this is father\n");
        // int *pi = (int *)shmat(shmid, 0, 0);
        while (*pi != 1123)
        {
            printf("father: waiting child\n");
            select(0);
        }
        printf("AC\n");
    }
    // init(); // 初始化管道
    // int pid = fork();
    // if (pid == 0)
    // {
    //     // 子进程
    //     wait_pipe();
    //     // 子进程去从共享内存中读取数据
    //     // 子进程进行共享内存的映射
    //     int *pi = (int *)shmat(shmid, 0, 0);
    //     if (pi == (int *)(-1))
    //     {
    //         printf("shmat error");
    //         exit(1);
    //     }
    //     printf("start: %d, end: %d\n", *pi, *(pi + 1));
    //     // 读取完毕后解除映射
    //     // shmdt(pi);
    //     // 删除共享内存
    //     // shmctl(shmid, IPC_RMID, NULL);
    //     destory_pipe();
    // }
    // else
    // {
    //     // 进行共享内存映射
    //     int *pi = (int *)shmat(shmid, 0, 0);
    //     if (pi == (int *)(-1))
    //     {
    //         printf("shmat error");
    //         exit(1);
    //     }
    //     // 往共享内存中写入数据（通过操作映射的地址即可）
    //     *pi = 100;
    //     *(pi + 1) = 200;
    //     // 操作完毕解除共享内存的映射
    //     // shmdt(pi);
    //     // 通知子进程读取数据
    //     notify_pipe();
    //     destory_pipe();
    //     wait(0);
    // }
    // exit(0);

    return 0;
}