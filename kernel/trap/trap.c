#include <driver.h>
#include <yield.h>
#include <mem_layout.h>
#include <memory.h>
#include <string.h>
#include <debug.h>
#include <process.h>
#include <trap.h>
#include <types.h>
#include <virtio.h>
#include <syscall.h>
#include <thread.h>
#include <signal.h>

char interruptsString[512];
int interruptRecoder[20];

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

extern struct ThreadList usedThreads;
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
    Thread *thread = NULL;

    assert(scause & SCAUSE_INTERRUPT);
    // printk("%lx\n", exceptionCode);
    // 处理中断
    interruptRecoder[exceptionCode] += 1;
    switch (exceptionCode)
    {
    case INTERRUPT_SEI:;
        // printk("INTERRUPT_SEI\n");
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
        // printk("external interrupt");
        return EXTERNAL_TRAP;
        break;
    case INTERRUPT_STI:; // s timer interrupt
        setNextTimeoutInterval(readRealTime() + 0xffffffff);
        int flag = myThread()->setAlarm;
        LIST_FOREACH(thread, &usedThreads, link)
        {
            u64 realtime = readRealTime();
            // printk("th: %lx, alarm %d\n", thread->threadId, thread->setAlarm);
            if (thread->setAlarm && thread->setAlarm <= realtime)
            {
                tkill(thread->threadId, SIGALRM);
                thread->setAlarm = 0;
                // printk("sendsigto: %lx\n", thread->threadId);
            }
        }
        if (!flag)
        {
            myProcess()->utime++;
            timeYield();
        }
        // timerTick();
        // user timer interrupt
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
    u64 sepc = readSepc();
    u64 sstatus = readSstatus();

    // printk("[kernelHandler] scause: %lx, stval: %lx, sepc: %lx, sip: %lx\n", readScause(), readStval(), readSepc(), readSip());
    // printk("[kernelHandler] scause: %lx, stval: %lx, sepc: %lx, sip: %lx\n", readScause(), readStval(), readSepc(), readSip());

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
        myProcess()->ktime++;
        // yield();
        timeYield();
        break;
    default:
        break;
    }

    writeSepc(sepc);
    writeSstatus(sstatus);
}

/**
 * @brief 用户态的异常处理函数，处理用户态的异常和中断
 * 处理了中断，系统调用，缺页异常
 *
 */
void userHandler()
{
    u64 scause = readScause();
    u64 stval = readStval();
    Trapframe *tf = getHartTrapFrame();
    u64 *pte = NULL;

    writeStvec((u64)kernelTrap);
    // if (scause != 8)
    // printk("[userHandler] scause: %lx, stval: %lx, sepc: %lx, sip: %lx, sp: %lx, threadid: %lx\n", scause, stval, readSepc(), readSip(), tf->sp, myThread()->threadId);
    // 判断中断或者异常，然后调用对应的处理函数

    // if (scause == 0xd)
    // {
    //     printk("ddd t0: %lx, a1: %lx\n", tf->t0, tf->a1);
    //     if (tf->a1 == 0x152c30)
    //     {
    //         printTrapframe(tf);
    //     }
    // }
    u64 exceptionCode = scause & SCAUSE_EXCEPTION_CODE;
    if (scause & SCAUSE_INTERRUPT)
    {
        handleInterrupt();
        myProcess()->utime++;
        // yield();
        timeYield();
    }
    else
    {
        // 处理异常
        switch (exceptionCode)
        {
        case EXCEPTION_ECALL:
            // printk("ecall\n");
            tf->epc += 4;
            // if (tf->a7 != 63 && tf->a7 != 64)
            // if (tf->a7 != 63)
            //     printk("ecall: %d, epc: %lx, tid:%lx, code: %lx\n", tf->a7, tf->epc, myThread()->threadId, *((u64 *)va2PA(myProcess()->pgdir, tf->epc + 4, 0)));

            if (syscallVector[tf->a7] == 0)
            {
                // printk("ecall unrealized: %d\n", tf->a7);
            }
            else
            {
                // intr_on();
                syscallVector[tf->a7]();
            }
            break;
        case EXCEPTION_LOAD_FAULT:
        case EXCEPTION_STORE_FAULT:;
            Page *page = pageLookup(myProcess()->pgdir, stval, &pte);
            if (page == NULL)
            {
                passiveAlloc(myProcess()->pgdir, stval);
            }
            else if (*pte & PTE_COW_BIT)
            {
                cowHandler(myProcess()->pgdir, stval);
            }
            else
            {
                // printk("*pte: 0x%lx\n", *pte);
                // printk("tf->a6: 0x%lx\n", tf->a6);
                // printk("[userHandler] scause: %lx, stval: %lx, sepc: %lx, sip: %lx, sp: %lx\n", scause, stval, readSepc(), readSip(), tf->sp);
                // int cow;
                // printk("pa2VA(pa): 0x%lx\n", va2PA(myProcess()->pgdir, stval, &cow));
                panic("unknown page fault.\n");
            }
            break;
        default:
            CNX_DEBUG("unknown interrupt: %lx\n", exceptionCode);

            passiveAlloc(myProcess()->pgdir, stval);
            break;
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
    trapframe->kernelSp = getThreadTopSp(myThread());
    trapframe->trapHandler = (u64)userHandler;
    trapframe->kernelHartId = hartId;
    handleSignal(myThread());

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
    printk(" a0: %lx \
    a1: %lx \
    a2: %lx \
    a3: %lx \
    a4: %lx \
    a5: %lx \
    a6: %lx \
    a7: %lx \
    t0: %lx \
    t1: %lx \
    t2: %lx \
    t3: %lx \
    t4: %lx \
    t5: %lx \
    t6: %lx \
    s0: %lx \
    s1: %lx \
    s2: %lx \
    s3: %lx \
    s4: %lx \
    s5: %lx \
    s6: %lx \
    s7: %lx \
    s8: %lx \
    s9: %lx \
    s10: %lx \
    s11: %lx \
    ra: %lx \
    sp: %lx \
    gp: %lx \
    tp: %lx \
    epc: %lx \
    kernelSp: %lx \
    kernelSatp: %lx \
    trapHandler: %lx \
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

void plicinit(void)
{
    // set desired IRQ priorities non-zero (otherwise disabled).
    *(u32 *)(PLIC_V + 10 * 4) = 1;
    *(u32 *)(PLIC_V + 1 * 4) = 1;
}

void plicinithart(void)
{
    int hart = readTp();

    // set enable bits for this hart's S-mode
    // for the uart and virtio disk.
    *(u32 *)PLIC_SENABLE(hart) = (1 << 10) | (1 << 1);

    // set this hart's S-mode priority threshold to 0.
    *(u32 *)PLIC_SPRIORITY(hart) = 0;
}

int printNum(int i, char *buf)
{
    int length = 0;
    int tmpi = i;
    if (i == 0)
    {
        buf[0] = '0';
        return 1;
    }
    while (tmpi)
    {
        tmpi /= 10;
        length++;
    }
    tmpi = i;
    buf[length] = '\0';

    int l = length;
    while (tmpi)
    {
        buf[l - 1] = tmpi % 10 + '0';
        l--;
        tmpi /= 10;
    }
    return length;
}

int updateInterruptsString()
{
    char *buf = interruptsString;
    for (int i = 0; i < 20; i++)
    {
        if (interruptRecoder[i] || i == 5 || i == 8)
        {
            int len = printNum(i, buf);
            buf[len] = ':';
            buf[len + 1] = ' ';
            buf += (len + 2);
            len = printNum(interruptRecoder[i], buf);
            buf[len] = '\n';
            buf += (len + 1);
        }
    }

    buf[0] = '\0';
    // printk("interrupt string is\n %s\n", interruptsString);
    return buf - interruptsString + 1;
}