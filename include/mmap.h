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

/* 0x01 - 0x03 are defined in linux/mman.h */
#define MAP_SHARED_BIT 0x01          /* Share changes */
#define MAP_PRIVATE_BIT 0X02         /* Changes are private */
#define MAP_SHARED_VALIDATE_BIT 0x03 /* share + validate extension flags */

#define MAP_TYPE_BIT 0x0f      /* Mask for type of mapping */
#define MAP_FIXED_BIT 0x10     /* Interpret addr exactly */
#define MAP_ANONYMOUS_BIT 0x20 /* don't use a file */

/* 0x0100 - 0x4000 flags are defined in asm-generic/mman.h */
#define MAP_POPULATE_BIT 0x008000        /* populate (prefault) pagetables */
#define MAP_NONBLOCK_BIT 0x010000        /* do not block on IO */
#define MAP_STACK_BIT 0x020000           /* give out an address that is best suited for process/thread stacks */
#define MAP_HUGETLB_BIT 0x040000         /* create a huge page mapping */
#define MAP_SYNC_BIT 0x080000            /* perform synchronous page faults for the mapping */
#define MAP_FIXED_NOREPLACE_BIT 0x100000 /* MAP_FIXED which doesn't unmap underlying mapping */
#define MAP_UNINITIALIZED_BIT 0x4000000  /* For anonymous mmap, memory could be uninitialized */

// #define MAP_FILE(flags) ((u64)flags & MAP_FILE_BIT)
#define MAP_SHARED(flags) ((u64)flags & MAP_SHARED_BIT)
#define MAP_PRIVATE(flags) ((u64)flags & MAP_PRIVATE_BIT)
#define MAP_ANONYMOUS(flags) ((u64)flags & MAP_ANONYMOUS_BIT)
// #define MAP_FAILED ((void *)-1)

#endif