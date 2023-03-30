/**
 * @file mem_layout.h
 * @brief 内存布局常量
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _MEMORY_LAYOUT_H_
#define _MEMORY_LAYOUT_H_

#define KERNEL_STACK_SIZE 0x10000 // 16 pages

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

#endif