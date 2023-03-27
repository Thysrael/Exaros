#include <type.h>

typedef unsigned __attribute__((__mode__(DI))) u64;

void SBIputchar(char c)
{
    register u64 a0 asm("a0") = (u64)c;
    register u64 a7 asm("a7") = (u64)1;
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a7)
                 : "memory");
}