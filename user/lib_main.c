#include "./lib/lib_main.h"
#include "./lib/syscall.h"
#include "./lib/print.h"

void libMain(int argc, char **argv)
{
    userMain(argc, argv);
    // exit(ret);
}