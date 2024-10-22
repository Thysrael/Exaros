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
#define PTE_ACCESSED_BIT ((u64)1 << 6) /* 被访问位（读/写/取），恒 1 */
#define PTE_DIRTY_BIT ((u64)1 << 7)    /* 被修改位（写），恒 1*/
#define PTE_COW_BIT ((u64)1 << 8)      /* Copy on Write 位 */
#define PTE_SHM_BIT ((u64)1 << 9)      /* share memory 位 */
#define PTE_PERM_WIDTH (10)
#define PTE2PT 512

#define PTE_VALID(pte) ((u64)pte & PTE_VALID_BIT) /* 0: 无效 */
#define PTE_USER(pte) ((u64)pte & PTE_USER_BIT)   /* 0: 无效 */
#define PTE_COW(pte) ((u64)pte & PTE_COW_BIT)     /* 0: 无效 */

#define GETLOW(x, bits) ((u64)x & (((u64)1 << bits) - 1))

#define ALIGN_DOWN(x, size) (((u64)(x)) & ~((u64)(size)-1))
#define ALIGN_UP(x, y) ((ALIGN_DOWN((x)-1, (y))) + (y))

#define PA2PPN(pa) ((u64)pa >> PAGE_SHIFT)
#define PPN2PA(ppn) ((u64)ppn << PAGE_SHIFT)
#define PA2PTE(pa) (PA2PPN(pa) << PTE_PERM_WIDTH)
#define PTE2PPN(pte) GETLOW((((u64)pte >> PTE_PERM_WIDTH)), 44)
#define PTE2PA(pte) PPN2PA(PTE2PPN(pte))
#define PAGE_OFFSET(x, y) ((x) & ((y)-1))
// 取出权限
#define PTE2PERM(pte) (((u64)(pte)) & ~((1ull << 54) - (1ull << 10)))

#define VAPPN(va, level) GETLOW((u64)va >> (PAGE_SHIFT + 9 * (level)), 9)
#define VAOFFSET(va) GETLOW((u64)va, PAGE_SHIFT)

typedef LIST_HEAD(PageList, Page) PageList;
typedef LIST_ENTRY(PageListEntry, Page) PageListEntry;

typedef struct Page
{
    PageListEntry link;
    u32 ref;
    u32 hardId;
} Page;

void freePageInit();
void kernelPageInit();
void pageStart();
i32 kernelPageMap(u64 *pgdir, u64 va, u64 pa, u64 perm);
i32 pageMap(u64 *pgdir, u64 va, u64 pa, u64 perm);
i32 pageRemove(u64 *pgdir, u64 va);
i32 pageFree(Page *page);
Page *pageLookup(u64 *pgdir, u64 va, u64 **ppte);
i32 pageWalk(u64 *pgdir, u64 va, bool create, u64 **ppte);
i32 pageAlloc(Page **new);
void bzero(void *start, u32 len);
void bcopy(void *src, void *dst, u32 len);
int copyin(u64 *pgdir, char *dst, u64 srcva, u64 len);
int copyout(u64 *pgdir, u64 va, char *src, u64 len);

/**
 * @brief page 2 Page iNdex
 *
 * @param page
 * @return u64 Page iNdex
 */
__attribute__((unused)) static u64 page2PN(Page *page)
{
    extern Page pages[];
    return page - pages;
}

/**
 * @brief Page iNdex 2 page
 *
 * @param pn Page iNdex
 * @return Page*
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
 * @return u64 Physical Address
 */
__attribute__((unused)) static u64 page2PA(Page *page)
{
    return PHYSICAL_MEMORY_BASE + (page2PN(page) << PAGE_SHIFT);
}

/**
 * @brief page 2 Page Table Entry
 *
 * @param page
 * @return u64 PPN part of PTE
 */
__attribute__((unused)) static u64 page2Pte(Page *page)
{
    return PA2PTE(page2PA(page));
}

/**
 * @brief physical address 2 Page iNdex
 *
 * @param pa
 * @return u64 Page iNdex
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
 * @param pa
 * @return Page*
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
i32 pageAlloc(Page **new);
i32 pageInsert(u64 *pgdir, u64 va, Page *pp, u64 perm);

u64 va2PA(u64 *pgdir, u64 va, int *cow);
u64 passiveAlloc(u64 *pgdir, u64 va);
u64 cowHandler(u64 *pgdir, u64 va);

int either_copyin(void *dst, int user_src, u64 src, u64 len);
int either_copyout(int user_dst, u64 dst, void *src, u64 len);
int either_memset(bool user, u64 dst, u8 value, u64 len);
int copyout(u64 *pgdir, u64 va, char *src, u64 len);
int memsetOut(u64 *pgdir, u64 dst, u8 value, u64 len);
void paDecreaseRef(u64 pa);
void *kmalloc(int size);
void kfree(char *startAddress);

#endif /* _MEMORY_H_ */