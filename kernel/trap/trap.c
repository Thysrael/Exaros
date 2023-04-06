#include <driver.h>
#include <riscv.h>

void trapInit()
{
    printk("Trap init start...");

    // 1. 设置 stvec 寄存器到中断处理函数
    writeStvec((u64)kernelTrap);

    // 2. 设置 sie 寄存器，使能中断
    writeSie(readSie() | SIE_SSIE | SIE_STIE | SIE_SSIE);

    // 3. 设置 sip（当前待处理的中断） 为 0
    writeSip(0);

    // 4. 设置 sstatus 的 SIE, SPIE
    writeSstatus(readSstatus() | SSTATUS_SIE | SSTATUS_SPIE);
}

void kernelHandler()
{
    u64 scause = readScause();
    u64 stval = readStval();
    u64 sepc = readSepc();
    u64 sip = readSip();

    printk("[kernelHandler] scause: %x, stval: %x, sepc: %x, sip: %x", scause, stval, sepc, sip);

    // cnx 觉得内核态的异常处理用不到
    panic("kernel trap happened!!");
}

void userHandler()
{
    u64 scause = readScause();
    u64 stval = readStval();
    u64 sepc = readSepc();
    u64 sip = readSip();

    printk("[userHandler] scause: %x, stval: %x, sepc: %x, sip: %x", scause, stval, sepc, sip);

    // 判断中断或者异常，然后调用对应的处理函数
    u64 exceptionCode = scause & SCAUSE_EXCEPTION_CODE;

    if (scause & SCAUSE_INTERRUPT)
    {
        // 处理中断
        switch (exceptionCode)
        {
        case INTERRUPT_UEI:
            // user external interrupt
            /* code */
            break;
        case INTERRUPT_USI:
            // user software interrupt
            /* code */
            break;
        case INTERRUPT_UTI:
            // user timer interrupt
            /* code */
            break;
        }
    }
    else
    {
        // 处理异常
        switch (exceptionCode)
        {
        case EXCEPTION_ECALL:
            printk("ecall\n");
            /* code */
            break;
        case EXCEPTION_LOAD_FAULT:
            /* code */
            break;
        case EXCEPTION_STORE_FAULT:
            /* code */
            break;
        default:
            panic("unknown interrupt\n");
        }
    }
}