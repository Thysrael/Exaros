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

#define SV39 (8)
#define MAKE_SATP(mode, ppn) (((u64)mode << 60) | (u64)ppn)

#endif