#include <driver.h>
#include <memory.h>
#include <trap.h>
#include <file.h>
#include <mem_layout.h>
#include <types.h>

#define UART_REG_TXDATA_FULL(data) (data & 0x80000000)  // 1 << 31
#define UART_REG_RXDATA_EMPTY(data) (data & 0x80000000) // 1 << 31

#define __io_br() \
    do {          \
    } while (0)
#define __io_ar() __asm__ __volatile__("fence i,r" \
                                       :           \
                                       :           \
                                       : "memory");
#define __io_bw() __asm__ __volatile__("fence w,o" \
                                       :           \
                                       :           \
                                       : "memory");
#define __io_aw() \
    do {          \
    } while (0)

static inline u32 __raw_readl(volatile void *addr)
{
    u32 val;
    asm volatile("lw %0, 0(%1)"
                 : "=r"(val)
                 : "r"(addr));
    return val;
}

static inline void __raw_writel(u32 val, volatile void *addr)
{
    asm volatile("sw %0, 0(%1)"
                 :
                 : "r"(val), "r"(addr));
}

#define readl(c) ({ u32 __v; __io_br(); __v = __raw_readl(c); __io_ar(); __v; })
#define writel(v, c) ({ __io_bw(); __raw_writel((v),(c)); __io_aw(); })

#ifdef VIRT
inline void putchar(char ch)
{
    putcharSbi(ch);
}
#else
inline void putchar(char ch)
{
    while (UART_REG_TXDATA_FULL(readl((volatile u32 *)UART_REG_TXDATA)))
        ;
    writel(ch, (volatile u32 *)UART_REG_TXDATA);
}
#endif

#ifdef VIRT
inline int getchar()
{
    return getcharSbi();
}
#else
inline int getchar()
{
    u32 ret = readl((volatile u32 *)UART_REG_RXDATA);
    if (UART_REG_RXDATA_EMPTY(ret)) { return -1; }
    return ret & 0xFF; // 1 byte
}
#endif

int consoleWrite(int isUser, u64 src, u64 start, u64 n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        char c;
        if (either_copyin(&c, isUser, src + i, 1) == -1)
            break;
        if (c == '\n')
            putchar('\r');
        putchar(c);
    }
    return i;
}

int consoleRead(int isUser, u64 dst, u64 start, u64 n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        char c = getchar();
        if (c == (char)-1 && i == 0)
        {
            getHartTrapFrame()->epc -= 4;
            yield();
        }
        if (c == (char)-1)
            return i;
        if (c == '\n')
            putchar('\r');
        if (either_copyout(isUser, dst + i, &c, 1) == -1)
            break;
    }
    return i;
}

void consoleInit()
{
    extern struct devsw devsw[];
    devsw[DEV_CONSOLE].read = consoleRead;
    devsw[DEV_CONSOLE].write = consoleWrite;
}