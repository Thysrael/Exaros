/**
 * @file types.h
 * @brief 拓展 C 语言类型系统
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _TYPES_H_
#define _TYPES_H_

typedef unsigned __attribute__((__mode__(QI))) u8;
typedef unsigned __attribute__((__mode__(HI))) u16;
typedef unsigned __attribute__((__mode__(SI))) u32;
typedef unsigned __attribute__((__mode__(DI))) u64;
typedef int __attribute__((__mode__(QI))) i8;
typedef int __attribute__((__mode__(HI))) i16;
typedef int __attribute__((__mode__(SI))) i32;
typedef int __attribute__((__mode__(DI))) i64;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef u32 uint32;
typedef unsigned short wchar; // wide char 16 bit

typedef u8 bool;

#define true 1
#define false 0
#define NULL 0

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define MIN(_a, _b)             \
    ({                          \
        typeof(_a) __a = (_a);  \
        typeof(_b) __b = (_b);  \
        __a <= __b ? __a : __b; \
    })

#define MAX(_a, _b)             \
    ({                          \
        typeof(_a) __a = (_a);  \
        typeof(_b) __b = (_b);  \
        __a >= __b ? __a : __b; \
    })

#define ROUND(a, n) (((((u64)(a)) + (n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((u64)(a)) & ~((n)-1))

/**
 * @brief 用于计算 64 位整数 x 的最低非零位（即从右往左数第一个非零位）
 *
 * @param x 一个 64 位数
 * @return int 第一个非零位
 */
inline int LOW_BIT64(u64 x)
{
    int res = 0;
    if ((x & ((1UL << 32) - 1)) == 0)
    {
        x >>= 32;
        res += 32;
    }
    if ((x & ((1UL << 16) - 1)) == 0)
    {
        x >>= 16;
        res += 16;
    }
    if ((x & ((1UL << 8) - 1)) == 0)
    {
        x >>= 8;
        res += 8;
    }
    if ((x & ((1UL << 4) - 1)) == 0)
    {
        x >>= 4;
        res += 4;
    }
    if ((x & ((1UL << 2) - 1)) == 0)
    {
        x >>= 2;
        res += 2;
    }
    if ((x & ((1UL << 1) - 1)) == 0)
    {
        x >>= 1;
        res += 1;
    }
    return x ? res : -1;
}

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#define LOWBIT(x) ((x) & (-x))

#endif