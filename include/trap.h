/**
 * @file trap.h
 * @brief 异常和中断的编号，以及处理函数
 * @date 2023-04-11
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _TRAP_H_
#define _TRAP_H_

#include <process.h>
#include <types.h>
#include <riscv.h>
#include <mem_layout.h>
#include <arch.h>

#define SCAUSE_INTERRUPT (1UL << 63)
#define SCAUSE_EXCEPTION_CODE ((1UL << 63) - 1)

// 中断编码
#define INTERRUPT_USI 0 // User software interrupt
#define INTERRUPT_SSI 1 // Supervisor software interrupt
// 2-3 reserved
#define INTERRUPT_UTI 4 // User timer interrupt
#define INTERRUPT_STI 5 // Supervisor timer interrupt
// 6-7 reserved
#define INTERRUPT_UEI 8 // User external interrupt
#define INTERRUPT_SEI 9 // Supervisor external interrupt

// 异常编码
// #define EXCEPTION_ 0         // Instruction address misaligned
// #define EXCEPTION_    1      // Instruction access fault
// #define EXCEPTION_    2      // Illegal instruction
// #define EXCEPTION_    3      // Breakpoint
// #define EXCEPTION_    4      // Load address misaligned
// #define EXCEPTION_    5      // Load access fault
// #define EXCEPTION_    6      // Store address misaligned
// #define EXCEPTION_    7      // Store access fault
#define EXCEPTION_ECALL 8 // Environment call from U mode
// #define EXCEPTION_    9      // Environment call from S mode
// #define EXCEPTION_    11     // Environment call from M mode
// #define EXCEPTION_    12     // Instruction page fault
#define EXCEPTION_LOAD_FAULT 13  // Load page fault
#define EXCEPTION_STORE_FAULT 15 // Store page fault

#define SIE_SSIE (1L << INTERRUPT_SSI) // Supervisor software interrupt
#define SIE_STIE (1L << INTERRUPT_STI) // Supervisor software interrupt
#define SIE_SEIE (1L << INTERRUPT_SEI) // Supervisor software interrupt

#define TIMER_INTERRUPT 2
#define EXTERNAL_TRAP 1
#define UNKNOWN_DEVICE 0

#if defined VIRT || defined QEMU
#define UART_IRQ 10
#define DISK_IRQ 1
#else
#define UART_IRQ 33
#define DISK_IRQ 27
#endif

inline static u32 interruptServed()
{
    u64 hart = readTp(); // thread id
#if defined VIRT || defined QEMU
    return *(u32 *)PLIC_SCLAIM(hart);
#else
    return *(u32 *)PLIC_MCLAIM(hart);
#endif
}
inline static void interruptCompleted(int irq)
{
    int hart = readTp();
#if defined VIRT || defined QEMU
    *(u32 *)PLIC_SCLAIM(hart) = irq;
#else
    *(u32 *)PLIC_MCLAIM(hart) = irq;
#endif
}

void trapInit();
int handleInterrupt();
void kernelHandler();
void kernelTrap();
void userHandler();
void userTrap();
void userReturn();
void userTrapReturn();
void printTrapframe(Trapframe *tf);

// about timer
#define TIMER_INTERVAL 100000

typedef struct IntervalTimer
{
    TimeSpec interval;
    TimeSpec expiration;
} IntervalTimer;

/*

VA_TOP     ----------------------
 4096           异常处理的代码
TRAMPOLINE ----------------------
 4096           对应每个核的 trapframe
TRAPFRAME  ----------------------
*/

/**
 * @brief 获取当前核的 trapframe 地址
 *
 * @return Trapframe*
 */
Trapframe *getHartTrapFrame();

void setNextTimeout();
void setNextTimeoutInterval(u64 interaval);
void timerTick();
void setTimer(IntervalTimer new);
IntervalTimer getTimer();
void plicinit(void);
void plicinithart(void);

int updateInterruptsString();
#endif