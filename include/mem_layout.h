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
    VA_MAX ---------------->+---------------------------+----------------- 0x40_0000_0000 (1<<38)
                            |       TRAMPOLINE:R-X      |       BY2PG
    TRAMPOLINE ------------>+---------------------------+-----------------
                            |       TRAPFRAME:RW-       |       BY2PG
    TRAPFRAME ------------->+---------------------------+----------------- 0x3f_ffff_e000
                            |           ...             |
    PLIC  ----------------->+---------------------------+----------------- 0x3F_0c20_9000
      |                     |       PLIC:RW-            |
    PLIC  ----------------->+---------------------------+----------------- 0x3F_0c00_0000
                            |            ...            |
    CLINT ----------------->+---------------------------+----------------- 0x3F_0201_0000
      |                     |       CLINT:RW-           |
    CLINT ----------------->+---------------------------+----------------- 0x3F_0200_0000
                            |            ...            |
    KERNEL_PROCESS_SP_TOP ->+---------------------------+----------------- 0x10_0000_0000
                            |    用户进程的内核栈        |
    KERNEL_PROCESS_SP_BOT ->+---------------------------+----------------- 0x0F_FD80_0000
                            |            ...            |
    ------------------------+---------------------------+----------------- 0x0F_C800_0000
                            |          BIT_MAP          |
    FILE_SYSTEM_CLUSTER_BITMAP_BASE --------------------+----------------- 0x0F_C000_0000 (KERNEL_PROCESS_SP_TOP - 1 << 30)
                            |       signalAction        |
    KERNEL_PROCESS_SIGNAL_BASE -------------------------+----------------- 0x0F_8000_0000 (FILE_SYSTEM_CLUSTER_BITMAP_BASE - 1 << 30)
                            |      socket buffer        |
    SOCKET_BUFFER_BASE ---> +---------------------------+----------------- 0x0F_4000_0000 (KERNEL_PROCESS_SIGNAL_BASE - 1 << 30)
                            |      Shared Memory        |
    SHM_BASE -------------> +---------------------------+----------------- 0x0F_0000_0000 (SOCKET_BUFFER_BASE - 1 << 30)
                            |                           |
                            |            ...            |
    PHYSICAL_MEMORY_END --->+---------------------------+----------------- 0x8800_0000
                            |       Free memory:RW-     |
                            |  初始化的时候插入 freelist  |
    kernelEnd ------------->+---------------------------+-----------------
                            |       Kernel data:RW-     |
    textEnd --------------->+---------------------------+-----------------
                            |       Kernel text:R-X     |
    BASE_ADDRESS, --------->+---------------------------+----------------- 0x8020_0000
    kernelStart,            |                           |
    textStart, -/           |       OpenSBI:R-X         |
                            |                           |
    PHYSICAL_MEMORY_BASE -->+---------------------------+----------------- 0x8000_0000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       VIRTIO:RW-          |
    VIRTIO ---------------->+---------------------------+----------------- 0x1000_1000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       UART0:RW-           |
    UART0 ----------------->+---------------------------+----------------- 0x1000_0000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       invalid memory      |
    0 --------------------->+---------------------------+----------------- 0x0000_0000
*/

/*  进程虚拟空间
    VA_MAX ---------------->+---------------------------+----0x40_0000_0000
                            |       TRAMPOLINE:R-X      |       BY2PG
    TRAMPOLINE ------------>+---------------------------+----0x3F_FFFF_F000
                            |       TRAPFRAME:RW-       |       BY2PG
    TRAMPOLINE ------------>+---------------------------+----0x3F_FFFF_E000
                            |       TRAPFRAME:RW-       |       BY2PG
    SIGNAL_TRAMPOLINE------>+---------------------------+----0x3F_FFFF_D000
    USER_STACK_TOP          |                           |        ^
                            | User Stack(dynamic, down) |        |
                            |                           |     1 << 32
    Stack Pointer --------->+---------------------------+        |
                            |                           |        v
    USER_STACK_BOTTOM, ---->+---------------------------+----0x3E_FFFF_D000
    USER_MMAP_HEAP_TOP      |                           |        ^
    mmapHeapPointer ------->+---------------------------+        |
                            |                           |     1 << 32
                            |User MMAP Heap(dynamic, up)|        |
                            |                           |        v
    USER_MMAP_HEAP_BOTTOM ->+---------------------------+----0x3D_FFFF_D000
    USER_BRK_HEAP_TOP       |                           |        ^
    brkHeapPointer -------->+---------------------------+        |
                            |                           |     1 << 32
                            |User BRK Heap(dynamic, up) |        |
                            |                           |        v
    USER_BRK_HEAP_BOTTOM -->+---------------------------+----0x3C_FFFF_D000
    USER_SHM_HEAP_BOTTOM    |                           |        ^
                            |                           |        |
    shmHeapPointer -------->+---------------------------+        |
                            |                           |     1 << 32
                            | User SHM Heap(dynamic,up) |        |
                            |                           |        v
    USER_SHM_HEAP_BOTTOM -->+---------------------------+----0x3B_FFFF_D000
                            |                           |
    ----------------------->+---------------------------+-----------------
                            |       .data               |
                            |       .bss                |
                            |       .text               |
    0 --------------------->+---------------------------+-------0x0000_0000
*/

// 用于分配给用户程序的内存
#define PHYSICAL_MEMORY_BASE (0x80000000ULL)
#define PHYSICAL_MEMORY_SIZE (0x20000000ULL)
#define PHYSICAL_MEMORY_END (PHYSICAL_MEMORY_BASE + PHYSICAL_MEMORY_SIZE)

/**
 * xv6 和 参考代码采取 38 位 va 以避免符号扩展
 * 这列直接使用 39 位，遇到问题再进行修改
 * UPDATE: 遇到符号扩展问题，修改为 38 位
 */
#define VA_WIDTH (38)
#define VA_MAX ((1ULL) << VA_WIDTH)

// Core Local Interruptor, 用于处理与处理器核心相关的定时器和中断
#define CLINT (0x02000000ULL)
#define PLIC (0x0c000000ULL)
#define UART0 (0x10010000ULL)
// #define UART1 (0x10011000ULL)
#define SPI (0x10050000ULL)
// virtio mmio interface
#define VIRTIO (0x10001000ULL)
#define VIRT_OFFSET (0x3F00000000ULL) // 虚拟地址的偏移
#define PLIC_V (PLIC + VIRT_OFFSET)
#define CLINT_V (CLINT + VIRT_OFFSET)
#define VIRTIO_V (VIRTIO + VIRT_OFFSET)

// trampoline, signal trampoline, stack
#define TRAMPOLINE (VA_MAX - PAGE_SIZE)
#define TRAPFRAME (TRAMPOLINE - PAGE_SIZE)
#define SIGNAL_TRAMPOLINE (TRAPFRAME - PAGE_SIZE)
#define USER_STACK_TOP SIGNAL_TRAMPOLINE
#define USER_STACK_BOTTOM (USER_STACK_TOP - (1UL << 32))

// 用户进程的堆
#define USER_MMAP_HEAP_TOP USER_STACK_BOTTOM
#define USER_MMAP_HEAP_BOTTOM (USER_MMAP_HEAP_TOP - (1UL << 32))
#define USER_BRK_HEAP_TOP USER_MMAP_HEAP_BOTTOM
#define USER_BRK_HEAP_BOTTOM (USER_BRK_HEAP_TOP - (1UL << 32))
#define USER_SHM_HEAP_TOP USER_BRK_HEAP_BOTTOM
#define USER_SHM_HEAP_BOTTOM (USER_SHM_HEAP_TOP - (1UL << 32))

// 用户的信号返回地址
// #define SIGNAL_TRAMPOLINE (USER_BRK_HEAP_BOTTOM - PAGE_SIZE)

// qemu puts programmable interrupt controller here.
#define PLIC_PRIORITY (PLIC_V + 0x0)
#define PLIC_PENDING (PLIC_V + 0x1000)
#define PLIC_MENABLE(hart) (PLIC_V + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC_V + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC_V + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC_V + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC_V + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC_V + 0x201004 + (hart)*0x2000)

// UART
#define UART_REG_TXDATA (UART0 + 0x00)
#define UART_REG_RXDATA (UART0 + 0x04)
#define UART_REG_TXCTRL (UART0 + 0x08)
#define UART_REG_RXCTRL (UART0 + 0x0C)

// SPI
#define SPI_REG_SCKDIV (SPI + 0x00)
#define SPI_REG_SCKMODE (SPI + 0x04)
#define SPI_REG_CSID (SPI + 0x10)
#define SPI_REG_CSDEF (SPI + 0x14)
#define SPI_REG_CSMODE (SPI + 0x18)

#define SPI_REG_FMT (SPI + 0x40)
#define SPI_REG_TXFIFO (SPI + 0x48)
#define SPI_REG_RXFIFO (SPI + 0x4c)

#define KERNEL_STACK_SIZE (0x10000ULL) // 16 pages

#define KERNEL_PROCESS_SP_TOP (1UL << 36)

#define PAGE_SHIFT (12)
#define PAGE_SIZE (0x1000ULL)
#define PAGE_NUM (PHYSICAL_MEMORY_SIZE >> PAGE_SHIFT)

#define FILE_SYSTEM_CLUSTER_BITMAP_BASE (KERNEL_PROCESS_SP_TOP - (1UL << 30))
#define KERNEL_PROCESS_SIGNAL_BASE (FILE_SYSTEM_CLUSTER_BITMAP_BASE - (1UL << 30))
#define SOCKET_BUFFER_BASE (KERNEL_PROCESS_SIGNAL_BASE - (1UL << 30))
#define SHM_BASE (SOCKET_BUFFER_BASE - (1UL << 30))

// #define USER_HEAP_TOP
// #define USER_HEAP_BOTTOM
#endif