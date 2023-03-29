#include <mem_layout.h>
#include <riscv.h>

    .section .entry
    .globl _start
_start:
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
    .space KERNEL_STACK_SIZE * CORE_NUM
    .globl kernelStackTop
kernelStackTop:
    
    .section .pgdir
    .align 12
    .globl kernelPageDirectory
kernelPageDirectory:
    .space PAGE_SIZE

