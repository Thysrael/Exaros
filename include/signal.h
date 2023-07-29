/**
 * @file signal.h
 * @brief 信号机制
 * @date 2023-07-21
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _SIGNAL_H
#define _SIGNAL_H

// 为了防止循环依赖，不能 include thread.h
#include "types.h"
#include "queue.h"
// #include "process.h"

/* from linux */
#define _NSIG 64

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPOLL SIGIO
#define SIGPWR 30
#define SIGSYS 31
#define SIGUNUSED 31

/* These should not be considered constants from userland.  */
#define SIGRTMIN 32
#define SIGRTMAX _NSIG
/* from linux end */

#define SIG_BLOCK 0   /* for blocking signals */
#define SIG_UNBLOCK 1 /* for unblocking signals */
#define SIG_SETMASK 2 /* for setting the signal mask */

#define SIGNAL_COUNT _NSIG
#define SIGNAL_CONTEXT_COUNT (4096)

typedef struct Thread Thread;
typedef struct Trapframe Trapframe;

typedef struct SignalInfo
{
    int signo;            /* signal number */
    int errno;            /* errno value */
    int code;             /* signal code */
    int trapno;           /* trap that caused hardware signal (unusued on most architectures) */
    u32 si_pid;           /* sending PID */
    u32 si_uid;           /* real UID of sending program */
    int si_status;        /* exit value or signal */
    u32 si_utime;         /* user time consumed */
    u32 si_stime;         /* system time consumed */
    u32 si_value;         /* signal value */
    int si_int;           /* POSIX.1b signal */
    void *si_ptr;         /* POSIX.1b signal */
    int si_overrun;       /* count of timer overrun */
    int si_timerid;       /* timer ID */
    void *si_addr;        /* memory location that generated fault */
    long si_band;         /* band event */
    int si_fd;            /* file descriptor */
    short si_addr_lsb;    /* LSB of address */
    void *si_lower;       /* lower bound when address vioation occured */
    void *si_upper;       /* upper bound when address violation occured */
    int si_pkey;          /* protection key on PTE causing faut */
    void *si_call_addr;   /* address of system call instruction */
    int si_syscall;       /* number of attempted syscall */
    unsigned int si_arch; /* arch of attempted syscall */
} SignalInfo;

// sig set 形态需要保持一致handler
// 参阅 linux

typedef struct SignalSet
{
    u64 signal;
} SignalSet;

typedef void (*__sighandler_t)(int);

// sig action 形态需要保持一致
// 参阅 linux
typedef struct SignalAction
{
    __sighandler_t sa_handler;
    unsigned long sa_flags;
    SignalSet sa_mask;
} SignalAction;

typedef struct SignalContext
{
    Trapframe contextRecover;
    SignalSet blockedRecover;
    u8 signal;
    LIST_ENTRY(link, SignalContext)
    link;
} SignalContext;

typedef LIST_HEAD(SignalContextList, SignalContext) SignalContextList;

SignalContext *getFirstPendingSignal(Thread *thread);
void signalContextFree(SignalContext *sc);
void signalInit();
void handleSignal();
int kill(int pid, int sig);
int tgkill(int tgid, int tid, int sig);
int tkill(int tid, int sig);
int rt_sigaction(int sig, u64 act, u64 oldAction);
int rt_sigprocmask(int how, SignalSet *set, SignalSet *oldset, int sigsetsize);
int rt_sigtimedwait(SignalSet *which, SignalInfo *info, TimeSpec *ts);
void rt_sigreturn();

int signalSetEmpty(SignalSet *set);
int signalSetFill(SignalSet *set);
int signalSetAdd(SignalSet *set, int signal);
int signalSetDel(SignalSet *set, int signal);
int signalSetOr(SignalSet *set, SignalSet *set1);
bool signalSetIsMember(SignalSet *set, int signal);
#endif