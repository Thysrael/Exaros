#ifndef _RISCV_H_
#define _RISCV_H_

#include <types.h>
#include <driver.h>
#include <trap.h>

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

static inline u64
readSstatus()
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

static inline void readTp(u64 x)
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

#endif