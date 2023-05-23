#include <driver.h>
#include <yield.h>
#include <mem_layout.h>
#include <memory.h>
#include <string.h>
#include <process.h>
#include <trap.h>
#include <types.h>
#include <virtio.h>
#include <syscall.h>

Trapframe *getHartTrapFrame()
{
    return (Trapframe *)(TRAPFRAME + getTp() * sizeof(Trapframe));
}

/**
 * @brief 初始化异常处理
 * 1. 设置 stvec 寄存器到中断处理函数
 * 2. 设置 sie 寄存器，使能中断（s级时钟中断，s级外部中断，s级软件中断）
 * 3. 设置 sip（当前待处理的中断） 为 0
 * 4. 设置 sstatus 的 SIE, SPIE, 使能中断
 */
void trapInit()
{
    printk("Trap init start...\n");

    // 1. 设置 stvec 寄存器到中断处理函数
    writeStvec((u64)kernelTrap);

    // 2. 设置 sie 寄存器，使能中断
    writeSie(readSie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

    // 3. 设置 sip（当前待处理的中断） 为 0
    writeSip(0);

    // 4. 设置 sstatus 的 SIE, SPIE
    writeSstatus(readSstatus() | SSTATUS_SIE | SSTATUS_SPIE);

    // 初始化时钟（为了防止输出太多东西，可以暂时注释掉）
    // setNextTimeout();

    printk("Trap init end.\n");
}

/**
 * @brief 处理中断，返回中断类型；（EXTERNAL_TRAP，TIMER_INTERRUPT，UNKNOWN_DEVICE）
 * 这个函数处理的中断有：s级外部中断，s级时钟中断
 *
 * @return int
 */
int handleInterrupt()
{
    u64 scause = readScause();
    u64 exceptionCode = scause & SCAUSE_EXCEPTION_CODE;

    assert(scause & SCAUSE_INTERRUPT);
    // 处理中断
    switch (exceptionCode)
    {
    case INTERRUPT_SEI:;
        printk("INTERRUPT_SEI\n");
        // // todo
        // // user external interrupt
        // int irq = interruptServed();
        // if (irq == UART_IRQ)
        // {
        //     int c = getchar();
        //     if (c != -1)
        //     {
        //         // consoleInterrupt(c);
        //     }
        // }
        // else if (irq == DISK_IRQ)
        // {
        //     // todo
        // }
        // else if (irq)
        // {
        //     panic("unexpected interrupt irq = %d\n", irq);
        // }
        // if (irq)
        // {
        //     interruptCompleted(irq);
        // }

        // this is a supervisor external interrupt, via PLIC.

        // irq indicates which device interrupted.
        u32 irq = interruptServed();

        if (irq == UART_IRQ)
        {
            // uartintr();
        }
        else if (irq == DISK_IRQ)
        {
            printk("1111\n\n");
            virtioDiskIntrupt();
        }
        else if (irq)
        {
            printk("unexpected interrupt irq=%d\n", irq);
        }

        // the PLIC allows each device to raise at most one
        // interrupt at a time; tell the PLIC the device is
        // now allowed to interrupt again.
        if (irq)
            interruptCompleted(irq);
        printk("external interrupt");
        return EXTERNAL_TRAP;
        break;
    case INTERRUPT_STI: // s timer interrupt
        // user timer interrupt
        timerTick();
        return TIMER_INTERRUPT;
        break;
    default:
        return UNKNOWN_DEVICE;
        break;
    }
}

/**
 * @brief 内核 trap 处理，只处理中断
 *
 */
void kernelHandler()
{
    u64 scause = readScause();
    u64 stval = readStval();
    u64 sepc = readSepc();
    u64 sip = readSip();
    u64 sstatus = readSstatus();

    printk("[kernelHandler] scause: %lx, stval: %lx, sepc: %lx, sip: %lx\n", scause, stval, sepc, sip);

    // Trapframe *trapframe = getHartTrapFrame();

    // 打印 trapFrame
    // printTrapframe(trapframe);

    // spp：进入异常的状态，0：用户态，1：内核态
    // 如果从用户态进入异常，则 panic
    if (!(sstatus & SSTATUS_SPP))
    {
        panic("kernel trap not from supervisor mode");
    }

    // 如果中断被打开，则 panic
    if (intr_get())
    {
        panic("kernel trap while interrupts enbled");
    }
    // intr_on();

    int device = handleInterrupt();
    switch (device)
    {
    case UNKNOWN_DEVICE:
        // u64 *pte;
        // int pa = pageLookup(myProcess()->pgdir, r_stval(), &pte);
        // panic("unhandled error %d,  %lx, %lx\n", scause, r_stval(), pa);
        panic("kernel trap");
        break;
    case TIMER_INTERRUPT:
        yield();
        break;
    default:
        break;
    }

    writeSepc(sepc);
    writeSstatus(sstatus);
}

extern Process *currentProcess[];
/**
 * @brief 用户态的异常处理函数，处理用户态的异常和中断
 * 处理了中断，系统调用，缺页异常
 *
 */
void userHandler()
{
    u64 scause = readScause();
    u64 stval = readStval();
    u64 sepc = readSepc();
    u64 sip = readSip();
    Trapframe *tf = getHartTrapFrame();
    u64 hartId = getTp();
    u64 *pte = NULL;

    writeStvec((u64)kernelTrap);
    if (0)
    {
        printk("[userHandler] scause: %lx, stval: %lx, sepc: %lx, sip: %lx\n", scause, stval, sepc, sip);
    }
    // 判断中断或者异常，然后调用对应的处理函数
    u64 exceptionCode = scause & SCAUSE_EXCEPTION_CODE;
    if (scause & SCAUSE_INTERRUPT)
    {
        handleInterrupt();
        yield();
    }
    else
    {
        // 处理异常
        switch (exceptionCode)
        {
        case EXCEPTION_ECALL:
            // printk("ecall\n");
            tf->epc += 4;
            syscallVector[tf->a7]();
            break;
        case EXCEPTION_LOAD_FAULT:
        case EXCEPTION_STORE_FAULT:;
            Page *page = pageLookup(currentProcess[hartId]->pgdir, stval, &pte);
            if (page == NULL)
            {
                passiveAlloc(currentProcess[hartId]->pgdir, stval);
            }
            else if (*pte & PTE_COW_BIT)
            {
                cowHandler(currentProcess[hartId]->pgdir, stval);
            }
            else
            {
                panic("unknown page fault.\n");
            }
            break;
        default:
            panic("unknown interrupt\n");
        }
    }
    userTrapReturn();
}

extern Process *currentpageFaultProcess[CORE_NUM];

/**
 * @brief 从 userHandler 返回到用户态
 * 这里和进程关系比较大，写完进程再改
 *
 */
void userTrapReturn()
{
    extern char trampoline[];

    int hartId = getTp();

    // stvec 是中断处理的入口地址
    writeStvec(TRAMPOLINE + ((u64)userTrap - (u64)trampoline));

    Trapframe *trapframe = getHartTrapFrame();

    trapframe->kernelSp = getProcessTopSp(myProcess());
    trapframe->trapHandler = (u64)userHandler;
    trapframe->kernelHartId = hartId;

    u64 sstatus = readSstatus();

    // spp 位置零，spie 位置 1 （spp 表示进入异常的前一个状态的特权级别，0:u, 1:s
    sstatus &= ~SSTATUS_SPP;
    sstatus |= SSTATUS_SPIE;

    writeSstatus(sstatus);

    u64 satp = MAKE_SATP(SV39, PA2PPN((myProcess()->pgdir)));

    u64 fn = TRAMPOLINE + ((u64)userReturn - (u64)trampoline);

    ((void (*)(u64, u64))fn)((u64)trapframe, satp);
}

/**
 * @brief 打印 Trapframe 中的信息
 *
 * @param tf
 */
void printTrapframe(Trapframe *tf)
{
    printk(" a0: %lx\n \
    a1: %lx\n \
    a2: %lx\n \
    a3: %lx\n \
    a4: %lx\n \
    a5: %lx\n \
    a6: %lx\n \
    a7: %lx\n \
    t0: %lx\n \
    t1: %lx\n \
    t2: %lx\n \
    t3: %lx\n \
    t4: %lx\n \
    t5: %lx\n \
    t6: %lx\n \
    s0: %lx\n \
    s1: %lx\n \
    s2: %lx\n \
    s3: %lx\n \
    s4: %lx\n \
    s5: %lx\n \
    s6: %lx\n \
    s7: %lx\n \
    s8: %lx\n \
    s9: %lx\n \
    s10: %lx\n \
    s11: %lx\n \
    ra: %lx\n \
    sp: %lx\n \
    gp: %lx\n \
    tp: %lx\n \
    epc: %lx\n \
    kernelSp: %lx\n \
    kernelSatp: %lx\n \
    trapHandler: %lx\n \
    kernelHartId: %lx\n",
           tf->a0, tf->a1, tf->a2, tf->a3, tf->a4,
           tf->a5, tf->a6, tf->a7, tf->t0, tf->t1,
           tf->t2, tf->t3, tf->t4, tf->t5, tf->t6,
           tf->s0, tf->s1, tf->s2, tf->s3, tf->s4,
           tf->s5, tf->s6, tf->s7, tf->s8, tf->s9,
           tf->s10, tf->s11, tf->ra, tf->sp, tf->gp,
           tf->tp, tf->epc, tf->kernelSp, tf->kernelSatp,
           tf->trapHandler, tf->kernelHartId);
}