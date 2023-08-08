#include "unistd.h"
#include "stdio.h"

char *argvBusybox[] = {"./busybox", "sh", "busybox_testcode.sh", 0};
char *argvLua[] = {"./busybox", "sh", "lua_testcode.sh", 0};
char *argvStatic[] = {"./busybox", "sh", "run-static.sh", 0};
char *argvNetperf[] = {"./busybox", "sh", "netperf_testcode.sh", 0};
char *argvIperf[] = {"./busybox", "sh", "iperf_testcode.sh", 0};
char *argvLibc[] = {"./busybox", "sh", "libctest_testcode.sh", 0};
char *argvIOZone[] = {"./busybox", "sh", "iozone_testcode.sh", 0};
char *argvDynamic[] = {"./busybox", "sh", "run-dynamic.sh", 0};
char *argvLmbanch[] = {"./busybox", "sh", "lmbench_testcode.sh", 0};
char *argvCyclic[] = {"./busybox", "sh", "cyclictest_testcode.sh", 0};
char *argvUnix[] = {"./busybox", "sh", "unixbench_testcode.sh", 0};
char *shell[] = {"./busybox", "sh", 0};
char *timet[] = {"./time-test"};
char *libc_bench[] = {"./libc-bench", 0};
char *tmp[] = {"./busybox", "sh", "tmp.sh", 0};
char *ababa[] = {"./busybox", "./iozone", "-a", "-r", "1k", "-s", "4m", 0};

char *argvAuto[] = {"/busybox", "sh", "auto.sh", 0};

void main()
{
    dev(1, O_RDWR); // stdin
    dup(0);         // stdout
    dup(0);         // stderr
    printf("busybox test begin.\n");
    int pid = fork();

    if (pid == 0)
    {
        exec("./busybox", argvLmbanch);
        // exec("./time-test", timet);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }

    exit(0);
}
