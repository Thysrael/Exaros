/**
 * @file mem_layout.h
 * @brief 内存布局常量
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _MEMORY_LAYOUT_H_
#define _MEMORY_LAYOUT_H_

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
#define PLIC ((u64)0x0c000000)

#define TRAMPOLINE (VA_MAX - PAGE_SIZE)
#define TRAPFRAME (TRAMPOLINE - PAGE_SIZE)
#endif