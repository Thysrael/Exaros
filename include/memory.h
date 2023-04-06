/**
 * @file memory.h
 * @brief memory.c 的函数定义
 * @date 2023-03-31
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <mem_layout.h>
#include <driver.h>
#include <queue.h>
#include <types.h>

#define PTE_VALID_BIT ((u64)1 << 0)    /* 有效位 */
#define PTE_READ_BIT ((u64)1 << 1)     /* 可读位 */
#define PTE_WRITE_BIT ((u64)1 << 2)    /* 可写位 */
#define PTE_EXECUTE_BIT ((u64)1 << 3)  /* 可运行位 */
#define PTE_USER_BIT ((u64)1 << 4)     /* 可在 User-MODE 访问位 */
#define PTE_GLOBAL_BIT ((u64)1 << 5)   /* 全局访问位 */
#define PTE_ACCESSED_BIT ((u64)1 << 6) /* 被访问位（读/写/取） */
#define PTE_DIRTY_BIT ((u64)1 << 7)    /* 被修改位（写） */
#define PTE_PERM_WIDTH (10)

#define PTE_VALID(pte) ((u64)pte & PTE_VALID_BIT) /* 0: 无效 */

#define GETLOW(x, bits) ((u64)x & (((u64)1 << bits) - 1))
#define ALIGN_DOWN(x, size) (((u64)x) & ~((u64)size - 1))
#define ALIGN_UP(x, size) (ALIGN_DOWN(x, size) + 1)

#define PA2PPN(pa) ((u64)pa >> PAGE_SHIFT)
#define PPN2PA(ppn) ((u64)ppn << PAGE_SHIFT)
#define PA2PTE(pa) (PA2PPN(pa) << PTE_PERM_WIDTH)
#define PTE2PPN(pte) GETLOW((((u64)pte >> PTE_PERM_WIDTH)), 44)
#define PTE2PA(pte) PPN2PA(PTE2PPN(pte))
#define VAPPN(va, level) GETLOW((u64)va >> (PAGE_SHIFT + 9 * (level)), 9)

typedef LIST_HEAD(PageList, Page) PageList;
typedef LIST_ENTRY(PageListEntry, Page) PageListEntry;

typedef struct Page
{
    PageListEntry link;
    u32 ref;
    u32 hardId;
} Page;

/**
 * @brief page 2 Page iNdex
 *
 * @param page
 * @return u64
 */
__attribute__((unused)) static u64 page2PN(Page *page)
{
    extern Page pages[];
    return page - pages;
}

/**
 * @brief page 2 Page iNdex
 *
 * @param page
 * @return u64
 */
__attribute__((unused)) static Page *pn2Page(u64 pn)
{
    extern Page pages[];
    return pages + pn;
}

/**
 * @brief page 2 Physical Address
 *
 * @param page
 * @return * page
 */
__attribute__((unused)) static u64 page2PA(Page *page)
{
    return PHYSICAL_MEMORY_BASE + (page2PN(page) << PAGE_SHIFT);
}

/**
 * @brief page 2 Page Table Entry
 *
 * @param page
 * @return u64
 */
__attribute__((unused)) static u64 page2Pte(Page *page)
{
    return PA2PTE(page2PA(page));
}

/**
 * @brief physical address 2 Page iNdex
 *
 * @param page
 * @return u64
 */
__attribute__((unused)) static u64 pa2PN(u64 pa)
{
    if (pa < PHYSICAL_MEMORY_BASE)
    {
        panic("Panic: page pa is too low\n");
    }
    return (pa - PHYSICAL_MEMORY_BASE) >> PAGE_SHIFT;
}

/**
 * @brief physical address 2 Page
 *
 * @param page
 * @return u64
 */
__attribute__((unused)) static Page *pa2Page(u64 pa)
{
    return pn2Page(pa2PN(pa));
}

/**
 * @brief page table entry 2 Page
 *
 * @param pte
 * @return Page*
 */
__attribute__((unused)) static Page *pte2Page(u64 pte)
{
    return pa2Page(PTE2PA(pte));
}

void memoryInit();

#endif /* _MEMORY_H_ */