#include <delay.h>
#include <riscv.h>

void usdelay(u64 interval)
{
    u64 cur = readRealTime();
#define TIMER_CLOCK 1000000LL // fu740 CPU Timer, Freq = 1000000Hz
    u64 end = cur + interval * (TIMER_CLOCK / 1000000);

    while (end >= cur)
    {
        cur = readRealTime();
    }
}
