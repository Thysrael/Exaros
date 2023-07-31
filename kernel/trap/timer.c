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
#include <thread.h>

static u32 ticks;
IntervalTimer timer;

void setNextTimeout()
{
    SBI_CALL_1(SBI_SET_TIMER, readRealTime() + TIMER_INTERVAL);
}

void setNextTimeoutInterval(u64 time)
{
    SBI_CALL_1(SBI_SET_TIMER, time);
}

void timerTick()
{
    ticks++;
    // printk("timer tick: %d\n", ticks);
    setNextTimeout();
}

void setTimer(IntervalTimer time)
{
    timer = time;
    u64 interval = time.expiration.second * 1000000 + time.expiration.microSecond;
    interval /= 20;
    u64 realtime = readRealTime();
    setNextTimeoutInterval(realtime + interval);
    Thread *t = myThread();
    t->setAlarm = interval + realtime;
}

IntervalTimer getTimer()
{
    return timer;
}

#endif