#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <riscv.h>

// debug 开关
// #define CNX_DEBUG_

// #define QS_DEBUG_

#ifdef CNX_DEBUG_
#define CNX_DEBUG(...)                                                     \
    do {                                                                   \
        printk("[CNX] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                               \
    } while (0)
#else
#define CNX_DEBUG(...)
#endif

#ifdef QS_DEBUG_
#define QS_DEBUG(...)                                                     \
    do {                                                                  \
        printk("[QS] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                              \
    } while (0)
#else
#define QS_DEBUG(...)
#endif

#ifdef CHL_DEBUG_
#define CHL_DEBUG(...)                                                                              \
    do {                                                                                            \
        printk("[CHL_DEBUG] hartId %d at %s: %d in %s(): ", getTp(), __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                                                        \
    } while (0)
#else
#define CHL_DEBUG(...)
#endif

#endif