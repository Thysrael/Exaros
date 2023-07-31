#include <types.h>
#include <process.h>
#include <elf.h>
#include <fat.h>
#include <memory.h>
#include <mem_layout.h>
#include <trap.h>
#include <dirmeta.h>
#include <debug.h>
#include <mmap.h>
#include <auxvec.h>
#include <string.h>
#include <thread.h>

#define MAX_ARG 32 // max exec arguments

// static int loadSeg(u64 *pagetable,
//                    u64 va,
//                    DirMeta *dm,
//                    u64 offset,
//                    u64 sz)
// {
//     u64 pa;
//     int cow;
//     u64 n;
//     u64 begin = va;
//     u64 end = va + sz;
//     printk("load seg at 0x%lx, offset is 0x%lx\n", begin, offset + (begin - va));
//     while (begin < end)
//     {
//         pa = va2PA(pagetable, begin, &cow);
//         if (pa == NULL)
//             panic("loadSeg: address should exist");
//         if (cow)
//         {
//             cowHandler(pagetable, begin);
//         }
//         n = PAGE_SIZE < end - begin ? PAGE_SIZE : end - begin;
//         if (metaRead(dm, 0, (u64)pa, offset + (begin - va), n) != n)
//             return -1;
//         begin += n;
//     }

//     return 0;
// }

// static int prepSeg(u64 *pagetable, u64 va, u64 filesz)
// {
//     // printk("prepseg;");
//     // if (va % PAGE_SIZE != 0)
//     // {
//     //     printk("seg va begin = 0x%lx\n", va);
//     // }
//     // assert(va % PAGE_SIZE == 0);
//     Page *p;
//     u64 pa;
//     int cow;
//     u64 begin = va;
//     u64 end = va + filesz;
//     u64 n;
//     while (begin < end)
//     {
//         pa = va2PA(pagetable, begin, &cow);
//         if (pa == NULL)
//         {
//             if (pageAlloc(&p) < 0)
//                 return -1;
//             pageInsert(pagetable, begin, p,
//                        PTE_EXECUTE_BIT | PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT);
//         }
//         n = PAGE_SIZE < end - begin ? PAGE_SIZE : end - begin;
//         begin += n;
//     }
//     return 0;
// }

// /**
//  * @brief 执行 path 文件，参数列表为 argv
//  *
//  * @param path 可执行文件或 shell 脚本文件的路径
//  * @param argv 执行参数
//  * @return u64 执行结果
//  */
// u64 exec(char *path, char **argv)
// {
//     int i, off;
//     u64 argc, sp, ustack[MAX_ARG + 1], stackbase;
//     Ehdr elf;
//     DirMeta *dm;
//     Phdr ph;
//     u64 *pagetable = 0;
//     Process *p = myProcess();
//     Page *page;

//     u64 *oldPageTable = p->pgdir;

//     p->mmapHeapTop = USER_MMAP_HEAP_BOTTOM;
//     p->brkHeapTop = USER_BRK_HEAP_BOTTOM;

//     // 为新进程申请一个页表
//     int r = allocPgdir(&page);

//     if (r < 0)
//     {
//         panic("pgdir alloc error\n");
//         return r;
//     }
//     pagetable = (u64 *)page2PA(page);
//     // printk("pagetable addr = 0x%lx\n", pagetable);

//     extern char trampoline[];
//     extern char trapframe[];

//     pageMap(pagetable, TRAMPOLINE, (u64)trampoline, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
//     pageMap(pagetable, TRAPFRAME, (u64)trapframe, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);

//     if ((dm = metaName(AT_FDCWD, path, true)) == 0)
//     {
//         printk("find file error, path: %s\n", path);
//         return -1;
//     }

//     // 读 elf header
//     if (metaRead(dm, 0, (u64)&elf, 0, sizeof(elf)) != sizeof(elf))
//     {
//         printk("read header error\n");
//         goto bad;
//     }
//     if (!isElfFormat((u8 *)&elf))
//     {
//         printk("not elf format\n");
//         goto bad;
//     }
//     // begin map
//     for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph))
//     {
//         if (metaRead(dm, 0, (u64)&ph, off, sizeof(ph)) != sizeof(ph))
//             goto bad;
//         if (ph.type != PT_LOAD)
//             continue;
//         if (ph.memsz < ph.filesz)
//             goto bad;
//         if (ph.vaddr + ph.memsz < ph.vaddr)
//             goto bad;
//         if (prepSeg(pagetable, ph.vaddr, ph.memsz))
//             goto bad;
//         // if ((ph.vaddr % PAGE_SIZE) != 0)
//         //     goto bad;
//         if (loadSeg(pagetable, ph.vaddr, dm, ph.offset, ph.filesz) < 0)
//             goto bad;
//     }
//     sp = USER_STACK_TOP;
//     stackbase = sp - PAGE_SIZE;
//     if (pageAlloc(&page))
//     {
//         printk("allock stack error\n");
//         goto bad;
//     }
//     pageInsert(pagetable, stackbase, page,
//                PTE_EXECUTE_BIT | PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT);

//     // Push argument strings, prepare rest of stack in ustack.
//     for (argc = 0; argv[argc]; argc++)
//     {
//         if (argc >= MAX_ARG)
//             goto bad;
//         sp -= strlen(argv[argc]) + 1;
//         sp -= sp % 16; // riscv sp must be 16-byte aligned
//         if (sp < stackbase)
//             goto bad;
//         if (copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
//             goto bad;
//         ustack[argc + 1] = sp;
//     }
//     // ustack[argc] = argc;? todo
//     ustack[0] = argc;
//     ustack[argc + 1] = 0;

//     // push the array of argv[] pointers.
//     sp -= (argc + 2) * sizeof(u64);
//     sp -= sp % 16;
//     if (sp < stackbase)
//         goto bad;
//     if (copyout(pagetable, sp, (char *)ustack, (argc + 2) * sizeof(u64)) < 0)
//         goto bad;

//     // main(argc, argv)
//     // a1 是第二个参数的地址
//     // getHartTrapFrame()->a1 = sp;

//     // Save program name for debugging.
//     /*
//     for (last = s = path; *s; s++)
//         if (*s == '/')
//             last = s + 1;
//     safestrcpy(p->name, last, sizeof(p->name));
//     */

//     // Commit to the user image.
//     p->pgdir = pagetable;
//     getHartTrapFrame()->epc = elf.entry; // initial program counter = main
//     // printk("epc:: %lx");
//     getHartTrapFrame()->sp = sp; // initial stack pointer

//     // free old pagetable
//     pgdirFree(oldPageTable);
//     asm volatile("fence.i");
//     // return argc; // this ends up in a0, the first argument to main(argc, argv)
//     return 0;

// bad:
//     if (pagetable)
//         pgdirFree((u64 *)pagetable);
//     return -1;
// }

/**
 * @brief 判断是否是 shell script
 *
 * @param binary 加载文件的内容
 * @return int 1 为是
 */
static inline int isScriptFormat(u8 *binary)
{
    return (binary[0] == '#' && binary[1] == '!');
}

/**
 * @brief 加载脚本，对脚本的第一行进行处理，获得脚本解释器的路径和参数，重新执行 exec
 *
 * @param srcMeta 脚本的 meta
 * @param path 脚本路径
 * @param argv 执行参数
 * @return int
 */
static u64 loadScript(DirMeta *srcMeta, char *path, char **argv)
{
    LOAD_DEBUG("shell script %s begin.\n", path);
    // 这里读取了脚本的前 128 字节，是 linux 的处理方式，这个 head 用于获得脚本的第一行
#define BINPRM_BUF_SIZE 128
    char script_head[BINPRM_BUF_SIZE];
    metaRead(srcMeta, 0, (u64)script_head, 0, BINPRM_BUF_SIZE - 1);

    char *cp = NULL; // cp 是一个常用的字符指针
    // 换行符说明前面的解释器已经写明了，cp 此时指向解释器的后一个字符
    // 如果没有在 bprmbuf 中找到 '\n'，那么，就将 cp 指针指到 bprmbuf 最后
    if ((cp = strchr(script_head, '\n')) == NULL)
        cp = script_head + strlen(script_head);
    *cp = '\0';
    // 对应没有 '\n' 的情况，进行倒序空白符收缩
    while (cp > script_head)
    {
        cp--;
        if ((*cp == ' ') || (*cp == '\t'))
            *cp = '\0';
        else
            break;
    }

    // 跳过前面的 "#!" 和空白符去读取解释器的路径
    for (cp = script_head + 2; (*cp == ' ') || (*cp == '\t'); cp++)
        ;
    char *interp_path = cp;

    // 获得解释器的参数（也就是同一行）
    char *interp_arg = NULL;
    // 将 cp 移动到下一个空白符
    for (; *cp && (*cp != ' ') && (*cp != '\t'); cp++)
        /* nothing */;
    // 将这个空白符赋成 '\0'
    while ((*cp == ' ') || (*cp == '\t'))
        *cp++ = '\0';
    // 这个空白符后的内容就是 i_arg
    if (*cp)
        interp_arg = cp;

    LOAD_DEBUG("interp path %s\n", interp_path);
    // TODO: 这是一个为了应对 /bin/sh 打的补丁
    if (strncmp(interp_path, "/bin/sh", 7) == 0)
    {
        // printk("some patch begin\n");
        char *scriptArgv[4] = {"./busybox", "sh"};
        scriptArgv[2] = path;
        scriptArgv[3] = NULL;
        return exec("/busybox", scriptArgv);
    }
    if (strncmp(interp_path, "/bin/busybox", 12) == 0)
    {
        interp_path = "/busybox";
    }
    LOAD_DEBUG("changing interp path: %s\n", interp_path);
    // 制作新的参数
    char *script_argv[MAX_ARG] = {0};
    int script_argc = 0;
    // 第一个参数是解释器的路径
    script_argv[script_argc++] = interp_path;
    // 如果有解释器参数的话，就拷贝解释器参数
    if (interp_arg)
    {
        script_argv[script_argc++] = interp_arg;
        LOAD_DEBUG("interp argv %s\n", interp_arg);
    }
    // 将原来的 argv 拷贝到 script_argv 中
    while (*argv)
    {
        script_argv[script_argc++] = *(argv++);
    }
    return exec(interp_path, script_argv);
}

/**
 * @brief 通过遍历待加载的程序的 segments，对于不同类型的 segment 进行不同操作。
 * 对于 LOAD，需要分配并加入 SegmentMap，为了后续的懒加载；
 * 对于 PHDR，需要记录 program header table 的首地址，方便后续的用户栈 AUX 信息；
 * 对于 INTERP，需要获得动态链接解释器的 DirMeta，方便后续的加载
 * 对于 PT_GNU_PROPERTY，需要报错
 *
 * @param srcMeta 待加载文件 meta
 * @param elfHeader 待加载文件的 elfHeader
 * @param interpOffset 动态解释器的偏移量
 * @param phdrAddr 返回值，返回 phAddr 信息
 * @param interpMeta 返回值，返回解释器的 meta
 */
static void segmentTraverse(DirMeta *srcMeta, Ehdr *elfHeader, u64 interpOffset, u64 *phdrAddr, DirMeta **interpMeta)
{
    LOAD_DEBUG("segment traverse begin.\n");
    LOAD_DEBUG("interOffset is 0x%x\n", interpOffset);
    // 遍历的 program header table，对于不同的 ph 进行不同的处理
    Phdr ph;
    Process *p = myProcess();
    LOAD_DEBUG("phnum is %d, phoff is 0x%x\n", elfHeader->phnum, elfHeader->phoff);
    for (int i = 0, off = elfHeader->phoff; i < elfHeader->phnum; i++, off += sizeof(Phdr))
    {
        metaRead(srcMeta, false, (u64)&ph, off, sizeof(Phdr));
        // 需要加载的段，那么采用懒加载策略
        if (ph.type == PT_LOAD)
        {
            SegmentMap *seg;
            // 进行非空白段的段映射
            if (ph.filesz > 0)
            {
                seg = segmentMapAlloc();
                seg->src = srcMeta;
                seg->srcOffset = ph.offset;
                seg->loadAddr = ph.vaddr + interpOffset;
                LOAD_DEBUG("load address start at 0x%lx, 0x%lx\n", seg->loadAddr, ph.vaddr);
                seg->len = ph.filesz;
                seg->flag = PTE_EXECUTE_BIT | PTE_READ_BIT | PTE_WRITE_BIT;
                segmentMapAppend(p, seg);
            }
            // 进行空白段的段映射，也就是即使原本是一个段，但是在这里也会被改为两个段
            if (ph.memsz > ph.filesz)
            {
                seg = segmentMapAlloc();
                seg->src = NULL;
                seg->srcOffset = 0;
                seg->loadAddr = ph.vaddr + ph.filesz + interpOffset;
                LOAD_DEBUG("load bss address start at 0x%lx, 0x%lx\n", seg->loadAddr, ph.vaddr);
                seg->len = ph.memsz - ph.filesz;
                seg->flag = PTE_READ_BIT | PTE_WRITE_BIT | MAP_ZERO;
                segmentMapAppend(p, seg);
            }

            // [ph.offset, ph.offset + ph.filesz]，program header table 在这个区间内，这一段就是 PHDR
            if (ph.offset <= elfHeader->phoff && elfHeader->phoff < ph.offset + ph.filesz)
            {
                *phdrAddr = elfHeader->phoff - ph.offset + ph.vaddr + interpOffset;
                LOAD_DEBUG("traverse the ph table, find the program header table, at %x\n", *phdrAddr);
            }
        }
        // 需要加载解释器
        else if (ph.type == PT_INTERP)
        {
            // 这个段里面应该记载着解释器的路径
            if (ph.filesz > 4096 || ph.filesz < 2)
                panic("interpreter path too long or too short!\n");

            // 读取段的内容，实际上就是动态解释器的路径
            char interp_path[FAT32_MAX_PATH];
            metaRead(srcMeta, 0, (u64)interp_path, ph.offset, ph.filesz);
            LOAD_DEBUG("dynamic interpreter path is %s\n", interp_path);
            // 由于 Fat32 文件系统不支持动态链接功能，因此比赛时各队伍请将 `/lib/ld-musl-riscv64-sf.so.1` 当作  `/libc.so` 处理
            strncpy(interp_path, "/libc.so\0", strlen("/libc.so") + 1);
            LOAD_DEBUG("changing dynamic interpreter path is %s\n", interp_path);
            // 根据这个东西查找 libc.so.6 的 DirMeta
            *interpMeta = metaName(AT_FDCWD, interp_path, true);
            if (*interpMeta == NULL)
                panic("open interpreter error!");
        }
        else if (ph.type == PT_GNU_PROPERTY)
        {
            panic("do not support PT_GNU_PROPERTY Segment");
        }
    }
}

/**
 * @brief 获得这个程序的所有 LOAD segment 的大小之和
 *
 * @param phdr 程序头表
 * @param phnum Phdr 的个数
 * @return u64 load segment 的总大小，更精确说的是覆盖范围（中间可能是空的）
 */
static u64 totalLoadRange(const Phdr *phdr, int phnum)
{
    u64 min_addr = -1;
    u64 max_addr = 0;
    bool pt_load = false;
    int i;

    for (i = 0; i < phnum; i++)
    {
        if (phdr[i].type == PT_LOAD)
        {
            min_addr = MIN(min_addr, ALIGN_DOWN(phdr[i].vaddr, PAGE_SIZE)); // 最低虚拟地址
            max_addr = MAX(max_addr, phdr[i].vaddr + phdr[i].memsz);        // 最高虚拟地址
            pt_load = true;
        }
    }
    return pt_load ? (max_addr - min_addr) : 0;
}

/**
 * @brief 只被用于解释器部分，应该是将解释器的所有 LOAD segment 映射到内存中。
 * 但是从调用情况来看，似乎每次只映射 segment
 *
 * @param filep 代表解释器的文件
 * @param addr load 的地址
 * @param eppnt 需要加载的 segment
 * @param prot 权限
 * @param type 某种像类型一样的东西
 * @param total_size load segment 的总长度
 * @return u64 映射后内存起始地址
 */
static u64 elf_map(struct File *filep, u64 addr, Phdr *eppnt, int prot, int type, u64 total_size)
{
    u64 map_addr;
    u64 size = eppnt->filesz + PAGE_OFFSET(eppnt->vaddr, PAGE_SIZE);
    u64 off = eppnt->offset - PAGE_OFFSET(eppnt->vaddr, PAGE_SIZE);
    addr = ALIGN_DOWN(addr, PAGE_SIZE);

    if (!size)
        return addr;

    /*
     * total_size is the size of the ELF (interpreter) image.
     * The _first_ do_mmap needs to know the full size, otherwise
     * randomization might put this image into an overlapping
     * position with the ELF binary image. (since size < total_size)
     * So we first map the 'big' image - and unmap the remainder at
     * the end. (which unmap is needed for ELF images with holes.)
     */
    // 这是第一次的情况，利用 do_map 将整个区域全部占住，这次是一个匿名 do_mmap
    // 之后在传参的时候，我们会注意将 total_size 置为 0
    if (total_size)
    {
        total_size = ALIGN_UP(total_size, PAGE_SIZE);
        map_addr = do_mmap(NULL, addr, total_size, prot, type, off);
        addr = map_addr;
        LOAD_DEBUG("first do map at 0x%lx\n", addr);
    }

    map_addr = do_mmap(filep, addr, size, prot, type, off);

    return map_addr;
}

static inline int make_prot(u32 p_flags)
{
    return PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT | PTE_USER_BIT | PTE_ACCESSED_BIT | PTE_DIRTY_BIT;
}

// static u64 heapInterpMap(u64 *pagetable, u64 total_size)
// {
//     Process *proc = myProcess();
//     // 分配出堆空间
//     proc->mmapHeapTop = ALIGN_UP(proc->mmapHeapTop, PAGE_SIZE);
//     u64 start = proc->mmapHeapTop;
//     proc->mmapHeapTop = ALIGN_UP(proc->mmapHeapTop + total_size, PAGE_SIZE);
//     assert(proc->mmapHeapTop < USER_STACK_BOTTOM);

//     Page *p;
//     u64 pa;
//     int cow;
//     u64 begin = start;
//     u64 end = begin + total_size;
//     u64 n;
//     while (begin < end)
//     {
//         printk("pre seg at 0x%lx\n", begin);
//         pa = va2PA(pagetable, begin, &cow);
//         if (pa == NULL)
//         {
//             if (pageAlloc(&p) < 0)
//                 return -1;
//             pageInsert(pagetable, begin, p,
//                        PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT | PTE_USER_BIT);
//         }
//         n = PAGE_SIZE < end - begin ? PAGE_SIZE : end - begin;
//         begin += n;
//     }
//     return start;
// }

/**
 * @brief 加载动态链接器，似乎是将动态链接器的 load segment 都加载到内存中
 *
 * @param pagetable 待加载进程的页表
 * @param interp_elf_ex 解释器的 elf 头
 * @param interpreter 解释器文件条目
 * @param no_base 这个值传入时为 0
 * @param interp_elf_phdata 解释器的 program table 的首地址
 * @return u64
 */
u64 load_elf_interp(u64 *pagetable, Ehdr *interp_elf_ex, DirMeta *interpreter, u64 no_base, Phdr *interp_elf_phdata)
{
    Phdr *eppnt;
    u64 load_addr = 0;
    // 应该是一个 bool 值，表示 load_addr 是否被赋值
    int load_addr_set = 0;
    u64 error = ~0UL;
    u64 total_size;
    int i;

    total_size = totalLoadRange(interp_elf_phdata, interp_elf_ex->phnum);
    if (!total_size)
    {
        error = -22;
        goto out;
    }
    LOAD_DEBUG("interper total load size is 0x%lx\n", total_size);
    /*
    | <-----------------total_size------------------> |
    | <------Seg1------>     <---------Seg2-------->  |
    首先申请 total_size（这里是不固定的，需要 OS 来分配）。直接调用 do_mmap();
    然后再把每个段填进去（这里的地址是确定的）
    */
    // load_addr = heapInterpMap(pagetable, total_size);
    // 遍历所有的 segment 项
    eppnt = interp_elf_phdata;
    for (i = 0; i < interp_elf_ex->phnum; i++, eppnt++)
    {
        if (eppnt->type == PT_LOAD)
        {
            // loadSeg(pagetable, eppnt->vaddr + load_addr, interpreter, eppnt->offset, eppnt->filesz);
            // type 是做 mmap 的类型
            int elf_type = MAP_PRIVATE_BIT;
            // prot 是页表的权限相关
            int elf_prot = make_prot(eppnt->flags);
            u64 vaddr = 0;
            // load 段的起始地址
            u64 map_addr;

            vaddr = eppnt->vaddr;
            LOAD_DEBUG("load vaddr is 0x%lx\n", vaddr);
            /* 第一次会走第二个分支，第二次会走第一个分支 */
            if (interp_elf_ex->type == ET_EXEC || load_addr_set)
            {
                elf_type |= MAP_FIXED_BIT;
                elf_prot = PTE_READ_BIT | PTE_WRITE_BIT | PTE_USER_BIT | PTE_ACCESSED_BIT;
            }

            else if (no_base && interp_elf_ex->type == ET_DYN)
                load_addr = -vaddr;
            LOAD_DEBUG("elf map addr is 0x%lx\n", load_addr + vaddr);
            struct File interp_file;
            interp_file.meta = interpreter;
            interp_file.type = FD_ENTRY;
            interp_file.readable = 1;
            map_addr = elf_map(&interp_file, load_addr + vaddr, eppnt, elf_prot, elf_type, total_size);
            total_size = 0;
            error = map_addr;
            if (map_addr == -1)
                panic("elf_map failed");

            if (!load_addr_set && interp_elf_ex->type == ET_DYN)
            {
                load_addr = map_addr - PAGE_OFFSET(vaddr, PAGE_SIZE);
                load_addr_set = 1;
            }
        }
    }

    error = load_addr;
out:
    return error;
}

/**
 * @brief 给当前进程分配一个新的页表
 *
 * @return u64* 原有页表
 */
static u64 *execAllocPgdir()
{
    u64 *pagetable = 0, *old_pagetable = myProcess()->pgdir;
    Page *page;
    int r = allocPgdir(&page);
    if (r < 0)
    {
        panic("setup page alloc error\n");
        return NULL;
    }
    pagetable = (u64 *)page2PA(page);
    myProcess()->pgdir = pagetable;
    LOAD_DEBUG("alloc a pgdir\n");
    return old_pagetable;
}

/*
    用户栈结构
    argc
    argv[1]
    argv[2]
    NULL

    envp[1]
    envp[2]
    NULL

    random(???)

    AT_HWCAP ELF_HWCAP
    AT_PAGESZ ELF_EXEC_PAGESIZE
    NULL NULL
    "argv1"
    "argv2"
    "va=a"
    "vb=b"
*/

static u64 initUserStack(char **argv, u64 phdrAddr, Ehdr *elfHeader, u64 interpLoadAddr, u64 interpOffset)
{
    LOAD_DEBUG("init user stack begin.\n");
    Process *p = myProcess();
    u64 *pagetable = p->pgdir;
    u64 sp = USER_STACK_TOP;

    u64 ustack[MAX_ARG + AT_VECTOR_SIZE];

    // 将 argv 拷贝到 ustack 中
    int argc = 0;
    for (argc = 0; argv[argc]; argc++)
    {
        sp -= strlen(argv[argc]) + 1;
        sp -= sp % 16; // riscv sp must be 16-byte aligned
        copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1);
        ustack[argc + 1] = sp;
    }
    ustack[0] = argc;
    ustack[argc + 1] = 0;
    LOAD_DEBUG("finished push argv\n");

    // 拷贝环境变量
    // char *envVariable[] = {"PATH=/"};
    char *envVariable[] = {"LD_LIBRARY_PATH=.", "PATH=/", "UB_BINDIR=./" /*, "LOOP_O=11", "TIMING_O=1", "ENOUGH=1"*/};

    int envCount = sizeof(envVariable) / sizeof(char *);
    for (int i = 0; i < envCount; i++)
    {
        sp -= strlen(envVariable[i]) + 1;
        sp -= sp % 16; // riscv sp must be 16-byte aligned
        copyout(pagetable, sp, envVariable[i], strlen(envVariable[i]) + 1);
        ustack[argc + 2 + i] = sp;
    }
    ustack[argc + 2 + envCount] = 0;
    LOAD_DEBUG("finished push env\n");

    // Generate 16 random bytes for userspace PRNG seeding.
    static u8 k_rand_bytes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    sp -= 32;
    sp -= sp % 16;
    copyout(pagetable, sp, (char *)k_rand_bytes, sizeof(k_rand_bytes));
    u64 u_rand_bytes = sp;
    LOAD_DEBUG("finished push rand bytes\n");

    u64 *elf_info = ustack + (argc + envCount + 3);
// elf_info 应该是用户栈的一部分
#define NEW_AUX_ENT(id, val) \
    do {                     \
        *elf_info++ = id;    \
        *elf_info++ = val;   \
    } while (0)

#define ELF_HWCAP 0 /* we assume this risc-v cpu have no extensions */
#define ELF_EXEC_PAGESIZE PAGE_SIZE

/* these function always return 0 */
#define from_kuid_munged(x, y) (0)
#define from_kgid_munged(x, y) (0)

    u64 secureexec = 0;                        // the default value is 1, 但是我不清楚哪些情况会把它变成 0
    NEW_AUX_ENT(AT_HWCAP, ELF_HWCAP);          // CPU 的 extension 信息
    NEW_AUX_ENT(AT_PAGESZ, ELF_EXEC_PAGESIZE); // PAGE_SIZE
    NEW_AUX_ENT(AT_PHDR, phdrAddr);            // Phdr * phdr_addr; 指向用户态。
    NEW_AUX_ENT(AT_PHENT, sizeof(Phdr));       // 每个 Phdr 的大小
    NEW_AUX_ENT(AT_PHNUM, elfHeader->phnum);   // phdr的数量
    NEW_AUX_ENT(AT_BASE, interpLoadAddr);
    NEW_AUX_ENT(AT_ENTRY, elfHeader->entry + interpOffset);            // 源程序的入口
    NEW_AUX_ENT(AT_UID, from_kuid_munged(cred->user_ns, cred->uid));   // 0
    NEW_AUX_ENT(AT_EUID, from_kuid_munged(cred->user_ns, cred->euid)); // 0
    NEW_AUX_ENT(AT_GID, from_kgid_munged(cred->user_ns, cred->gid));   // 0
    NEW_AUX_ENT(AT_EGID, from_kgid_munged(cred->user_ns, cred->egid)); // 0
    NEW_AUX_ENT(AT_SECURE, secureexec);                                // 安全，默认1。该模式下不会启用LD_LIBRARY_PATH等
    NEW_AUX_ENT(AT_RANDOM, u_rand_bytes);                              // 16byte随机数的地址。
#ifdef ELF_HWCAP2
    NEW_AUX_ENT(AT_HWCAP2, ELF_HWCAP2);
#endif
    NEW_AUX_ENT(AT_EXECFN, ustack[1] /*用户态地址*/); /* 传递给动态连接器该程序的名称 */
                                                      /* And advance past the AT_NULL entry.  */
    NEW_AUX_ENT(0, 0);
    // auxiliary 辅助数组
#undef NEW_AUX_ENT
    u64 copy_size = (elf_info - ustack) * sizeof(u64);
    sp -= copy_size; /* now elf_info is the stack top */
    sp -= sp % 16;
    copyout(pagetable, sp, (char *)ustack, copy_size);
    LOAD_DEBUG("finished push auxv\n");

    // arguments to user main(argc, argv)
    // argc is returned via the system call return
    // value, which goes in a0.
    getHartTrapFrame()->a1 = sp;
    LOAD_DEBUG("init user stack finished.\n");
    return sp;
}

/**
 * @brief 初始化用户虚拟内存空间，包括映射 trampoline, trapframe, signalTrampoline, user stack 四个部分
 *
 */
static void initUserMemory()
{
    u64 *pagetable = myProcess()->pgdir;
    extern char trampoline[];
    extern char trapframe[];
    extern char signalTrampoline[];
    pageMap(pagetable, TRAMPOLINE, (u64)trampoline, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    pageMap(pagetable, TRAPFRAME, (u64)trapframe, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT);
    pageMap(pagetable, SIGNAL_TRAMPOLINE, (u64)signalTrampoline, PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT | PTE_USER_BIT);
    Page *page;
    int r = pageAlloc(&page);
    if (r < 0)
    {
        panic("setup stack alloc error\n");
        return;
    }
    pageInsert(pagetable, USER_STACK_TOP - PAGE_SIZE, page,
               PTE_USER_BIT | PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT); // We must alloc the stack
    LOAD_DEBUG("init user memory finished.\n");
}

u64 exec(char *path, char **argv)
{
    LOAD_DEBUG("load %s begin.\n", path);
    // 分配一个新的页字典，因为后续有需要的，所以需要先分配
    u64 *oldPageTable = execAllocPgdir();
    initUserMemory();
    // 根据 path 查询文件系统 meta
    DirMeta *srcMeta;
    processSegmentMapFree(myProcess());
    Process *p = myProcess();
    p->brkHeapTop = USER_BRK_HEAP_BOTTOM;
    p->mmapHeapTop = USER_MMAP_HEAP_BOTTOM;
    p->shmHeapTop = USER_SHM_HEAP_BOTTOM;
    if ((srcMeta = metaName(AT_FDCWD, path, true)) == 0)
    {
        printk("find file error, path: %s\n", path);
        return -1;
    }

    // 读取 elf 头部（也有可能是 shell script 的头部）
    Ehdr elfHeader;
    metaRead(srcMeta, false, (u64)&elfHeader, 0, sizeof(elfHeader));
    // 如果是脚本，那么就按照脚本形式加载
    if (isScriptFormat((u8 *)&elfHeader))
    {
        return loadScript(srcMeta, path, argv);
    }
    // TODO: 暂时对应那种 ./script.sh 的形式
    if (!isElfFormat((u8 *)&elfHeader))
    {
        char *scriptArgv[4] = {"./busybox", "sh"};
        scriptArgv[2] = path;
        scriptArgv[3] = NULL;
        return exec("/busybox", scriptArgv);
    }
    // 如果不是脚本，那么就是 elf 文件
    assert(isElfFormat((u8 *)&elfHeader));

    // 遍历 segments 完成多项任务
    u64 phAddr = 0;
    DirMeta *interpMeta = NULL;
    u64 elfEntry;
    // 因为动态链接器也是动态对象，所以他的代码都是相对的，从 0 开始，我们需要将其加上一个固定的偏移
    // 也就是从 0x10000 开始，其实其他的估计也行，只需要不和当前程序的其他静态段重合即可
    u64 interpOffset = 0;
    if (elfHeader.type == ET_DYN)
    {
        interpOffset += 0x10000;
    }
    segmentTraverse(srcMeta, &elfHeader, interpOffset, &phAddr, &interpMeta);

    // 加载解释器，并确定加载程序的入口
    u64 interpLoadAddr = 0; // 动态解释器的加载首地址
    if (interpMeta != NULL)
    {
        u64 load_bias = 0; // load_bias only work when object is ET_DYN, such as ./ld.so
        Ehdr interpElfHeader;
        metaRead(interpMeta, false, (u64)&interpElfHeader, 0, sizeof(Ehdr));
        // 计算出整个 phdr 表的大小
        u64 interpPhdrsSize = sizeof(Phdr) * interpElfHeader.phnum;
        // 分配一页，读取整个 phdr 表
        Phdr *interpPhdrData = kmalloc(interpPhdrsSize);
        // 读出 interpreter 的 program table
        metaRead(interpMeta, false, (u64)interpPhdrData, interpElfHeader.phoff, interpPhdrsSize);
        LOAD_DEBUG("begin load interperter.\n");
        interpLoadAddr = load_elf_interp(myProcess()->pgdir, &interpElfHeader, interpMeta, load_bias,
                                         interpPhdrData);
        elfEntry = interpLoadAddr;
        LOAD_DEBUG("interpLoadAddr is 0x%lx\n", interpLoadAddr);
        elfEntry += interpElfHeader.entry;
        LOAD_DEBUG("relative offset entry is 0x%0x\n", interpElfHeader.entry);
        LOAD_DEBUG("interp elf entry is 0x%lx\n", elfEntry);
        kfree((char *)interpPhdrData);
    }
    else
    {
        elfEntry = elfHeader.entry + interpOffset;
    }

    // 用户栈的构造
    u64 sp = initUserStack(argv, phAddr, &elfHeader, interpLoadAddr, interpOffset);
    // 其他的收尾工作
    myThread()->clearChildTid = 0;
    getHartTrapFrame()->epc = elfEntry;
    LOAD_DEBUG("elf entry is 0x%lx\n", elfEntry);
    getHartTrapFrame()->sp = sp; // initial stack pointer
    LOAD_DEBUG("stack pointer is 0x%lx\n", sp);

    p->execFile = srcMeta;
    pgdirFree(oldPageTable);
    asm volatile("fence.i");
    return 0;
}