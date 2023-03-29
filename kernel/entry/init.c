#include <types.h>
#include <driver.h>

static inline void clearBSS()
{
    extern u64 bssStart[];
    extern u64 bssEnd[];
    for (u64 *i = bssStart; i < bssEnd; i++)
    {
        *i = 0;
    }
}

void main(u64 hartId)
{
    clearBSS();
    printk("hello, world, %d\n", hartId);
}