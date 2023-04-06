#ifndef _TRAP_H_
#define _TRAP_H_

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

void trapInit();
void kernelTrap();
void kernelHandler();

#endif