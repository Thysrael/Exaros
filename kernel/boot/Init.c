#include <driver.h>
#include <type.h>

void main(u64 hartid, u64 dtb)
{
    // putString("Hello World!\n");
    for (int i = 'a'; i < 'z'; i++)
        SBIputchar(i);
    SBIputchar('b');
    // putString("Hello World1222!\n");
    printf("Hello World!\n");
    while (1)
        ;
}