#include <tmpfile.h>
#include <mem_layout.h>
#include <string.h>
#include <memory.h>
#include <driver.h>
#include <process.h>
#include <debug.h>
#include <linux_struct.h>

Tmpfile tmpfiles[TMPFILE_COUNT];

/**
 * @brief 获得 tmpfile buffer，也就是一种索引查询
 *
 * @param t 待查询的 tmpfile
 * @return u64 buffer 的首地址
 */
static inline u64 getTmpfileBufferBase(Tmpfile *t)
{
    return TMPFILE_BUFFER_BASE + (((u64)(t - tmpfiles)) << TMPFILE_SHIFT);
}

void tmpfileAlloc(char *name, Tmpfile **t)
{
    for (int i = 0; i < TMPFILE_COUNT; i++)
    {
        if (!tmpfiles[i].used)
        {
            strncpy(tmpfiles[i].filename, name + 5, TMPFILE_NAME_LEN);
            tmpfiles[i].filename[TMPFILE_NAME_LEN] = 0;
            TMPF_DEBUG("tmpfile name is %s\n", tmpfiles[i].filename);
            tmpfiles[i].offset = 0;
            tmpfiles[i].fileSize = 0;
            tmpfiles[i].used = true;
            // TODO: 分配 16 页，所以严格上 tmpfile 只有在 64KB 以下的时候正常工作
            // 可以参考 pipe 的边读写边分配的方式
            for (int j = 0; j < 16; j++)
            {
                Page *page;
                int r;
                if ((r = pageAlloc(&page)) < 0)
                {
                    panic("run out memory for the tmpfile\n");
                }
                u64 base = getTmpfileBufferBase(&tmpfiles[i]);
                extern u64 kernelPageDirectory[];
                pageInsert(kernelPageDirectory, base + (j << PAGE_SHIFT), page, PTE_READ_BIT | PTE_WRITE_BIT);
            }
            *t = &tmpfiles[i];
            return;
        }
    }

    panic("run out tmpfile\n");
}

/**
 * @brief 根据文件名查询对应的 tmpfile
 *
 * @param name 文件名，为 /tmp/tmpfile_xxxx 格式
 * @return Tmpfile* 查找到的文件名。没有找到返回 NULL
 */
Tmpfile *tmpfileName(char *name)
{
    for (int i = 0; i < TMPFILE_COUNT; i++)
    {
        if (tmpfiles[i].used)
        {
            if (strncmp(name + 5, tmpfiles[i].filename, TMPFILE_NAME_LEN) == 0)
            {
                TMPF_DEBUG("find the tmpfile named %s\n", tmpfiles[i].filename);
                return &tmpfiles[i];
            }
        }
    }
    return NULL;
}

void tmpfileClose(Tmpfile *t)
{
    TMPF_DEBUG("close the tmpfile %s\n", t->filename);
    extern u64 kernelPageDirectory[];
    u64 base = getTmpfileBufferBase(t);
    for (int i = 0; i < 16; i++)
    {
        pageRemove(kernelPageDirectory, base + (i << PAGE_SHIFT));
    }
    t->used = false;
}

/**
 * @brief 从 tmpfile 中读取文件
 *
 * @param tmpfile 临时文件
 * @param isUser 读出后是放置到 user 还是 kernel
 * @param dst 目的地址
 * @param n 读取长度
 * @return int 读出内容长度
 */
int tmpfileRead(Tmpfile *tmpfile, bool isUser, u64 dst, u32 n)
{
    TMPF_DEBUG("Read the tmpfile %s, offset: %ld, len: %d\n", tmpfile->filename, tmpfile->offset, n);
    int i;
    char ch;
    u64 *pageTable = myProcess()->pgdir;
    u64 pa = dst;
    // 这里似乎需要为用户考虑用户空间是否充足的问题，可以看做是手写一个循环队列版的 copyout
    if (isUser)
    {
        pa = va2PA(pageTable, dst, NULL);
        if (pa == NULL)
        {
            pa = passiveAlloc(pageTable, dst);
        }
    }

    u8 *data = (u8 *)getTmpfileBufferBase(tmpfile);

    if (tmpfile->offset + n > tmpfile->fileSize)
    {
        n = tmpfile->fileSize - tmpfile->offset;
        TMPF_DEBUG("n has changed to %d\n", n);
    }
    for (i = 0; i < n;)
    {
        // 读出一个字节的内容
        ch = data[(tmpfile->offset++) & (TMPFILE_SIZE - 1)];
        // 写入内容
        *((char *)pa) = ch;
        i++;
        // 突然需要分配地址（也就是到了页的边缘）
        if (isUser && (!((dst + i) & (PAGE_SIZE - 1))))
        {
            pa = va2PA(pageTable, dst + i, NULL);
            if (pa == NULL)
            {
                pa = passiveAlloc(pageTable, dst + i);
            }
        }
        else
        {
            pa++;
        }
    }

    return i;
}

int tmpfileWrite(Tmpfile *tmpfile, bool isUser, u64 addr, int n)
{
    TMPF_DEBUG("Write the tmpfile %s, offset: %ld, len: %d\n", tmpfile->filename, tmpfile->offset, n);
    int i = 0, cow;

    u64 *pageTable = myProcess()->pgdir;
    u64 pa = addr;
    // 这里的分配似乎是没有意义的？
    if (isUser)
    {
        pa = va2PA(pageTable, addr, &cow);
        if (pa == NULL)
        {
            cow = 0;
            pa = passiveAlloc(pageTable, addr);
        }
        if (cow)
        {
            pa = cowHandler(pageTable, addr);
        }
    }
    u8 *data = (u8 *)getTmpfileBufferBase(tmpfile);
    while (i < n)
    {
        char ch;
        ch = *((char *)pa);
        data[(tmpfile->offset++) & (TMPFILE_SIZE - 1)] = ch;
        i++;
        if (isUser && (!((addr + i) & (PAGE_SIZE - 1))))
        {
            pa = va2PA(pageTable, addr + i, &cow);
            if (pa == NULL)
            {
                cow = 0;
                pa = passiveAlloc(pageTable, addr + i);
            }
            if (cow)
            {
                pa = cowHandler(pageTable, addr);
                // pa = va2PA(pageTable, addr, NULL);
            }
        }
        else
        {
            pa++;
        }
    }
    // 更新 filesize, offset 经历前面的拷贝，已经发生了偏移
    if (tmpfile->offset > tmpfile->fileSize)
    {
        tmpfile->fileSize = tmpfile->offset;
    }
    return i;
}

/**
 * @brief 获得 tmpfile 对应文件的状态
 *
 * @param tmpfile 临时文件
 * @param st 待填写状态
 */
void tmpfileStat(Tmpfile *tmpfile, struct kstat *st)
{
    st->st_dev = 0; // DEV_SD = 0
    st->st_size = tmpfile->fileSize;
    st->st_ino = tmpfile - tmpfiles;
    st->st_mode = REG_TYPE;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0; // What's this?
    st->st_blksize = 512;
    st->st_blocks = st->st_size / st->st_blksize;
    st->st_atime_sec = (((u64)tmpfile->_crt_time_tenth) << 32) + (((u64)tmpfile->_crt_time) << 16) + tmpfile->_crt_date;
    st->st_atime_nsec = 0;
    st->st_mtime_sec = (((u64)tmpfile->_lst_acce_date) << 32) + (((u64)tmpfile->_lst_wrt_time) << 16) + tmpfile->_lst_wrt_date;
    st->st_mtime_nsec = 0;
    st->st_ctime_sec = 0;
    st->st_ctime_nsec = 0;
}

#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
void tmpfileSetTime(Tmpfile *tmpfile, TimeSpec ts[2])
{
    u64 time = r_time();
    TimeSpec now;
    now.second = time / 1000000;
    now.microSecond = time % 1000000 * 1000;
    if (ts[0].microSecond != UTIME_OMIT)
    {
        if (ts[0].microSecond == UTIME_NOW)
        {
            ts[0].second = now.second;
        }
        tmpfile->_crt_date = ts[0].second & ((1 << 16) - 1);
        tmpfile->_crt_time = (ts[0].second >> 16) & ((1 << 16) - 1);
        tmpfile->_crt_time_tenth = (ts[0].second >> 32) & ((1 << 8) - 1);
    }
    if (ts[1].microSecond != UTIME_OMIT)
    {
        if (ts[1].microSecond == UTIME_NOW)
        {
            ts[1].second = now.second;
        }
        tmpfile->_lst_wrt_date = ts[1].second & ((1 << 16) - 1);
        tmpfile->_lst_wrt_time = (ts[1].second >> 16) & ((1 << 16) - 1);
        tmpfile->_lst_acce_date = (ts[1].second >> 32) & ((1 << 16) - 1);
    }
}