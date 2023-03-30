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

#endif