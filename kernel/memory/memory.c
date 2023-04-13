#include <memory.h>
#include <driver.h>
#include <types.h>
#include <error.h>
#include <riscv.h>

extern char kernelEnd[];
Page pages[PAGE_NUM];
PageList freePageList;
extern u64 kernelPageDirectory[];

void freePageInit();
void kernelPageInit();
void pageStart();
i32 pageMap(u64 *pgdir, u64 va, u64 pa, u64 perm);
i32 pageRemove(u64 *pgdir, u64 va);
i32 pageFree(Page *page);
Page *pageLookup(u64 *pgdir, u64 va, u64 **ppte);
i32 pageWalk(u64 *pgdir, u64 va, bool create, u64 **ppte);
i32 pageAlloc(Page **new);
void bzero(void *start, u32 len);

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
 * 其他地址无需真实 page 管理，添加映射即可
 *
 * 而 TLB 管理的物理空间为
 * [0x0, PHYSICAL_MEMORY_END)
 * 注意两者 PPN 的区别
 */
void freePageInit()
{
    u64 i;
    u64 kernelPageNum = ((u64)kernelEnd - PHYSICAL_MEMORY_BASE) / PAGE_SIZE;
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
    extern char trampoline[];

    pageMap(kernelPageDirectory, CLINT, CLINT,
            PTE_READ_BIT | PTE_WRITE_BIT);

    pageMap(kernelPageDirectory, PLIC, PLIC,
            PTE_READ_BIT | PTE_WRITE_BIT);

    pageMap(kernelPageDirectory, UART0, UART0,
            PTE_READ_BIT | PTE_WRITE_BIT);

    pageMap(kernelPageDirectory, VIRTIO, VIRTIO,
            PTE_READ_BIT | PTE_WRITE_BIT);

    va = pa = (u64)textStart;
    size = (u64)textEnd - (u64)textStart;
    for (i = 0; i < size; i += PAGE_SIZE)
    {
        pageMap(kernelPageDirectory, va + i, pa + i,
                PTE_READ_BIT | PTE_EXECUTE_BIT);
    }

    va = pa = (u64)textEnd;
    size = (u64)kernelEnd - (u64)textEnd;
    for (i = 0; i < size; i += PAGE_SIZE)
    {
        pageMap(kernelPageDirectory, va + i, pa + i,
                PTE_READ_BIT | PTE_WRITE_BIT);
    }

    va = pa = (u64)kernelEnd;
    size = (u64)PHYSICAL_MEMORY_END - (u64)kernelEnd;
    for (i = 0; i < size; i += PAGE_SIZE)
    {
        pageMap(kernelPageDirectory, va + i, pa + i,
                PTE_READ_BIT | PTE_WRITE_BIT);
    }

    /**
     * 映射 trampoline
     * 注意到 trampoline 本身位于 Kernel text 被直接映射一次
     * 再映射到高位一次
     */
    pageMap(kernelPageDirectory, TRAMPOLINE, (u64)trampoline,
            PTE_READ_BIT | PTE_EXECUTE_BIT);
}

/**
 * @brief
 * 开启分页：
 * 1. 写 stap 寄存器
 * 2. 刷新 TLB
 */
void pageStart()
{
    sfenceVma();
    writeSatp(MAKE_SATP(SV39, PA2PPN(kernelPageDirectory)));
    sfenceVma();
}

/**
 * @brief
 * 插入 va -> pa 的映射
 *
 * @param pgdir 一级页表指针
 * @param va 虚拟地址
 * @param pa 物理地址
 * @param perm 权限参数
 * @return i32 非零值不正常退出
 */
i32 pageMap(u64 *pgdir, u64 va, u64 pa, u64 perm)
{
    u64 *pte;
    i32 ret;
    va = ALIGN_DOWN(va, PAGE_SIZE);
    pa = ALIGN_DOWN(pa, PAGE_SIZE);
    perm |= PTE_ACCESSED_BIT | PTE_DIRTY_BIT;
    ret = pageWalk(pgdir, va, false, &pte);
    if ((pte != NULL) && PTE_VALID(*pte))
    {
        printk("Panic: remap\n");
        return -1;
    }
    ret = pageWalk(pgdir, va, true, &pte);
    if (ret)
    {
        return ret;
    }
    *pte = PA2PTE(pa) | perm | PTE_VALID_BIT;
    /* pa 对应页管理块未进行操作
    因为这个块本身只有 OS 访问 */
    return 0;
}

/**
 * @brief 删除 va 和对应物理页的映射
 *
 * @param pgdir
 * @param va
 * @return i32 非 0 则删除失败
 */
i32 pageRemove(u64 *pgdir, u64 va)
{
    u64 *pte = NULL;
    Page *page = pageLookup(pgdir, va, &pte);
    if (pte == NULL)
    {
        return -1;
        // TODO
    }
    page->ref--;
    pageFree(page);
    *pte = 0;
    return 0;
}

/**
 * @brief 释放空闲页
 *
 * @param page 页管理块指针
 * @return i32 非零则释放失败
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
    pageWalk(pgdir, va, false, &pte);
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
 * @return i32 非零则查询失败
 */
i32 pageWalk(u64 *pgdir, u64 va, bool create, u64 **ppte)
{
    // printk("---- Walk begin\n");
    // printk("va: 0x%lx\n", va);

    u64 *pte = pgdir + VAPPN(va, 2);
    Page *page = NULL;
    for (int i = 1; i >= 0; i--)
    {
        // printk("*PTE: 0x%lx\n", pte);
        // printk("PTE: 0x%lx\n", *pte);
        if (!PTE_VALID(*pte))
        {
            if (!create)
            {
                *ppte = NULL;
                // printk("---- Walk End\n");
                return -1;
            }
            pageAlloc(&page);
            page->ref++;
            *pte = page2Pte(page) | PTE_VALID_BIT;
        }
        // printk("new PTE: 0x%lx\n", *pte);
        // printk("new PTE PA: 0x%lx\n", PTE2PA(*pte));
        pte = (u64 *)PTE2PA(*pte) + VAPPN(va, i);
    }
    *ppte = pte;
    // printk("---- Walk End\n");
    return 0;
}

/**
 * @brief
 * 分配一个空闲物理页
 *
 * @param ppage 页管理块二级指针
 * @return i32 非零值不正常退出
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
    bzero((void *)page2PA(page), PAGE_SIZE);
    *ppage = page;
    return 0;
}

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