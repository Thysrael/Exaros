/**
 * @file mem_layout.h
 * @brief 内存布局常量
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _MEMORY_LAYOUT_H_
#define _MEMORY_LAYOUT_H_

/*  内核虚拟空间
    VA_MAX ---------------->+---------------------------+-------0x7f ffff ffff
                            |       TRAMPOLINE:R-X      |       BY2PG
    TRAMPOLINE ------------>+---------------------------+-----------------
                            |                           |
    PHYSICAL_MEMORY_END --->+---------------------------+-------0x8800 0000
                            |       Free memory:RW-     |
    kernelEnd ------------->+---------------------------+-----------------
                            |       Kernel data:RW-     |
    textEnd --------------->+---------------------------+-----------------
                            |       Kernel text:R-X     |
    BASE_ADDRESS, --------->+---------------------------+-------0x8020 0000
    kernelStart,            |                           |
    textStart, -/           |       OpenSBI:R-X         |
                            |                           |
    PHYSICAL_MEMORY_BASE -->+---------------------------+-------0x8000 0000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       VIRTIO:RW-          |
    VIRTIO ---------------->+---------------------------+-------0x1000 1000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       UART0:RW-           |
    UART0 ----------------->+---------------------------+-------0x1000 0000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       PLIC:RW-            |
    PLIC  ----------------->+---------------------------+-------0x0c00 0000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       CLINT:RW-           |
    CLINT ----------------->+---------------------------+-------0x0200 0000
                            |       invalid memory      |
    0 --------------------->+---------------------------+-------0x0000 0000
*/

#define PHYSICAL_MEMORY_BASE ((u64)0x80000000)
#define PHYSICAL_MEMORY_SIZE ((u64)0x08000000)
#define PHYSICAL_MEMORY_END (PHYSICAL_MEMORY_BASE + PHYSICAL_MEMORY_SIZE)

/**
 * xv6 和 参考代码采取 38 位 va 以避免符号扩展
 * 这列直接使用 39 位，遇到问题再进行修改
 */
#define VA_WIDTH (39)
#define VA_MAX ((u64)(1) << VA_WIDTH)

#define KERNEL_STACK_SIZE 0x10000 // 16 pages

/* Page is different from Physical Page*/
#define PAGE_SHIFT (12)
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_NUM (PHYSICAL_MEMORY_SIZE >> PAGE_SHIFT)

#define CLINT ((u64)0x02000000)
#define PLIC ((u64)0x0c000000)
#define UART0 ((u64)0x10000000)
#define VIRTIO ((u64)0x10001000)

#define VIRT_OFFSET 0x3F00000000L
#define PLIC_V (PLIC + VIRT_OFFSET)

#define TRAMPOLINE (VA_MAX - PAGE_SIZE)
#define TRAPFRAME (TRAMPOLINE - PAGE_SIZE)

// qemu puts programmable interrupt controller here.
#define PLIC_PRIORITY (PLIC_V + 0x0)
#define PLIC_PENDING (PLIC_V + 0x1000)
#define PLIC_MENABLE(hart) (PLIC_V + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC_V + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC_V + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC_V + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC_V + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC_V + 0x201004 + (hart)*0x2000)

#endif