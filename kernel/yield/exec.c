#include <types.h>
#include <process.h>
#include <elf.h>
#include <fat.h>
#include <memory.h>
#include <mem_layout.h>
#include <trap.h>
#include <dirmeta.h>

#define MAX_ARG 32 // max exec arguments

static int loadSeg(u64 *pagetable,
                   u64 va,
                   DirMeta *dm,
                   u64 offset,
                   u64 sz)
{
    u64 i, n;
    u64 pa;
    int cow;
    for (i = 0; i < sz; i += PAGE_SIZE)
    {
        pa = va2PA(pagetable, va + i, &cow);
        if (pa == NULL)
            panic("loadSeg: address should exist");
        if (cow)
        {
            cowHandler(pagetable, va + i);
        }
        if (sz - i < PAGE_SIZE)
            n = sz - i;
        else
            n = PAGE_SIZE;
        if (metaRead(dm, 0, (u64)pa, offset + i, n) != n)
            return -1;
    }

    return 0;
}

static int prepSeg(u64 *pagetable, u64 va, u64 filesz)
{
    assert(va % PAGE_SIZE == 0);
    for (int i = va; i < va + filesz; i += PAGE_SIZE)
    {
        Page *p;
        if (pageAlloc(&p) < 0)
            return -1;
        pageInsert(pagetable, i, p,
                   PTE_EXECUTE_BIT | PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT);
    }
    return 0;
}

/**
 * @brief 执行 path 文件，参数列表为 argv
 *
 * @param path
 * @param argv
 * @return u64
 */
u64 exec(char *path, char **argv)
{
    int i, off;
    u64 argc, sp, ustack[MAX_ARG], stackbase;
    Ehdr elf;
    DirMeta *dm;
    Phdr ph;
    u64 *pagetable = 0;
    Process *p = myProcess();
    Page *page;

    u64 *oldPageTable = p->pgdir;

    // 为新进程申请一个页表
    int r = allocPgdir(&page);
    if (r < 0)
    {
        panic("pgdir alloc error\n");
        return r;
    }
    p->mmapHeapBottom = USER_HEAP_BOTTOM;
    pagetable = (u64 *)page2PA(page);
    extern char trampoline[];
    extern char trapframe[];

    pageInsert(pagetable, TRAMPOLINE, pa2Page((u64)trampoline), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    pageInsert(pagetable, TRAPFRAME, pa2Page((u64)trapframe), PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);

    if ((dm = metaName(AT_FDCWD, path, true)) == 0)
    {
        printk("find file error, path: %s\n", path);
        return -1;
    }

    // 读 elf header
    if (metaRead(dm, 0, (u64)&elf, 0, sizeof(elf)) != sizeof(elf))
    {
        printk("read header error\n");
        goto bad;
    }
    if (!isElfFormat((u8 *)&elf))
    {
        printk("not elf format\n");
        goto bad;
    }

    // begin map
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph))
    {
        if (metaRead(dm, 0, (u64)&ph, off, sizeof(ph)) != sizeof(ph))
            goto bad;
        if (ph.type != PT_LOAD)
            continue;
        if (ph.memsz < ph.filesz)
            goto bad;
        if (ph.vaddr + ph.memsz < ph.vaddr)
            goto bad;
        if (prepSeg(pagetable, ph.vaddr, ph.memsz))
            goto bad;
        if ((ph.vaddr % PAGE_SIZE) != 0)
            goto bad;
        if (loadSeg(pagetable, ph.vaddr, dm, ph.offset, ph.filesz) < 0)
            goto bad;
    }
    sp = USER_STACK_TOP;
    stackbase = sp - PAGE_SIZE;
    if (pageAlloc(&page))
    {
        printk("allock stack error\n");
        goto bad;
    }
    pageInsert(pagetable, stackbase, page,
               PTE_EXECUTE_BIT | PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT);

    // Push argument strings, prepare rest of stack in ustack.
    for (argc = 0; argv[argc]; argc++)
    {
        if (argc >= MAX_ARG)
            goto bad;
        sp -= strlen(argv[argc]) + 1;
        sp -= sp % 16; // riscv sp must be 16-byte aligned
        if (sp < stackbase)
            goto bad;
        if (copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
            goto bad;
        ustack[argc] = sp;
    }
    ustack[argc] = 0;

    // push the array of argv[] pointers.
    sp -= (argc + 1) * sizeof(u64);
    sp -= sp % 16;
    if (sp < stackbase)
        goto bad;
    if (copyout(pagetable, sp, (char *)ustack, (argc + 1) * sizeof(u64)) < 0)
        goto bad;

    // main(argc, argv)
    // a1 是第二个参数的地址
    getHartTrapFrame()->a1 = sp;

    // Save program name for debugging.
    /*
    for (last = s = path; *s; s++)
        if (*s == '/')
            last = s + 1;
    safestrcpy(p->name, last, sizeof(p->name));
    */

    // Commit to the user image.
    p->pgdir = pagetable;
    getHartTrapFrame()->epc = elf.entry; // initial program counter = main
    getHartTrapFrame()->sp = sp;         // initial stack pointer

    // free old pagetable
    pgdirFree(oldPageTable);
    asm volatile("fence.i");
    return argc; // this ends up in a0, the first argument to main(argc, argv)

bad:
    if (pagetable)
        pgdirFree((u64 *)pagetable);
    return -1;
}
