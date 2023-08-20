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
char *argvTime[] = {"./time-test", 0};
char *argvLibcBench[] = {"./libc-bench", 0};
char *tmp[] = {"./busybox", "sh", "tmp.sh", 0};
char *ababa[] = {"./busybox", "./iozone", "-a", "-r", "1k", "-s", "4m", 0};

char *argvAuto[] = {"/busybox", "sh", "auto.sh", 0};
char *argvAuto2[] = {"/busybox", "sh", "auto2.sh", 0};
char *argvCp1[] = {"/copy-file-range-test-1", 0};
char *argvCp2[] = {"/copy-file-range-test-2", 0};
char *argvCp3[] = {"/copy-file-range-test-3", 0};

char *argvInterruptsTest1[] = {"./interrupts-test-1", 0};
char *argvInterruptsTest2[] = {"./interrupts-test-2", 0};
void main()
{
    dev(1, O_RDWR); // stdin
    dup(0);         // stdout
    dup(0);         // stderr
    printf("busybox test begin.\n");
    int pid;

    pid = fork();
    if (pid == 0)
    {
        exec("./time-test", argvTime);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvAuto2);
    }
    else
    {
        wait(0);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./interrupts-test-1", argvInterruptsTest1);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }
    pid = fork();
    if (pid == 0)
    {
        exec("./interrupts-test-2", argvInterruptsTest2);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("/copy-file-range-test-2", argvCp1);
        // exec("./time-test", timet);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }
    pid = fork();
    if (pid == 0)
    {
        exec("/copy-file-range-test-2", argvCp2);
        // exec("./time-test", timet);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }
    pid = fork();
    if (pid == 0)
    {
        exec("/copy-file-range-test-3", argvCp3);
        // exec("./time-test", timet);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvAuto);
        // exec("./time-test", timet);
    }
    else
    {
        wait(0);
        // exec("./time-test", timet);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvBusybox);
    }
    else
    {
        wait(0);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvLua);
    }
    else
    {
        wait(0);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvLibc);
    }
    else
    {
        wait(0);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./libc-bench", argvLibcBench);
    }
    else
    {
        wait(0);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvCyclic);
    }
    else
    {
        wait(0);
    }

    pid = fork();
    if (pid == 0)
    {
        exec("./busybox", argvIOZone);
    }
    else
    {
        wait(0);
    }

    exit(0);
}
