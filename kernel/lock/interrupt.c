#include <lock.h>
#include <riscv.h>
#include <hart.h>
#include <driver.h>

/**
 * @brief 禁用中断，如果 interruptLayer 是 0，将原来的 SIE 保存在 hart->lastInterruptEnable
 * interruptLayer++;
 *
 */
void interruptPush(void)
{
    int oldInterruptEnable = intr_get();

    // 禁用中断
    intr_off();

    struct Hart *hart = myHart();
    if (hart->interruptLayer == 0)
        hart->lastInterruptEnable = oldInterruptEnable;
    hart->interruptLayer++;
}

/**
 * @brief interruptLayer--;
 * 如果 interruptLayer == 0，并且 hart->lastInterruptEnable 允许中断，则打开中断
 *
 */
void interruptPop(void)
{
    struct Hart *hart = myHart();
    if (intr_get())
    {
        panic("Interrupt bit still have!\n");
    }

    if (hart->interruptLayer < 0)
    {
        panic("Interrupt close error! Not match!\n");
    }

    hart->interruptLayer--;
    if (hart->interruptLayer == 0 && hart->lastInterruptEnable)
        intr_on();
}