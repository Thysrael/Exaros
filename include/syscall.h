#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYSCALL_GET_PROCESS_ID 1
#define SYSCALL_PROCESS_DESTORY 3
#define SYSCALL_PUTCHAR 4
#define SYSCALL_PUT_STRING 5

#define SYSCALL_SBRK 13 // TODO

#define SYSCALL_CWD 17
#define SYSCALL_DEV 20
#define SYSCALL_DUP 23
#define SYSCALL_DUP3 24
#define SYSCALL_fcntl 25
#define SYSCALL_IOCONTROL 29

#define SYSCALL_MKDIRAT 34
#define SYSCALL_UNLINKAT 35
#define SYSCALL_LINKAT 37
#define SYSCALL_UMOUNT 39
#define SYSCALL_MOUNT 40
#define SYS_statfs 43
#define SYSCALL_FTRUNCATE 46
#define SYSCALL_ACCESS 48

#define SYSCALL_CHDIR 49

#define SYSCALL_FCHMOD_AT 53
#define SYSCALL_OPEN 55
#define SYSCALL_OPENAT 56
#define SYSCALL_CLOSE 57
#define SYSCALL_PIPE2 59
#define SYSCALL_GET_DIRENT 61
#define SYSCALL_LSEEK 62
#define SYSCALL_READ 63
#define SYSCALL_WRITE 64
#define SYSCALL_READ_VECTOR 65
#define SYSCALL_WRITE_VECTOR 66
#define SYSCALL_PREAD 67
#define SYSCALL_PWRITE 68

#define SYSCALL_SEND_FILE 71
#define SYSCALL_SELECT 72
#define SYSCALL_POLL 73
#define SYS_readlinkat 78
#define SYSCALL_FSTATAT 79

#define SYSCALL_FSTAT 80
#define SYSCALL_SYNC 81
#define SYSCALL_UTIMENSAT 88
#define SYS_fsync 82

#define SYSCALL_EXIT 93
#define SYSCALL_EXIT_GROUP 94 // TODO
#define SYSCALL_SET_TID_ADDRESS 96
#define SYS_futex 98

#define SYSCALL_SLEEP_TIME 101

#define SYSCALL_SET_TIMER 103
#define SYSCALL_SET_TIME 112
#define SYSCALL_GET_TIME 113

#define SYS_syslog 116

#define SYSCALL_SCHED_YIELD 124
#define SYSCALL_KILL 129
#define SYSCALL_TKILL 130
#define SYSCALL_TGKILL 131
#define SYSCALL_SIGNAL_ACTION 134
#define SYSCALL_SIGNAL_PROCESS_MASK 135
#define SYS_rt_sigtimedwait 137
#define SYSCALL_SIGRETURN 139
#define SYS_getresuid 148
#define SYS_getresgid 150
#define SYS_times 153
#define SYS_setpgid 154
#define SYS_getpgid 155
#define SYS_getsid 156
#define SYS_setsid 157
#define SYSCALL_GET_CPU_TIMES 153
#define SYSCALL_UNAME 160
#define SYS_umask 166
#define SYSCALL_GET_TIME_OF_DAY 169
#define SYSCALL_GET_PID 172
#define SYSCALL_GET_PARENT_PID 173
#define SYSCALL_GET_USER_ID 174
#define SYSCALL_GET_EFFECTIVE_USER_ID 175
#define SYSCALL_GET_GROUP_ID 176
#define SYSCALL_GET_EFFECTIVE_GROUP_ID 177
#define SYSCALL_GET_THREAD_ID 178
#define SYS_sysinfo 179
#define SYSCALL_SHUTDOWN 210

#define SYSCALL_SHM_GET 194
#define SYSCALL_SHM_CTL 195
#define SYSCALL_SHM_AT 196
#define SYSCALL_SHM_DT 197

#define SYSCALL_SOCKET 198
#define SYSCALL_BIND 200
#define SYSCALL_LISTEN 201
#define SYSCALL_ACCEPT 202
#define SYSCALL_CONNECT 203
#define SYSCALL_GET_SOCKET_NAME 204
#define SYSCALL_SEND_TO 206
#define SYSCALL_RECEIVE_FROM 207
#define SYSCALL_SET_SOCKET_OPTION 208

#define SYSCALL_BRK 214

#define SYSCALL_UNMAP_MEMORY 215
#define SYSCALL_CLONE 220
#define SYSCALL_EXEC 221
#define SYSCALL_MAP_MEMORY 222
#define SYSCALL_MEMORY_PROTECT 226

#define SYS_mprotect 226
#define SYS_msync 227
#define SYS_mlock 228
#define SYS_madvise 233
#define SYSCALL_WAIT 260
#define SYS_prlimit64 261
#define SYS_renameat2 276

#define MAX_SYSCALL 350

void syscallGetProcessId();
void syscallSetTidAddress();
void syscallYield();
void syscallPutchar();
void syscallProcessDestory();
void syscallClone();
void syscallPutString();
void syscallGetParentProcessId();
void syscallWait();
void syscallExit();
void syscallGetCpuTimes();
void syscallGetTime();
void syscallSleepTime();
void syscallBrk();
void syscallSetBrk();
void syscallMapMemory();
void syscallUnMapMemory();
void syscallExec();
void syscallUname();
void syscallDup(void);
void syscallDupAndSet(void);
void syscallRead(void);
void syscallWrite(void);
void syscallClose(void);
void syscallGetFileState(void);
void syscallOpenAt(void);
void syscallMakeDirAt(void);
void syscallChangeDir(void);
void syscallGetWorkDir(void);
void syscallPipe(void);
void syscallDevice(void);
void syscallGetDirent(void);
void syscallMount(void);
void syscallUmount(void);
void syscallUnlinkAt(void);
void syscallLinkAt(void);
void syscallShutdown(void);
void syscallKill();
void syscallTkill();
void syscallTgkill();
void syscallSigreturn();

void syscallGetUserId();
void syscallGetGroupId();
void syscallGetFileStateAt(void);
void syscallSignalAction();
void syscallSignProccessMask();
void syscallIOControl();
void syscallReadVector();
void syscallWriteVector();
void syscallGetClockTime();
void syscallPoll();
void syscall_fcntl(void);
void syscallGetTheardId(void);
void syscallSetTime(void);
void syscallSetTimer(void);
void syscallGetTheardId(void);
void syscallSelect(void);
void syscallPRead(void);
void syscallPWrite();
void syscallSendFile(void);

void doNothing();
void syscallGetresuid();
void syscallGetresgid();
void syscallSetpgid();
void syscallGetpgid();
void syscallGetsid();
void syscallSetsid();
void syscallFutex();
void syscallSyslog();
void syscallUmask();
void syscallSysinfo();
void syscallLseek();
void syscallUtimensat();
void syscallAccess();
void syscallMemoryProtect();

#define SYSCALL_LSEEK 62
#define SYSCALL_UTIMENSAT 88
#define SYSCALL_ACCESS 48

#define AT_FDCWD -100

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void syscallRtSigtimedwait();
void syscallTimes();
void syscallMadvise();
void syscallMlock();
void syscallMprotect();
void syscallMsync();
void syscallReadlinkat();
// void syscallRenameat2();
void syscallStatfs();
void syscallFchmodat();
void syscallFsync();
void syscallPrlimit64();
void syscallSocket();
void syscallBind();
void syscallGetSocketName();
void syscallSetSocketOption();
void syscallSendTo();
void syscallReceiveFrom();
void syscallListen();
void syscallConnect();
void syscallAccept();

void syscallFchmodAt();
void syscallSync();
void syscallFtruncate();
void syscallSHMGet();
void syscallSHMCtrl();
void syscallSHMAt();
void syscallSHMDt();

extern void (*syscallVector[])(void);

int do_linkat(int oldDirFd, char *oldPath, int newDirFd, char *newPath);

#endif