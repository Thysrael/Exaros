#include <memory.h>
#include <string.h>
#include <driver.h>

void tlbExceptionHandler()
{
}

u64 cowHandler(u64 *pgdir, u64 va)
{
    Page *page, *newPage;
    u64 *pte = NULL;
    page = pageLookup(pgdir, va, &pte);
    if (!(*pte & PTE_COW_BIT))
    {
        panic("access denied\n");
        return 0;
    }
    int r = pageAlloc(&newPage);
    if (r < 0)
    {
        panic("cow handler error");
        return 0;
    }
    memmove((void *)page2PA(newPage), (void *)page2PA(page), PAGE_SIZE);
    pageInsert(pgdir, va, newPage, (PTE2PERM(*pte) | PTE_WRITE_BIT) & ~PTE_COW_BIT);
    return page2PA(page) + (va & 0xFFF);
}

void lazyAllocateHandler()
{
}