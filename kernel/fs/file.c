#include <file.h>
#include <pipe.h>
#include <process.h>
#include <linux_struct.h>
#include <fat.h>
#include <string.h>
#include <mmap.h>

struct devsw devsw[NDEV];
struct
{
    // struct Spinlock lock;
    File file[NFILE];
} ftable;

/**
 * @brief 初始化的过程就是将文件列表清零的过程
 *
 */
void fileinit()
{
    File *f;
    for (f = ftable.file; f < ftable.file + NFILE; f++)
    {
        memset(f, 0, sizeof(File));
    }
}

/**
 * @brief 分配一个文件结构体的本质是遍历文件数组，找到一个空的文件结构体
 *
 * @return File* 分配好的 file
 */
File *filealloc()
{
    File *f;

    for (f = ftable.file; f < ftable.file + NFILE; f++)
    {
        if (f->ref == 0)
        {
            f->ref = 1;
            return f;
        }
    }
    return NULL;
}

/**
 * @brief 提高 file 的引用数
 *
 * @param f 文件
 */
void filedup(File *f)
{
    if (f->ref < 1)
        panic("filedup");
    f->ref++;
}

/**
 * @brief 降低文件的引用次数，当引用次数降为 0 的时候，关闭文件
 *
 * @param f 文件
 */
void fileclose(File *f)
{
    File ff;

    if (f->ref < 1)
        panic("fileclose");
    if (--f->ref > 0)
    {
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;

    if (ff.type == FD_PIPE)
    {
        pipeClose(ff.pipe, ff.writable);
    }
}

/**
 * @brief 获得文件信息 file stat
 *
 * @param f 文件
 * @param addr file stat 的地址
 * @return int 成功为 0
 */
int filestat(File *f, u64 addr)
{
    struct Process *p = myProcess();
    struct kstat st;

    if (f->type == FD_ENTRY)
    {
        metaStat(f->meta, &st);
        if (copyout(p->pgdir, addr, (char *)&st, sizeof(st)) < 0)
            return -1;
        return 0;
    }
    return -1;
}

/**
 * @brief 读入文件，对于不同文件采用不同方式
 *
 * @param f 文件
 * @param isUser 是否是用户态
 * @param addr 读入地址
 * @param n 读入长度
 * @return int
 */
int fileread(File *f, bool isUser, u64 addr, int n)
{
    int r = 0;

    if (f->readable == 0)
        return -1;

    switch (f->type)
    {
    case FD_PIPE:
        r = pipeRead(f->pipe, isUser, addr, n);
        break;
    case FD_DEVICE:
        if (f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
            return -1;
        r = devsw[f->major].read(isUser, addr, 0, n);
        break;
    case FD_ENTRY:
        if ((r = metaRead(f->meta, isUser, addr, f->off, n)) > 0)
            f->off += r;
        break;
    default:
        panic("fileread");
    }

    return r;
}

/**
 * @brief 写入文件，对于不同文件采用不同方式
 *
 * @param f 文件
 * @param isUser 是否是用户态
 * @param addr 写入地址
 * @param n 写入长度
 * @return int
 */
int filewrite(File *f, bool isUser, u64 addr, int n)
{
    int ret = 0;

    if (f->writable == 0)
        return -1;

    if (f->type == FD_PIPE)
    {
        ret = pipeWrite(f->pipe, isUser, addr, n);
    }
    else if (f->type == FD_DEVICE)
    {
        if (f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
            return -1;
        ret = devsw[f->major].write(isUser, addr, 0, n);
    }
    else if (f->type == FD_ENTRY)
    {
        if (metaWrite(f->meta, isUser, addr, f->off, n) == n)
        {
            ret = n;
            f->off += n;
        }
        else
        {
            ret = -1;
        }
    }
    else
    {
        panic("filewrite");
    }

    return ret;
}

/**
 * @brief 利用递归获得路径
 *
 * @param d
 * @param isUser
 * @param buf
 * @param maxLen
 * @return int
 */
int getAbsolutePath(DirMeta *d, int isUser, u64 buf, int maxLen)
{
    char path[FAT32_MAX_PATH];

    if (d->parent == NULL)
    {
        return either_copyout(isUser, buf, "/", 2);
    }
    char *s = path + FAT32_MAX_PATH - 1;
    *s = '\0';
    while (d->parent)
    {
        int len = strlen(d->filename);
        s -= len;
        if (s <= path || s - path <= FAT32_MAX_PATH - maxLen) // can't reach root "/"
            return -1;
        strncpy(s, d->filename, len);
        *--s = '/';
        d = d->parent;
    }
    return either_copyout(isUser, buf, (void *)s, strlen(s) + 1);
}

/**
 * @brief 将文件中的一部分映射到内存中，文件是广义的
 *
 * @param fd 文件
 * @param start 内存起始地址
 * @param len 内存长度
 * @param perm 权限
 * @param flags 这个函数的一些控制符，似乎是用控制 mmap 的行为的
 * @param off 从文件的 off 开始读
 * @return u64 起始地址
 */
u64 do_mmap(struct File *fd, u64 start, u64 len, int perm, int flags, u64 off)
{
    bool alloc = (start == 0);
    assert(PAGE_OFFSET(start, PAGE_SIZE) == 0);
    Process *p = myProcess();
    if (alloc)
    {
        p->mmapHeapTop = ALIGN_UP(p->mmapHeapTop, PAGE_SIZE);
        start = p->mmapHeapTop;
        p->mmapHeapTop = ALIGN_UP(p->mmapHeapTop + len, PAGE_SIZE);
        assert(p->mmapHeapTop < USER_STACK_BOTTOM);
    }

    u64 addr = start, end = start + len;
    if (flags & MAP_FIXED_BIT)
    {
        assert(start <= p->brkHeapTop);
        p->brkHeapTop = MAX(ALIGN_UP(end, PAGE_SIZE), p->brkHeapTop);
    }

    // 为 [start, start + len] 分配多个物理页
    while (start < end)
    {
        // 先查询页表项
        u64 *pte;
        u64 pa = page2PA(pageLookup(p->pgdir, start, &pte));
        // 需要写时复制
        if (pa > 0 && (*pte & PTE_COW_BIT))
        {
            cowHandler(p->pgdir, start);
            pa = page2PA(pageLookup(p->pgdir, start, &pte));
        }

        // 如果没有查到，那么就分配一页
        if (pa == 0)
        {
            Page *page;
            if (pageAlloc(&page) < 0)
            {
                return -1;
            }
            pageInsert(p->pgdir, start, page, perm | PTE_USER_BIT);
        }
        else
        {
            bzero((void *)pa, MIN(PAGE_SIZE, end - start));
            *pte = PA2PTE(pa) | perm | PTE_USER_BIT | PTE_VALID_BIT;
        }
        start += PAGE_SIZE;
    }

    if (flags & MAP_ANONYMOUS_BIT)
    {
        return addr;
    }
    /* if fd == NULL, we think this is a anonymous map */
    if (fd != NULL)
    {
        fd->off = off;
        if (fileread(fd, true, addr, len) >= 0)
        {
            return addr;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return addr;
    }
}
