#ifndef _HART_H_
#define _HART_H_

#include <riscv.h>
#include <process.h>
#include <mem_layout.h>
#include <types.h>

struct Hart
{
    int interruptLayer;      // depth of disable interrupt
    int lastInterruptEnable; // were interrupts enabled before disable interrupt
};

struct Trapframe;

struct Hart harts[CORE_NUM];
inline struct Hart *myHart()
{
    int r = getTp();
    return &harts[r];
}

#endif