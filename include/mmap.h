/**
 * @file mmap.h
 * @brief syscall mmap 所需的宏
 * @date 2023-05-25
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _MMAP_H_
#define _MMAP_H_

#include <types.h>

#define PROT_NONE_BIT 0x00
#define PROT_READ_BIT 0x01
#define PROT_WRITE_BIT 0x02
#define PROT_EXEC_BIT 0x04
#define PROT_NONE(prot) ((u64)prot & PROT_NONE_BIT)
#define PROT_READ(prot) ((u64)prot & PROT_READ_BIT)
#define PROT_WRITE(prot) ((u64)prot & PROT_WRITE_BIT)
#define PROT_EXEC(prot) ((u64)prot & PROT_EXEC_BIT)

// #define MAP_FILE_BIT 0x00
#define MAP_SHARED_BIT 0x01
#define MAP_PRIVATE_BIT 0X02
#define MAP_ANONYMOUS_BIT 0x20
// #define MAP_FILE(flags) ((u64)flags & MAP_FILE_BIT)
#define MAP_SHARED(flags) ((u64)flags & MAP_SHARED_BIT)
#define MAP_PRIVATE(flags) ((u64)flags & MAP_PRIVATE_BIT)
#define MAP_ANONYMOUS(flags) ((u64)flags & MAP_ANONYMOUS_BIT)
// #define MAP_FAILED ((void *)-1)

#endif