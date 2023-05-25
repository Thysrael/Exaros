/**
 * @file main.c
 * @brief 用户程序启动第一个 C 部分
 * _start(.S) -> __start_main(.c) -> main(.c)
 * @date 2023-05-25
 *
 * @copyright Copyright (c) 2023
 */

#include <unistd.h>

extern int main();

int __start_main(long *p)
{
    // int argc = p[0];
    // char **argv = (void *)(p + 1);
    // exit(main(argc, argv));
    exit(main());
    return 0;
}

// extern int userMain();

// void libMain(int argc, char **argv)
// {
//     int ret = userMain(argc, argv);
//     exit(ret);
// }