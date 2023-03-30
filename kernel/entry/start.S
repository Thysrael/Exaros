# 对于主核和副核分别进行启动，主核启动包括清空 BSS

#include <mem_layout.h>
#include <riscv.h>

    .section .entry
    .globl _start
_start:
    # t0 is the offset of hart stack
    mv t0, a0
    add t0, t0, 1
    slli t0, t0, 16
    la sp, kernelStack
    add sp, sp, t0
    call main

loop:
    j loop

    .section .stack
    .globl kernelStack
    .align 12
kernelStack:
    # every hart has a stack
    .space KERNEL_STACK_SIZE * CORE_NUM
    .globl kernelStackTop
kernelStackTop:
    
    .section .pgdir
    .align 12
    .globl kernelPageDirectory
kernelPageDirectory:
    .space PAGE_SIZE

