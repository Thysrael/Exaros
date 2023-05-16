#include <memory.h>
#include <driver.h>

void tlbExceptionHandler()
{
}

u64 cowHandler(u64 *pgdir, u64 va)
{
    u64 pa;
    u64 *pte = NULL;
    pa = pageLookup(pgdir, va, &pte);
    if (!(*pte & PTE_COW_BIT))
    {
        panic("access denied\n");
        return 0;
    }
    Page *page;
    int r = pageAlloc(&page);
    if (r < 0)
    {
        panic("cow handler error");
        return 0;
    }
    pa = pageLookup(pgdir, va, &pte);
    bcopy((void *)pa, (void *)page2PA(page), PAGE_SIZE);
    pageInsert(pgdir, va, page, (PTE2PERM(*pte) | PTE_WRITE_BIT) & ~PTE_COW_BIT);
    return page2PA(page) + (va & 0xFFF);
}

void lazyAllocateHandler()
{
}