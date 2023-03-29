#include <types.h>
#include <sbi.h>

inline void putchar(char c)
{
    register u64 a0 asm("a0") = (u64)c;
    register u64 a7 asm("a7") = (u64)SBI_CONSOLE_PUTCHAR;
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a7)
                 : "memory");
};

inline int getchar()
{
    register u64 a7 asm("a7") = (u64)SBI_CONSOLE_GETCHAR;
    register u64 a0 asm("a0");
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a7)
                 : "memory");
    return a0;
}

void printk(const char *fmt, ...);
void _panic_(const char *, int, const char *, const char *, ...) __attribute__((noreturn));
void _assert_(const char *, int, const char *, u64);
#define panic(...) _panic_(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define assert(x) _assert_(__FILE__, __LINE__, __func__, (x))
