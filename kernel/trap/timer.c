/**
 * @file timer.c
 * @brief 关于时钟
 * @date 2023-04-11
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _TIMER_H_
#define _TIMER_H_

#include <riscv.h>
#include <trap.h>
#include <driver.h>

static u32 ticks;
IntervalTimer timer;

void setNextTimeout()
{
    SBI_CALL_1(SBI_SET_TIMER, readRealTime() + TIMER_INTERVAL);
}

void timerTick()
{
    ticks++;
    // printk("timer tick: %d\n", ticks);
    setNextTimeout();
}

void setTimer(IntervalTimer new)
{
    timer = new;
}

IntervalTimer getTimer()
{
    return timer;
}

#endif