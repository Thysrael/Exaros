#include "unistd.h"
#include "stdio.h"

// char *argvBusybox[] = {"./busybox", "sh", "busybox_testcode.sh", 0};
// char *argvLua[] = {"./busybox", "sh", "lua_testcode.sh", 0};
// char *argvLmbanch[] = {"./busybox", "sh", "lmbench_testcode.sh", 0};
char *shell[] = {"./busybox", "sh", 0};

void main()
{
    dev(1, O_RDWR); // stdin
    dup(0);         // stdout
    dup(0);         // stderr
    printf("busybox test begin.\n");
    int pid = fork();

    if (pid == 0)
    {
        exec("./busybox", shell);
    }
    else
    {
        wait(0);
    }

    exit(0);
}
