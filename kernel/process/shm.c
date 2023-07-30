#include <shm.h>
#include <mem_layout.h>
#include <memory.h>
#include <debug.h>
#include <process.h>
#include <debug.h>

ShareMemory shms[SHM_COUNT];
u64 shmTop = SHM_BASE;

/**
 * @brief 遍历 shms 数组，分配一个 SharedMemory，并按照需求空间的大小在内核分配空间
 *
 * @param size shared memory 的大小
 * @return int shmid，也就是 shms 的 id
 */
int shmAlloc(int key, u64 size, int shmflg)
{
    // key, flag 均没有用到
    SHM_DEBUG("key: %d, size: 0x%lx, shmflag: 0x%x\n", key, size, shmflg);
    for (int i = 0; i < SHM_COUNT; i++)
    {
        if (!shms[i].used)
        {
            shms[i].used = true;
            shms[i].start = shmTop;
            size = ALIGN_UP(size, PAGE_SIZE);
            shms[i].size = size;
            shmTop += size;

            // 在内核分配空间
            u64 cur = shms[i].start;
            u64 end = cur + size;
            SHM_DEBUG("allocate shared memory from 0x%lx to 0x%lx\n", cur, end);
            while (cur < end)
            {
                Page *page;
                extern u64 kernelPageDirectory[];
                pageAlloc(&page);
                // 权限位参考了 trampoline
                pageInsert(kernelPageDirectory, cur, page, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
                cur += PAGE_SIZE;
            }
            return i;
        }
    }
    panic("shm run out.\n");
    return -1;
}

/**
 * @brief 将共享内存和用户的 shm heap 空间建立映射关系
 *
 * @param shmid 共享内存 id
 * @return u64 共享内存在 heap 的首地址
 */
u64 shmAt(int shmid, u64 shmaddr, int shmflg)
{
    // shmaddr, shmflg 均没有用到
    SHM_DEBUG("shmid: %d, shmaddr: 0x%lx, shmflag: 0x%x\n", shmid, shmaddr, shmflg);
    if (shms[shmid].used == false)
    {
        panic("shmAt before shmGet\n");
        return -1;
    }
    Process *p = myProcess();
    u64 kernelCur = shms[shmid].start;
    u64 userCur = p->shmHeapTop;
    u64 userEnd = p->shmHeapTop + shms[shmid].size;
    extern u64 kernelPageDirectory[];
    // int cow;
    while (userCur < userEnd)
    {
        u64 *tmpPte;
        Page *page = pageLookup(kernelPageDirectory, kernelCur, &tmpPte);
        u64 pa = page2PA(page);
        SHM_DEBUG("map user va 0x%lx to kenerl va 0x%lx, pa is 0x%lx\n", userCur, kernelCur, pa);
        pageMap(p->pgdir, userCur, pa, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT | PTE_USER_BIT);
        userCur += PAGE_SIZE;
        kernelCur += PAGE_SIZE;
    }

    p->shmHeapTop += shms[shmid].size;

    return p->shmHeapTop - shms[shmid].size;
}