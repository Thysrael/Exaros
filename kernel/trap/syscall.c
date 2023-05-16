#include <syscall.h>
#include <trap.h>
#include <driver.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR] syscallPutchar,
    [SYSCALL_GET_PROCESS_ID] syscallGetProcessId,
    [SYSCALL_SCHED_YIELD] syscallYield,
    [SYSCALL_PROCESS_DESTORY] syscallProcessDestory,
    [SYSCALL_CLONE] syscallClone,
    [SYSCALL_PUT_STRING] syscallPutString,
    [SYSCALL_GET_PID] syscallGetProcessId,
    [SYSCALL_GET_PARENT_PID] syscallGetParentProcessId,
    [SYSCALL_WAIT] syscallWait,
    [SYSCALL_DEV] syscallDevice,
    [SYSCALL_DUP] syscallDup,
    [SYSCALL_EXIT] syscallExit,
    [SYSCALL_PIPE2] syscallPipe,
    [SYSCALL_WRITE] syscallWrite,
    [SYSCALL_READ] syscallRead,
    [SYSCALL_CLOSE] syscallClose,
    [SYSCALL_OPENAT] syscallOpenAt,
    [SYSCALL_GET_CPU_TIMES] syscallGetCpuTimes,
    [SYSCALL_GET_TIME] syscallGetTime,
    [SYSCALL_SLEEP_TIME] syscallSleepTime,
    [SYSCALL_DUP3] syscallDupAndSet,
    [SYSCALL_CHDIR] syscallChangeDir,
    [SYSCALL_CWD] syscallGetWorkDir,
    [SYSCALL_MKDIRAT] syscallMakeDirAt,
    [SYSCALL_BRK] syscallBrk,
    [SYSCALL_SBRK] syscallSetBrk,
    [SYSCALL_FSTAT] syscallGetFileState,
    [SYSCALL_MAP_MEMORY] syscallMapMemory,
    [SYSCALL_UNMAP_MEMORY] syscallUnMapMemory,
    [SYSCALL_EXEC] syscallExec,
    [SYSCALL_GET_DIRENT] syscallGetDirent,
    [SYSCALL_MOUNT] syscallMount,
    [SYSCALL_UMOUNT] syscallUmount,
    [SYSCALL_LINKAT] syscallLinkAt,
    [SYSCALL_UNLINKAT] syscallUnlinkAt,
    [SYSCALL_UNAME] syscallUname};

void syscallPutchar()
{
    putchar(getHartTrapFrame()->a0);
};