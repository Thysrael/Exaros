#include <driver.h>
#include <memory.h>
#include <trap.h>
#include <file.h>

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
