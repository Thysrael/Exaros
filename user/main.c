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