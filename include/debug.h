#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <riscv.h>

// debug 开关
// #define CNX_DEBUG_
// #define QS_DEBUG_
// #define CHL_DEBUG_
// #define LOAD_DEBUG_
// #define SYSCALL_DEBUG_
// #define NET_DEBUG_
// #define SOCKET_DEBUG_
// #define SHM_DEBUG_
#define TMPF_DEBUG_

#ifdef CNX_DEBUG_
#define CNX_DEBUG(...)       \
    do {                     \
        printk(__VA_ARGS__); \
    } while (0)
#else
#define CNX_DEBUG(...)
#endif

#ifdef SYSCALL_DEBUG_
#define SYSCALL_DEBUG(...)   \
    do {                     \
        printk(__VA_ARGS__); \
    } while (0)
#else
#define SYSCALL_DEBUG(...)
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

#ifdef LOAD_DEBUG_
#define LOAD_DEBUG(...)                                                     \
    do {                                                                    \
        printk("[LOAD] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                                \
    } while (0)
#else
#define LOAD_DEBUG(...)
#endif

#ifdef NET_DEBUG_
#define NET_DEBUG(...)                                                     \
    do {                                                                   \
        printk("[NET] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                               \
    } while (0)
#else
#define NET_DEBUG(...)
#endif

#ifdef SOCKET_DEBUG_
#define SOCKET_DEBUG(...)                                                     \
    do {                                                                      \
        printk("[SOCKET] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                                  \
    } while (0)
#else
#define SOCKET_DEBUG(...)
#endif

#ifdef SHM_DEBUG_
#define SHM_DEBUG(...)                                                     \
    do {                                                                   \
        printk("[SHM] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                               \
    } while (0)
#else
#define SHM_DEBUG(...)
#endif

#ifdef TMPF_DEBUG_
#define TMPF_DEBUG(...)                                                     \
    do {                                                                    \
        printk("[TMPF] at %s: %d in %s(): ", __FILE__, __LINE__, __func__); \
        printk(__VA_ARGS__);                                                \
    } while (0)
#else
#define TMPF_DEBUG(...)
#endif

#endif