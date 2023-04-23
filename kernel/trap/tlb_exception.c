#include <memory.h>
#include <driver.h>

void passiveAlloc(u64 *pgdir, u64 va)
{
    Page *pp = NULL;
    // 需要增加 alloc wrong va 判断
    panic_on(pageAlloc(&pp));
    panic_on(pageInsert(pgdir, va, pp, PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT));
}

void tlbExceptionHandler()
{
}

void cowHandler()
{
}

void lazyAllocateHandler()
{
}