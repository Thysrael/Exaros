/**
 * @file elf_loader.c
 * @brief 加载 elf 文件
 * @date 2023-04-13
 *
 * @copyright Copyright (c) 2023
 */

#include <elf.h>
#include <types.h>
#include <error.h>
#include <process.h>
#include <memory.h>
#include <string.h>

/**
 * @brief 将 binary 开始的二进制文件段加载到进程 process va 开始的虚拟内存
 *
 * @param va 加载起始地址
 * @param segmentSize 段大小
 * @param binary 二进制文件地址
 * @param binSize 二进制文件大小
 * @param process process 控制控制块
 * @return int
 */
int codeMapper(u64 va, u32 segmentSize, u8 *binary, u32 binSize, void *process)
{
    Process *pcs = (Process *)process;
    Page *page = NULL;
    u64 offset = va - ALIGN_DOWN(va, PAGE_SIZE);
    u64 i, r = 0;
    u64 *j;
    u64 perm = PTE_EXECUTE_BIT | PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT;

    if (offset > 0)
    {
        page = pageLookup(pcs->pgdir, va, &j);

        if (page == NULL)
        {
            if (pageAlloc(&page) < 0) return -E_NO_MEM;
            kernelPageMap(pcs->pgdir, va, page2PA(page), perm);
        }
        r = MIN(binSize, PAGE_SIZE - offset);
        memmove((void *)page2PA(page) + offset, binary, r);
    }
    for (i = r; i < binSize; i += r)
    {
        if (pageAlloc(&page) != 0) return -E_NO_MEM;

        kernelPageMap(pcs->pgdir, va + i, page2PA(page), perm);
        r = MIN(PAGE_SIZE, binSize - i);
        memmove((void *)page2PA(page), binary + i, r);
    }
    offset = va + i - ALIGN_DOWN(va + i, PAGE_SIZE);
    if (offset > 0)
    {
        page = pageLookup(pcs->pgdir, va + i, &j);
        if (page == NULL)
        {
            if (pageAlloc(&page) < 0) return -E_NO_MEM;
            kernelPageMap(pcs->pgdir, va + i, page2PA(page), perm);
        }
        r = MIN(segmentSize - i, PAGE_SIZE - offset);
        bzero((void *)page2PA(page) + offset, r);
    }
    for (i += r; i < segmentSize; i += r)
    {
        if (pageAlloc(&page) != 0)
            return -E_NO_MEM;
        pageInsert(pcs->pgdir, va + i, page, perm);
        r = MIN(PAGE_SIZE, segmentSize - i);
        bzero((void *)page2PA(page), r);
    }
    return 0;
}

/**
 * @brief 加载 elf 文件到进程对应的内存中
 *
 * @param binary 二进制 elf 文件地址
 * @param size 文件大小
 * @param entry 用于返回 elf 的入口地址
 * @param process process 控制块
 * @return int
 */
int loadElf(u8 *binary, int size, u64 *entry, void *process)
{
    Ehdr *ehdr = (Ehdr *)binary;
    Phdr *phdr = 0;
    u8 *phTable = 0;
    u16 entryCnt, entrySize;

    // check
    if (size < 4 || !isElfFormat(binary))
    {
        return -E_NOT_ELF;
    }

    phTable = binary + ehdr->phoff;
    entryCnt = ehdr->phnum;
    entrySize = ehdr->phentsize;

    while (entryCnt--)
    {
        phdr = (Phdr *)phTable;
        if (phdr->type == PT_LOAD)
        {
            int r = codeMapper(phdr->vaddr, phdr->memsz, binary + phdr->offset, phdr->filesz, process);
            if (r < 0) return r;
        }
        phTable += entrySize;
    }
    *entry = ehdr->entry;
    return 0;
}