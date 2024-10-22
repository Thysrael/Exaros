/**
 * @file driver.h
 * @brief UART, SD 驱动
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <types.h>
#include <sbi.h>

/**
 * @brief 打印字符
 *
 * @param c 待打印字符
 */
void putchar(char c);
// {
//     register u64 a0 asm("a0") = (u64)c;
//     register u64 a7 asm("a7") = (u64)SBI_CONSOLE_PUTCHAR;
//     asm volatile("ecall"
//                  : "+r"(a0)
//                  : "r"(a7)
//                  : "memory");
// };

/**
 * @brief 从控制台获取字符
 *
 * @return int 获取到的字符
 */
int getchar();

/**
 * @brief 使用 OpenSBI 向控制台输出字符
 *
 * @param c
 */
inline void putcharSbi(char c)
{
    register u64 a0 asm("a0") = (u64)c;
    register u64 a7 asm("a7") = (u64)SBI_CONSOLE_PUTCHAR;
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a7)
                 : "memory");
};

/**
 * @brief 使用 OpenSBI 从控制台获取字符
 *
 * @return int
 */
inline int getcharSbi()
{
    register u64 a7 asm("a7") = (u64)SBI_CONSOLE_GETCHAR;
    register u64 a0 asm("a0");
    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a7)
                 : "memory");
    return a0;
};

void printk(const char *fmt, ...);
void _panic_(const char *, int, const char *, const char *, ...) __attribute__((noreturn));
void _assert_(const char *, const char *, int, const char *, u64);
#define panic(...) _panic_(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define assert(x) _assert_(#x, __FILE__, __LINE__, __func__, (x))
#define panic_on(expr)                           \
    do {                                         \
        int r = (expr);                          \
        if (r != 0)                              \
        {                                        \
            panic("'" #expr "' returned %d", r); \
        }                                        \
    } while (0)

void consoleInit();
#endif /* _DRIVER_H_ */
