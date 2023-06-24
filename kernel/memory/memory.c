#include <memory.h>
#include <driver.h>
#include <process.h>
#include <types.h>
#include <error.h>
#include <riscv.h>
#include <string.h>

extern char kernelEnd[];
Page pages[PAGE_NUM];
PageList freePageList;
extern u64 kernelPageDirectory[];

/**
 * @brief 初始化虚拟页表系统
 * 1. 建立并插入空闲页管理块
 * 2. 建立内核页表
 * 3. 开启分页
 */
void memoryInit()
{
    freePageInit();
    // printk("finish freePageInit\n");
    kernelPageInit();
    // printk("finish kernelPageInit\n");
    pageStart();
    // printk("finish pageStart\n");
}

/**
 * @brief 初始化空闲 page 列表
 * 注意，可管理的物理空间为
 * [PHYSICAL_MEMORY_BASE, PHYSICAL_MEMORY_END)
 * 其他地址无需真实 page 管理，直接添加映射
 *
 * 而 TLB 管理的物理空间为
 * [0x0, PHYSICAL_MEMORY_END)
 * 注意两者 PPN 的区别
 *
 * 为设置映射时的 pageAlloc 提供可用页
 */
void freePageInit()
{
    u64 i;
    u64 kernelPageNum = ((u64)kernelEnd - PHYSICAL_MEMORY_BASE) / PAGE_SIZE;
    // 内核占用的 page 永远不会被释放
    for (i = 0; i < kernelPageNum; ++i)
    {
        pages[i].ref = 1;
    }
    for (i = kernelPageNum; i < PAGE_NUM; ++i)
    {
        pages[i].ref = 0;
        LIST_INSERT_HEAD(&freePageList, pages + i, link);
    }
}

/**
 * @brief
 * 创建内核页表
 * 建立必要的映射
 */
void kernelPageInit()
{
    u64 va, pa;
    u64 i, size;
    extern char textStart[];
    extern char textEnd[];
    extern char kernelEnd[];
    // extern char trampoline[];

    // CLINT 	200_0000 ~ 200_C000
    for (i = 0; i < 0x100000; i += PAGE_SIZE)
    {
        kernelPageMap(kernelPageDirectory, CLINT_V + i, CLINT + i,
                      PTE_READ_BIT | PTE_WRITE_BIT | PTE_ACCESSED_BIT | PTE_DIRTY_BIT);
    }
    // PLIC
    for (i = 0; i < 0x4000; i += PAGE_SIZE)
    {
        kernelPageMap(kernelPageDirectory, PLIC_V + i, PLIC + i,
                      PTE_READ_BIT | PTE_WRITE_BIT | PTE_ACCESSED_BIT | PTE_DIRTY_BIT);
    }

    for (i = 0; i < 0x9000; i += PAGE_SIZE)
    {
        kernelPageMap(kernelPageDirectory, PLIC_V + 0x200000 + i, PLIC + 0x200000 + i,
                      PTE_READ_BIT | PTE_WRITE_BIT | PTE_ACCESSED_BIT | PTE_DIRTY_BIT);
    }

    kernelPageMap(kernelPageDirectory, UART0, UART0,
                  PTE_READ_BIT | PTE_WRITE_BIT | PTE_ACCESSED_BIT | PTE_DIRTY_BIT);

    kernelPageMap(kernelPageDirectory, VIRTIO_V, VIRTIO,
                  PTE_READ_BIT | PTE_WRITE_BIT | PTE_ACCESSED_BIT | PTE_DIRTY_BIT);

    // 内核代码
    va = pa = (u64)textStart;
    size = (u64)textEnd - (u64)textStart;
    for (i = 0; i < size; i += PAGE_SIZE)
    {
        kernelPageMap(kernelPageDirectory, va + i, pa + i,
                      PTE_READ_BIT | PTE_EXECUTE_BIT);
    }

    // 内核数据
    va = pa = (u64)textEnd;
    size = (u64)kernelEnd - (u64)textEnd;
    for (i = 0; i < size; i += PAGE_SIZE)
    {
        kernelPageMap(kernelPageDirectory, va + i, pa + i,
                      PTE_READ_BIT | PTE_WRITE_BIT);
    }

    // 内核以上的部分，直接
    va = pa = (u64)kernelEnd;
    size = (u64)PHYSICAL_MEMORY_END - (u64)kernelEnd;
    for (i = 0; i < size; i += PAGE_SIZE)
    {
        kernelPageMap(kernelPageDirectory, va + i, pa + i,
                      PTE_READ_BIT | PTE_WRITE_BIT);
    }

    /* 将处于内核的 TRAMPOLINE 和 TRAPFRAME 暴露到特定地址 */
    // 需要在写进程切换的时候定义 trampoline 和 trapframe
    extern char trampoline[];
    extern char trapframe[];
    kernelPageMap(kernelPageDirectory, TRAMPOLINE, (u64)trampoline,
                  PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    kernelPageMap(kernelPageDirectory, TRAPFRAME, (u64)trapframe,
                  PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
}

/**
 * @brief 开启分页：
 * 1. 刷新 TLB
 * 2. 写 stap 寄存器
 * 3. 刷新 TLB
 */
void pageStart()
{
    sfenceVma();
    writeSatp(MAKE_SATP(SV39, PA2PPN(kernelPageDirectory)));
    sfenceVma();
}

/**
 * @brief 插入 va -> pa 的映射
 * 可用于所有地址
 * [0, PHYSICAL_MEMORY_END)
 * 不处理 page->ref
 *
 * @param pgdir 一级页表指针
 * @param va 虚拟地址
 * @param pa 物理地址
 * @param perm 权限参数
 * @return i32 非 0 异常
 */
i32 kernelPageMap(u64 *pgdir, u64 va, u64 pa, u64 perm)
{
    u64 *pte;
    va = ALIGN_DOWN(va, PAGE_SIZE);
    pa = ALIGN_DOWN(pa, PAGE_SIZE);
    perm |= PTE_ACCESSED_BIT | PTE_DIRTY_BIT;
    try(pageWalk(pgdir, va, false, &pte));
    if ((pte != NULL) && PTE_VALID(*pte))
    {
        // panic("Remapping");
        // return -E_UNSPECIFIED;
        pageRemove(pgdir, va);
    }
    try(pageWalk(pgdir, va, true, &pte));
    *pte = PA2PTE(pa) | perm | PTE_VALID_BIT;
    return 0;
}

/**
 * @brief 插入 va -> pa 的映射
 * 只能用于由页管理块管理的地址
 * [PHYSICAL_MEMORY_BASE, PHYSICAL_MEMORY_END)
 * 处理 page->ref
 *
 * @param pgdir 一级页表指针
 * @param va 虚拟地址
 * @param pa 物理地址
 * @param perm 权限参数
 * @return i32 非 0 异常
 */
i32 pageMap(u64 *pgdir, u64 va, u64 pa, u64 perm)
{
    try(kernelPageMap(pgdir, va, pa, perm));
    u64 *pte = NULL;
    Page *page = pageLookup(pgdir, va, &pte);
    page->ref++;
    return 0;
}

/**
 * @brief 删除 va 和对应物理页的映射
 *
 * @param pgdir
 * @param va
 * @return i32 非 0 删除失败
 */
i32 pageRemove(u64 *pgdir, u64 va)
{
    u64 *pte = NULL;
    Page *page = pageLookup(pgdir, va, &pte);
    if (pte == NULL)
    {
        return -1;
    }
    page->ref--;
    pageFree(page);
    *pte = 0;
    return 0;
}

/**
 * @brief 尝试释放空闲页
 *
 * @param page 页管理块指针
 * @return i32 非 0 释放失败
 */
i32 pageFree(Page *page)
{
    if (page->ref == 0)
    {
        LIST_INSERT_HEAD(&freePageList, page, link);
        return 0;
    }
    return -1;
}

/**
 * @brief 查询 va 对应最后一级页表项的指针
 * 返回 va 对应的 page
 *
 * @param pgdir 一级页表指针
 * @param va 虚拟地址
 * @param ppte 最后一级 PTE 二级指针
 * @return Page* 页控制块指针
 */
Page *pageLookup(u64 *pgdir, u64 va, u64 **ppte)
{
    u64 *pte;
    Page *page;
    panic_on(pageWalk(pgdir, va, false, &pte));
    if ((pte == NULL) || !PTE_VALID(*pte))
    {
        return NULL;
    }
    page = pte2Page(*pte);
    if (ppte != NULL)
    {
        *ppte = pte;
    }
    return page;
}

/**
 * @brief 查询 va 对应最后一级页表项的指针
 * 查询过程中根据 create 插入页表
 *
 * @param pgdir 一级页表指针
 * @param va 虚拟地址
 * @param create 是否在页表中插入新页
 * @param pte 最后一级 PTE 二级指针
 * @return i32 非 0 异常
 */
i32 pageWalk(u64 *pgdir, u64 va, bool create, u64 **ppte)
{
    u64 *pte = pgdir + VAPPN(va, 2);
    Page *page = NULL;
    for (int i = 1; i >= 0; i--)
    {
        if (!PTE_VALID(*pte))
        {
            if (!create)
            {
                *ppte = NULL;
                return 0;
            }
            try(pageAlloc(&page));
            page->ref++;
            *pte = page2Pte(page) | PTE_VALID_BIT;
        }
        pte = (u64 *)PTE2PA(*pte) + VAPPN(va, i);
    }
    *ppte = pte;
    return 0;
}

/**
 * @brief 分配一个空闲物理页
 * 从链表取出一个空闲页，不修改 ref
 *
 * @param ppage 页管理块二级指针
 * @return i32 非 0 异常
 */
i32 pageAlloc(Page **ppage)
{
    Page *page;
    if (LIST_EMPTY(&freePageList))
    {
        return -E_NO_MEM;
    }
    page = LIST_FIRST(&freePageList);
    LIST_REMOVE(page, link);

    // printk("pp: %lx\n", (u64)page2PA(page));
    memset((void *)page2PA(page), 0, PAGE_SIZE);
    // bzero((void *)page2PA(page), PAGE_SIZE);
    *ppage = page;
    return 0;
}

// void bcopy(void *src, void *dst, u32 len)
// {
//     void *finish = src + len;

//     while (src < finish)
//     {
//         *(u8 *)dst = *(u8 *)src;
//         src++;
//         dst++;
//     }
// }

// 记得切换为 memset
void bzero(void *start, u32 len)
{
    void *finish = start + len;
    if (len <= 7)
    {
        while (start < finish)
        {
            *(u8 *)start++ = 0;
        }
        return;
    }
    while (((u64)start) & 7)
    {
        *(u8 *)start++ = 0;
    }
    while (start + 7 < finish)
    {
        *(u64 *)start = 0;
        start += 8;
    }
    while (start < finish)
    {
        *(u8 *)start++ = 0;
    }
}

/**
 * @brief 给 va 插入映射的新页 pp
 * 自动处理 page ref
 *
 * @param pgdir 一级页表指针
 * @param va 被映射的虚拟地址
 * @param pp 映射的物理页管理块
 * @param perm 权限位
 * @return i32 非 0 异常
 */
i32 pageInsert(u64 *pgdir, u64 va, Page *pp, u64 perm)
{
    u64 *pte;
    perm |= PTE_ACCESSED_BIT | PTE_DIRTY_BIT;
    try(pageWalk(pgdir, va, false, &pte));
    // 如果原来已经有映射，并且不等于 pp，移除原来的映射
    if (pte && PTE_VALID(*pte))
    {
        if (pte2Page(*pte) != pp)
        {
            pageRemove(pgdir, va);
        }
        else
        {
            *pte = page2Pte(pp) | perm | PTE_VALID_BIT;
            return 0;
        }
    }
    try(pageWalk(pgdir, va, true, &pte));
    // printk("pte:: %lx\n", pte);
    *pte = page2Pte(pp) | perm | PTE_VALID_BIT;
    pp->ref++;
    return 0;
}

/**
 * @brief 将一段内存内容拷贝到内核中
 *
 * @param dst 目的地址
 * @param isUserSrc 源地址是否是用户地址
 * @param src 源地址
 * @param len 长度
 * @return int 0 为成功
 */
int either_copyin(void *dst, int isUserSrc, u64 src, u64 len)
{
    if (isUserSrc)
    {
        Process *p = myProcess();
        return copyin(p->pgdir, dst, src, len);
    }
    else
    {
        memmove(dst, (char *)src, len);
        return 0;
    }
}

/**
 * @brief 将一段内存从内核中拷贝出来
 *
 * @param isUserDst 目的地址是否是用户地址
 * @param dst 目的地址
 * @param src 源地址
 * @param len 长度
 * @return int 0 为成功
 */
int either_copyout(int isUserDst, u64 dst, void *src, u64 len)
{
    if (isUserDst)
    {
        Process *p = myProcess();
        return copyout(p->pgdir, dst, (char *)src, len);
    }
    else
    {
        memmove((char *)dst, (void *)src, len);
        return 0;
    }
}

int either_memset(bool user, u64 dst, u8 value, u64 len)
{
    if (user)
    {
        Process *p = myProcess();
        return memsetOut(p->pgdir, dst, value, len);
    }
    memset((void *)dst, value, len);
    return 0;
}

/**
 * @brief 查询 va 在 pgdir 中对应的 pa
 * 返回 va 对应数据是否标记为 cow
 * 只可用于用户页表
 *
 * @param pgdir 一级页表指针
 * @param va 虚拟地址
 * @param cow 保存 cow 的指针
 * @return u64 对应的物理地址（0 异常）
 */
u64 va2PA(u64 *pgdir, u64 va, int *cow)
{
    u64 *pte;
    u64 pa;
    if (va >= VA_MAX)
        return 0;
    pageWalk(pgdir, va, 0, &pte);
    if (pte == NULL)
        return 0;
    if (!PTE_VALID(*pte))
        return 0;
    if (!PTE_USER(*pte))
        return 0;
    if (cow)
        *cow = PTE_COW(*pte) > 0;
    pa = PTE2PA(*pte) + VAOFFSET(va);
    return pa;
}

/**
 * @brief 用户进程为 va 申请物理页
 * 权限默认为 RW+U
 *
 * @param pgdir 一级页表指针
 * @param va 用户空间虚拟地址
 */
u64 passiveAlloc(u64 *pgdir, u64 va)
{
    Page *pp = NULL;
    // 需要增加 alloc wrong va 判断
    panic_on(pageAlloc(&pp));
    panic_on(pageInsert(pgdir, va, pp, PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT));

    return page2PA(pp) + (va & 0xFFF);
}

/**
 * @brief 用户进程数据传递给内核
 * 获取用户虚拟地址对应物理地址
 * 直接读物理地址（内核直接映射）
 *
 * @param pgdir
 * @param va
 * @param dst
 * @param len
 * @return int
 */
int copyin(u64 *pgdir, char *dst, u64 va, u64 len)
{
    u64 n, pa;
    int cow;

    while (len > 0)
    {
        pa = va2PA(pgdir, va, &cow); // 内核态获取用户虚拟地址对应物理地址
        panic_on(!pa);
        n = PAGE_SIZE - (pa - ALIGN_DOWN(pa, PAGE_SIZE));
        if (n > len)
        {
            n = len;
        }
        memmove(dst, (void *)pa, n);
        len -= n;
        dst += n;
        va += n;
    }
    return 0;
}

/**
 * @brief kernel 数据传递给用户进程
 * 获取用户虚拟地址对应物理地址
 * 直接写物理地址（内核直接映射）
 *
 * @param pgdir 用户页表
 * @param va 用户虚拟地址
 * @param src 内核地址
 * @param len 长度
 * @return int
 */
int copyout(u64 *pgdir, u64 va, char *src, u64 len)
{
    u64 n, pa;
    int cow;

    while (len > 0)
    {
        pa = va2PA(pgdir, va, &cow);
        if (!pa)
        { // 不存在对应映射，申请新页
            passiveAlloc(pgdir, va);
            pa = va2PA(pgdir, va, &cow);
            // cow = 0;
        }
        if (cow)
        { // cow 手动处理
            cowHandler(pgdir, va);
            pa = va2PA(pgdir, va, &cow);
        }
        n = PAGE_SIZE - (pa - ALIGN_DOWN(pa, PAGE_SIZE));
        if (n > len)
        {
            n = len;
        }
        memmove((void *)pa, src, n);
        len -= n;
        src += n;
        va += n;
    }

    return 0;
}

/* 虚拟地址 memset */
int memsetOut(u64 *pgdir, u64 dst, u8 value, u64 len)
{
    u64 n, va0, pa0;
    int cow;

    while (len > 0)
    {
        va0 = ALIGN_DOWN(dst, PAGE_SIZE);
        pa0 = va2PA(pgdir, dst, &cow);
        if (pa0 == NULL)
        {
            cow = 0;
            pa0 = passiveAlloc(pgdir, dst);
        }
        if (cow)
        {
            pa0 = cowHandler(pgdir, dst);
        }
        n = PAGE_SIZE - (dst - va0);
        if (n > len)
            n = len;
        memset((void *)pa0, value, n);
        len -= n;
        dst = va0 + PAGE_SIZE;
    }
    return 0;
}

void paDecreaseRef(u64 pa)
{
    Page *page = pa2Page(pa);
    page->ref--;
    assert(page->ref == 0);
    if (page->ref == 0)
    {
        LIST_INSERT_HEAD(&freePageList, page, link);
    }
}