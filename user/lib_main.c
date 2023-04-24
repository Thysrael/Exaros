#include "./lib/lib_main.h"
#include "./lib/syscall.h"
#include "./lib/syscall_lib.h"
#include "./lib/print.h"

void libMain(int argc, char **argv)
{
    int ret = userMain(argc, argv);
    exit(ret);
}