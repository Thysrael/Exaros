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

typedef struct SignalSet
{
    u64 signal;
} SignalSet;

typedef struct SignalAction
{
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    unsigned mask[2];
} SignalAction;

typedef unsigned long gregset_t[32];

struct __riscv_f_ext_state
{
    unsigned int f[32];
    unsigned int fcsr;
};

struct __riscv_d_ext_state
{
    unsigned long long f[32];
    unsigned int fcsr;
};

struct __riscv_q_ext_state
{
    unsigned long long f[64] __attribute__((aligned(16)));
    unsigned int fcsr;
    unsigned int reserved[3];
};

union __riscv_fp_state
{
    struct __riscv_f_ext_state f;
    struct __riscv_d_ext_state d;
    struct __riscv_q_ext_state q;
};
typedef union __riscv_fp_state fpregset_t;

typedef struct sigcontext
{
    gregset_t gregs;
    fpregset_t fpregs;
} mcontext;

typedef struct sigaltstack
{
    void *ss_sp;
    int ss_flags;
    u64 ss_size;
} sigaltstack;

typedef struct ucontext
{
    unsigned long uc_flags;
    struct ucontext *uc_link;
    struct sigaltstack uc_stack;
    SignalSet uc_sigmask;
    mcontext uc_mcontext;
} ucontext;

typedef struct SignalContext
{
    Trapframe contextRecover;
    bool start;
    u8 signal;
    ucontext *uContext;
    LIST_ENTRY(link, SignalContext)
    link;
} SignalContext;

typedef LIST_HEAD(SignalContextList, SignalContext) SignalContextList;

void signalInit();
void handleSignal(Thread *thread);
int kill(int pid, int sig);
int tgkill(int tgid, int tid, int sig);
int tkill(int tid, int sig);
int rt_sigaction(int sig, u64 act, u64 oldAction);
int rt_sigprocmask(int how, SignalSet *set, SignalSet *oldset, int sigsetsize);
void sigreturn();
void signalContextFree(SignalContext *sc);

int signalEmptySet(SignalSet *set);

int signalFillset(SignalSet *set);

int signalAddSet(SignalSet *set, int signal);

int signalDelSet(SignalSet *set, int signal);

bool signalIsMember(SignalSet *set, int signal);

SignalContext *getFirstSignalContext(Thread *thread);
#endif