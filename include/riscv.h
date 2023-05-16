/**
 * @file riscv.h
 * @brief riscv platform 上的常量、对于寄存器读写的 getter，setter
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _RISCV_H_
#define _RISCV_H_

#include <types.h>
#include <driver.h>

#define CORE_NUM 2

#define XLEN 64

// sstatus 寄存器
#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

// 读写系统寄存器

#define READ_CSR(dst, src) ({    \
    asm volatile("csrr %0, " src \
                 : "=r"(dst));   \
})

#define WRITE_CSR(dst, src) ({      \
    asm volatile("csrw " dst ", %0" \
                 :                  \
                 : "r"(src));       \
})

static inline u64 readRealTime()
{
    u64 x;
    // asm volatile("csrr %0, time" : "=r" (x) );
    // this instruction will trap in SBI
    asm volatile("rdtime %0"
                 : "=r"(x));
    return x;
}

static inline u64 readSstatus()
{
    u64 x;
    READ_CSR(x, "sstatus");
    return x;
}

static inline void writeSstatus(u64 x)
{
    WRITE_CSR("sstatus", x);
}

static inline u64 readSip()
{
    u64 x;
    READ_CSR(x, "sip");
    return x;
}

static inline void writeSip(u64 x)
{
    WRITE_CSR("sip", x);
}

static inline u64 readSie()
{
    u64 x;
    READ_CSR(x, "sie");
    return x;
}

static inline void writeSie(u64 x)
{
    WRITE_CSR("sie", x);
}

static inline u64 readStvec()
{
    u64 x;
    READ_CSR(x, "stvec");
    return x;
}

static inline void writeStvec(u64 x)
{
    WRITE_CSR("stvec", x);
}

static inline u64 readScause()
{
    u64 x;
    READ_CSR(x, "scause");
    return x;
}

static inline void writeScause(u64 x)
{
    WRITE_CSR("scause", x);
}

// read/write Sepc
static inline u64 readSepc()
{
    u64 x;
    READ_CSR(x, "sepc");
    return x;
}

static inline void writeSepc(u64 x)
{
    WRITE_CSR("sepc", x);
}

// read/write Sscratch
static inline u64 readSscratch()
{
    u64 x;
    READ_CSR(x, "sscratch");
    return x;
}

static inline void writeSscratch(u64 x)
{
    WRITE_CSR("sscratch", x);
}

// read/write Stval
static inline u64 readStval()
{
    u64 x;
    READ_CSR(x, "stval");
    return x;
}

static inline void writeStval(u64 x)
{
    WRITE_CSR("stval", x);
}

static inline u64 readTp()
{
    u64 x;
    asm volatile("mv %0, tp"
                 : "=r"(x));
    return x;
}

/**
 * @brief are devide interrupts enabled?
 *
 * @return true
 * @return false
 */
static inline bool intr_get()
{
    u64 x = readSstatus();
    // not enabled
    return (x & SSTATUS_SIE) != 0;
}

#define CORE_NUM 2

/**
 * @brief set the TP(Thread Pointer) using HART ID
 *
 * @param hartId 硬件多线程编号
 */
inline void setTp(u64 hartId)
{
    asm volatile("mv tp, %0"
                 :
                 : "r"(hartId & 0x7));
}

/**
 * @brief Get the hartID
 *
 * @return u64 hartID
 */
inline u64 getTp()
{
    u64 hartId;
    asm volatile("mv %0, tp"
                 : "=r"(hartId));
    return hartId;
}

/**
 * @brief Refresh all TLB
 *
 */
inline void sfenceVma()
{
    asm volatile("sfence.vma");
}

/**
 * @brief Write x to STAP
 *
 * @param x
 */
inline void writeSatp(u64 x)
{
    /**
     * The CSRRW (Atomic Read/Write CSR) instruction atomically
     * swaps values in the CSRs and integer registers.
     * CSRRW reads the old value of the CSR,
     * zero-extends the value to XLEN bits,
     * then writes it to integer register rd.
     * The initial value in rs1 is written to the CSR.
     * If rd=x0, then the instruction shall not read the CSR
     * and shall not cause any of the side effects
     * that might occur on a CSR read.
     *
     * csrw csr, rs = csrrw x0, csr, rs
     */
    asm volatile("csrw satp, %0"
                 :
                 : "r"(x));
}

// enable device interrupts
static inline void intr_on()
{
    writeSstatus(readSstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void intr_off()
{
    writeSstatus(readSstatus() & ~SSTATUS_SIE);
}

#define SV39 (8)
#define MAKE_SATP(mode, ppn) (((u64)mode << 60) | (u64)ppn)

#endif