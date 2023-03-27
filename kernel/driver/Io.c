#include <type.h>
#include <driver.h>

void putString(char *ss)
{
    char *s = (char *)ss;
    while (*s)
    {
        // if (*s == '\n')
        // {
        //     SBIputchar('\r');
        // }
        SBIputchar(*s++);
    }
    return;
}

void printk(char *fmt, ...)
{
    
}
