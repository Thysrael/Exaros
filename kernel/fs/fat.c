#include <types.h>
#include <fs.h>
#include <bio.h>
#include <string.h>
#include <fat.h>
#include <process.h>
#include <linux_struct.h>
#include <driver.h>

extern Inode inodes[];
extern DirMeta dirMetas[];

/**
 * @brief 计算簇号为 cluster 的簇的第一个 sector 的编号
 * 需要注意 cluster 从 2 开始编号
 *
 * @param fs 文件系统
 * @param cluster 簇号
 * @return u32 sector 编号
 */
static inline u32 firstSecOfClus(FileSystem *fs, u32 cluster)
{
    return ((cluster - 2) * fs->superBlock.BPB.secPerClus) + fs->superBlock.firstDataSec;
}

/**
 * @brief 计算簇号为 cluster 的簇在 FAT 表中对应表项所在的扇区号
 *
 * @param fs 文件系统
 * @param cluster 簇号
 * @param fatNum fat 表的编号，一般是 1
 * @return u32 对应扇区号
 */
static inline u32 fatSecOfClus(FileSystem *fs, u32 cluster, u8 fatNum)
{
    return fs->superBlock.BPB.rsvdSecCnt + (cluster << 2) / fs->superBlock.BPB.bytsPerSec + fs->superBlock.BPB.FATsz * (fatNum - 1);
}

/**
 * @brief 计算簇号为 cluster 的簇在 FAT 表中对应表项所在的扇区偏移
 *
 * @param fs 文件系统
 * @param cluster 簇号
 * @return u32 扇区内偏移
 */
static inline u32 fatOffsetOfClus(FileSystem *fs, u32 cluster)
{
    return (cluster << 2) % fs->superBlock.BPB.bytsPerSec;
}

/**
 * @brief 根据文件内扇区编号和文件 meta 计算文件系统扇区编号
 *
 * @param meta 文件 meta
 * @param dataSectorNum  文件内扇区编号
 * @return int 文件系统扇区编号
 */
int getSectorNumber(DirMeta *meta, int dataSectorNum)
{
    // sector size == (1 << 9)
    int offset = (dataSectorNum << 9);
    if (offset > meta->fileSize)
    {
        return -1;
    }

    FileSystem *fs = meta->fileSystem;
    relocClus(fs, meta, offset, 0);

    return firstSecOfClus(fs, meta->curClus) + offset % fs->superBlock.bytsPerClus / fs->superBlock.BPB.bytsPerSec;
}

/**
 * @brief 读取 cluster 在 FAT 表中的条目项内容
 *
 * @param fs 文件系统
 * @param cluster 簇号
 * @return u32 条目项内容，一般是下一个簇号
 */
static u32 readFat(FileSystem *fs, u32 cluster)
{
    // cluster 过大
    if (cluster >= FAT32_EOC)
    {
        return cluster;
    }
    // cluster 过大
    if (cluster > fs->superBlock.dataClusCnt + 1)
    {
        return 0;
    }
    // 计算对应的 fat 表的 sector
    u32 fatSec = fatSecOfClus(fs, cluster, 1);
    // TODO:here should be a cache layer for FAT table, but not implemented yet.
    // 读入这个 FAT 表扇区
    Buf *b = fs->read(fs, fatSec);
    // 通过偏移量读取出 cluster 对应的条目项，里面的内容是下一个簇号
    u32 nextClus = *(u32 *)(b->data + fatOffsetOfClus(fs, cluster));
    brelse(b);
    return nextClus;
}

/**
 * @brief 写入 content 到 cluster 在 FAT 表中的条目项
 *
 * @param fs 文件系统
 * @param cluster 簇号
 * @param content 写入内容
 * @return int 0 为成功，-1 为失败
 */
static int writeFat(FileSystem *fs, u32 cluster, u32 content)
{
    if (cluster > fs->superBlock.dataClusCnt + 1)
    {
        return -1;
    }
    u32 fatSec = fatSecOfClus(fs, cluster, 1);
    Buf *b = fs->read(fs, fatSec);
    u32 off = fatOffsetOfClus(fs, cluster);
    *(u32 *)(b->data + off) = content;
    bwrite(b);
    brelse(b);
    return 0;
}

/**
 * @brief 清空 cluster 簇
 *
 * @param fs 文件系统
 * @param cluster 簇号
 */
static void zeroClus(FileSystem *fs, u32 cluster)
{
    u32 sec = firstSecOfClus(fs, cluster);
    Buf *b;
    // 逐个将簇内的每个扇区清空
    for (int i = 0; i < fs->superBlock.BPB.secPerClus; i++)
    {
        b = fs->read(fs, sec++);
        memset(b->data, 0, BUFFER_SIZE);
        bwrite(b);
        brelse(b);
    }
}

/**
 * @brief 分配一个空闲簇，原始 FAT 表中利用遍历找到空闲簇，此处用 bitmap 对空闲簇进行记录
 *
 * @param fs 文件系统
 * @return u32 空闲簇号
 */
static u32 allocClus(FileSystem *fs)
{
    // 获得位图地址
    u64 *clusterBitmap = (u64 *)getFileSystemClusterBitmap(fs);
    // 计算 fs 中簇的总数
    int totalClusterNumber = fs->superBlock.BPB.FATsz * fs->superBlock.BPB.bytsPerSec / sizeof(u32);
    for (int i = 0; i < (totalClusterNumber / 64); i++)
    {
        if (~clusterBitmap[i])
        {
            int bit = LOW_BIT64(~clusterBitmap[i]);
            clusterBitmap[i] |= (1UL << bit);
            // 找到没有被分配的簇
            int cluster = (i << 6) | bit;
            Buf *b;
            u32 entryPerSec = fs->superBlock.BPB.bytsPerSec / sizeof(u32);
            // 计算所在的扇区号（FAT 表区域）
            u32 sec = fs->superBlock.BPB.rsvdSecCnt + cluster / entryPerSec;
            b = fs->read(fs, sec);
            // 更新 FAT 表
            int j = cluster % entryPerSec;
            ((u32 *)(b->data))[j] = FAT32_EOC + 7;
            bwrite(b);
            brelse(b);
            zeroClus(fs, cluster);
            return cluster;
        }
    }
    panic("");
}

/**
 * @brief 释放指定簇号中的内容，需要更新 FAT 表和 cluster bitmap
 *
 * @param fs 文件系统
 * @param cluster 簇号
 */
static void freeClus(FileSystem *fs, u32 cluster)
{
    writeFat(fs, cluster, 0);
    u64 *clusterBitmap = (u64 *)getFileSystemClusterBitmap(fs);
    clusterBitmap[cluster >> 6] &= ~(1UL << (cluster & 63));
}

/**
 * @brief 读写某个簇的 `off` 偏移往后长度 `n` 的内容。本质是一个 sector 一个 sector 的去读
 *
 * @param fs 文件系统
 * @param cluster 簇号
 * @param write 如果 `write` 为真，则为写簇，否则为读簇
 * @param user 如果 `user` 为 true 则为读写用户态，否则是读写内核态。
 * @param data 读写的内存地址
 * @param off 偏移量
 * @param n 内容长度
 * @return uint 读写内容的长度
 */
u32 rwClus(FileSystem *fs, u32 cluster, int write, int user, u64 data, u32 off, u32 n)
{
    // 检查数据偏移量和数据长度是否超出了簇的范围
    if (off + n > fs->superBlock.bytsPerClus)
        panic("offset out of range");
    // tot 是已经处理完的字节数，m 是每次处理的大小
    u32 tot, m;
    Buf *bp;
    // 获得当前 offset 对应的 sector
    u32 sec = firstSecOfClus(fs, cluster) + off / fs->superBlock.BPB.bytsPerSec;
    // 将 offset 处理成 sector 内偏移
    off = off % fs->superBlock.BPB.bytsPerSec;

    int bad = 0;
    for (tot = 0; tot < n; tot += m, off += m, data += m, sec++)
    {
        // 读出这个 sector
        bp = fs->read(fs, sec);
        m = BUFFER_SIZE - off % BUFFER_SIZE;
        if (n - tot < m)
        {
            m = n - tot;
        }
        if (write)
        {
            // 将 bp-> data 写成 data 为起始地址的内容
            if ((bad = either_copyin(bp->data + (off % BUFFER_SIZE), user, data, m))
                != -1)
            {
                bwrite(bp);
            }
        }
        else
        {
            bad = either_copyout(user, data, bp->data + (off % BUFFER_SIZE), m);
        }
        brelse(bp);
        if (bad == -1)
        {
            break;
        }
    }

    return tot;
}

/**
 * 根据给定的偏移量和文件目录项，重新计算当前簇号（meta->curClus）和簇内偏移量
 * @param   meta        给定的文件目录项，修改它的 curClus
 * @param   off         the offset from the beginning of the relative file
 * @param   alloc       whether alloc new cluster when meeting end of FAT chains
 * @return              相对于当前簇的偏移量
 */
int relocClus(FileSystem *fs, DirMeta *meta, u32 off, int alloc)
{
    assert(meta->firstClus != 0);
    assert(meta->inodeMaxCluster > 0);
    assert(meta->firstClus == meta->inode.item[0]);
    // 计算偏移量 `off` 所在的簇号
    int clusNum = off / fs->superBlock.bytsPerClus;
    // 计算 offset 的簇内偏移
    int ret = off % fs->superBlock.bytsPerClus;

    // 当 max 大于 clusNum 时，说明已经缓存，可以立刻更新并返回
    if (clusNum < meta->inodeMaxCluster)
    {
        metaFindInode(meta, clusNum);
        return ret;
    }

    // 如果没有被缓存，那么就需要先更新 curClus 为最后一个然后进行迭代
    if (meta->inodeMaxCluster > 0)
    {
        metaFindInode(meta, meta->inodeMaxCluster - 1);
    }
    // 迭代直到满足要求
    while (clusNum > meta->clusCnt)
    {
        int clus = readFat(fs, meta->curClus);
        if (clus >= FAT32_EOC)
        {
            if (alloc)
            {
                clus = allocClus(fs);
                writeFat(fs, meta->curClus, clus);
            }
            else
            {
                meta->curClus = meta->firstClus;
                meta->clusCnt = 0;
                return -1;
            }
        }
        meta->curClus = clus;
        meta->clusCnt++;

        // 分配结束后，进行 inode 的更新
        u32 pos = meta->clusCnt;
        assert(pos == meta->inodeMaxCluster);
        meta->inodeMaxCluster++;
        if (pos < INODE_SECOND_LEVEL_BOTTOM)
        {
            meta->inode.item[pos] = clus;
            continue;
        }
        if (pos < INODE_THIRD_LEVEL_BOTTOM)
        {
            int idx1 = INODE_SECOND_ITEM_BASE + (pos - INODE_SECOND_LEVEL_BOTTOM) / INODE_ITEM_NUM;
            int idx2 = (pos - INODE_SECOND_LEVEL_BOTTOM) % INODE_ITEM_NUM;
            if (idx2 == 0)
            {
                meta->inode.item[idx1] = inodeAlloc();
            }
            inodes[meta->inode.item[idx1]].item[idx2] = clus;
            continue;
        }
        if (pos < INODE_THIRD_LEVEL_TOP)
        {
            int idx1 = INODE_THIRD_ITEM_BASE + (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM / INODE_ITEM_NUM;
            int idx2 = (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM % INODE_ITEM_NUM;
            int idx3 = (pos - INODE_THIRD_LEVEL_BOTTOM) % INODE_ITEM_NUM;
            if (idx3 == 0)
            {
                if (idx2 == 0)
                {
                    meta->inode.item[idx1] = inodeAlloc();
                }
                inodes[meta->inode.item[idx1]].item[idx2] = inodeAlloc();
            }
            inodes[inodes[meta->inode.item[idx1]].item[idx2]].item[idx3] = clus;
            continue;
        }
        panic("");
    }
    return ret;
}

/**
 * @brief 读取文件内容，文件是通过 meta 来指定的
 *
 * @param meta 文件对应的目录项
 * @param userDst 是否是用户地址
 * @param dst 读出的内容存放的地址
 * @param off 读入时的偏移量
 * @param n 内容长度
 * @return int 读出内容长度
 */
int metaRead(DirMeta *meta, int userDst, u64 dst, u32 off, u32 n)
{
    // 对于两种特殊的设备
    if (meta->dev == ZERO)
    {
        if (!either_memset(userDst, dst, 0, n))
        {
            return n;
        }
        panic("error!\n");
    }
    else if (meta->dev == OSRELEASE)
    {
        char osrelease[] = "10.2.0";
        if (!either_copyout(userDst, dst, (char *)osrelease, sizeof(osrelease)))
        {
            return sizeof(osrelease);
        }
        panic("error!\n");
    }

    // 检查偏移是否符合要求
    if (off > meta->fileSize || off + n < off || (meta->attribute & ATTR_DIRECTORY))
    {
        return 0;
    }
    if (off + n > meta->fileSize)
    {
        n = meta->fileSize - off;
    }

    FileSystem *fs = meta->fileSystem;
    u32 tot, m;
    // 一个簇一个簇的读出
    for (tot = 0; meta->curClus < FAT32_EOC && tot < n;
         tot += m, off += m, dst += m)
    {
        relocClus(fs, meta, off, 0);
        m = fs->superBlock.bytsPerClus - off % fs->superBlock.bytsPerClus;
        if (n - tot < m)
        {
            m = n - tot;
        }
        if (rwClus(fs, meta->curClus, 0, userDst, dst,
                   off % fs->superBlock.bytsPerClus, m)
            != m)
        {
            break;
        }
    }
    return tot;
}

/**
 * @brief 写入文件内容，文件是通过 meta 来指定的
 *
 * @param meta 文件对应的目录项
 * @param userSrc 是否是用户地址
 * @param dst 读出的内容存放的地址
 * @param off 读入时的偏移量
 * @param n 内容长度
 * @return int 读出内容长度
 */
int metaWrite(DirMeta *meta, int userSrc, u64 src, u32 off, u32 n)
{
    if (meta->dev == NONE)
    {
        return n;
    }
    if (off + n < off || (u64)off + n > 0xffffffff || (meta->attribute & ATTR_READ_ONLY))
    {
        return -1;
    }
    FileSystem *fs = meta->fileSystem;
    if (meta->firstClus == 0)
    {
        meta->curClus = meta->firstClus = meta->inode.item[0] = allocClus(fs);
        meta->inodeMaxCluster = 1;
        meta->clusCnt = 0;
    }
    u32 tot, m;
    for (tot = 0; tot < n; tot += m, off += m, src += m)
    {
        relocClus(fs, meta, off, 1);
        m = fs->superBlock.bytsPerClus - off % fs->superBlock.bytsPerClus;
        if (n - tot < m)
        {
            m = n - tot;
        }
        if (rwClus(fs, meta->curClus, 1, userSrc, src,
                   off % fs->superBlock.bytsPerClus, m)
            != m)
        {
            break;
        }
    }
    if (n > 0)
    {
        if (off > meta->fileSize)
        {
            meta->fileSize = off;
        }
    }
    return tot;
}

/**
 * @brief 给定目录的 DirMeta 和文件名，查找目录下文件名对应的 DirMeta
 *
 * @param parentDir 查找目录
 * @param filename 文件名
 * @return DirMeta* 文件 meta
 */
static DirMeta *dirlookup(DirMeta *parentDir, char *filename)
{
    // 如果不是目录
    if (!(parentDir->attribute & ATTR_DIRECTORY))
        panic("dirlookup not DIR");
    // 如果文件是当前目录
    if (strncmp(filename, ".", FAT32_MAX_FILENAME) == 0)
    {
        return parentDir;
    }
    // 如果文件是上级目录
    else if (strncmp(filename, "..", FAT32_MAX_FILENAME) == 0)
    {
        // 如果是根目录，则返回根目录
        if (parentDir == &parentDir->fileSystem->root)
        {
            return &parentDir->fileSystem->root;
        }
        // 返回上级目录
        return parentDir->parent;
    }
    // 遍历所有的孩子节点，找到与文件名相同的文件
    for (DirMeta *fileMeta = parentDir->firstChild; fileMeta; fileMeta = fileMeta->nextBrother)
    {
        if (strncmp(fileMeta->filename, filename, FAT32_MAX_FILENAME) == 0)
        {
            return fileMeta;
        }
    }
    return NULL;
}

/**
 * @brief 在指定目录下创建具有指定属性和文件名的文件
 *
 * @param parent 目录 meta
 * @param name 文件名
 * @param attr 文件属性
 * @return DirMeta* 文件 meta
 */
DirMeta *metaAlloc(DirMeta *parent, char *name, int attr)
{
    if (!(parent->attribute & ATTR_DIRECTORY))
    {
        panic("metaAlloc not dir");
    }

    DirMeta *child;
    // child exists
    if ((child = dirlookup(parent, name)) != NULL)
    {
        return child;
    }

    dirMetaAlloc(&child);
    // 处理这个文件是否是链接
    if (attr == ATTR_LINK)
    {
        child->attribute = 0;
        child->reserve = DT_LNK;
    }
    else
    {
        child->attribute = attr;
        child->reserve = 0;
    }

    child->fileSize = 0;
    child->firstClus = 0;
    metaFreeInode(child);
    child->parent = parent;
    // 头插法
    child->nextBrother = parent->firstChild;
    parent->firstChild = child;
    child->clusCnt = 0;
    child->curClus = 0;
    child->fileSystem = parent->fileSystem;
    strncpy(child->filename, name, FAT32_MAX_FILENAME);
    child->filename[FAT32_MAX_FILENAME] = '\0';
    FileSystem *fs = child->fileSystem;
    // generate "." and ".." for meta
    if (attr == ATTR_DIRECTORY)
    {
        child->attribute |= ATTR_DIRECTORY;
        child->curClus = child->firstClus = child->inode.item[0] = allocClus(fs);
        child->inodeMaxCluster = 1;
    }
    else
    {
        child->attribute |= ATTR_ARCHIVE;
    }
    return child;
}

/**
 * @brief 在 path 路径下创建一个新的文件，并返回对应的 meta
 *
 * @param fd 起辅助作用的 fd
 * @param path 路径
 * @param type 文件类型
 * @param mode 模式
 * @return DirMeta* 对应的 meta
 */
DirMeta *metaCreate(int fd, char *path, short type, int mode)
{
    DirMeta *fileMeta, *parentDir;
    char name[FAT32_MAX_FILENAME + 1];

    if ((parentDir = metaNameDir(fd, path, name)) == NULL)
    {
        return NULL;
    }

    // 虚拟文件权限 mode 转 fat 格式的权限 mode
    if (type == T_DIR)
    {
        mode = ATTR_DIRECTORY;
    }
    else if (type == T_LINK)
    {
        mode = ATTR_LINK;
    }
    else if (type == T_CHAR)
    {
        mode = ATTR_CHARACTER_DEVICE;
    }
    else if (mode & O_RDONLY)
    {
        mode = ATTR_READ_ONLY;
    }
    else
    {
        mode = 0;
    }

    if ((fileMeta = metaAlloc(parentDir, name, mode)) == NULL)
    {
        return NULL;
    }

    return fileMeta;
}

/**
 * @brief 清空 meta 对应的文件
 *
 * @param meta 需要清空文件的 meta
 */
void metaTrunc(DirMeta *meta)
{
    FileSystem *fs = meta->fileSystem;
    // 逐个簇清空
    for (u32 clus = meta->firstClus; clus >= 2 && clus < FAT32_EOC;)
    {
        u32 next = readFat(fs, clus);
        freeClus(fs, clus);
        clus = next;
    }
    meta->fileSize = 0;
    meta->firstClus = 0;
    metaFreeInode(meta);
}

/**
 * @brief 将 meta 对应的文件移除，用的是链表法
 *
 * @param meta 待移除的文件
 */
void metaRemove(DirMeta *meta)
{
    DirMeta *i = meta->parent->firstChild;
    if (i == meta)
    {
        meta->parent->firstChild = meta->nextBrother;
    }
    else
    {
        for (; i->nextBrother; i = i->nextBrother)
        {
            if (i->nextBrother == meta)
            {
                i->nextBrother = meta->nextBrother;
                break;
            }
        }
    }
    dirMetaFree(meta);
}

/**
 * @brief 获得 meta 对应文件的状态
 *
 * @param meta 文件 meta
 * @param st 待填写状态
 */
void metaStat(DirMeta *meta, struct kstat *st)
{
    st->st_dev = meta->dev;
    st->st_size = meta->fileSize;
    st->st_ino = (meta - dirMetas);
    st->st_mode = (meta->attribute & ATTR_DIRECTORY        ? DIR_TYPE :
                   meta->attribute & ATTR_CHARACTER_DEVICE ? CHR_TYPE :
                                                             REG_TYPE);
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0; // What's this?
    st->st_blksize = meta->fileSystem->superBlock.BPB.bytsPerSec;
    st->st_blocks = st->st_size / st->st_blksize;

    if (meta->parent != NULL)
    {
        u32 entcnt = 0;
        FileSystem *fs = meta->fileSystem;
        u32 off = relocClus(fs, meta->parent, meta->off, 0);
        rwClus(fs, meta->parent->curClus, 0, 0, (u64)&entcnt, off, 1);
        entcnt &= ~LAST_LONG_ENTRY;
        off = relocClus(fs, meta->parent, meta->off + (entcnt << 5), 0);
        Dentry de;
        rwClus(meta->fileSystem, meta->parent->curClus, 0, 0, (u64)&de, off, sizeof(de));
        st->st_atime_sec = (((u64)de.sne._crt_time_tenth) << 32) + (((u64)de.sne._crt_time) << 16) + de.sne._crt_date;
        st->st_atime_nsec = 0;
        st->st_mtime_sec = (((u64)de.sne._lst_acce_date) << 32) + (((u64)de.sne._lst_wrt_time) << 16) + de.sne._lst_wrt_date;
        st->st_mtime_nsec = 0;
        st->st_ctime_sec = 0;
        st->st_ctime_nsec = 0;
    }
    else
    {
        st->st_atime_sec = 0;
        st->st_atime_nsec = 0;
        st->st_mtime_sec = 0;
        st->st_mtime_nsec = 0;
        st->st_ctime_sec = 0;
        st->st_ctime_nsec = 0;
    }
}

/**
 * @brief 从一个路径字符串中提取出下一个文件或目录的名称，并将其存储在一个字符数组 name 中
 *
 * @param path 路径
 * @param name 路径上的字段
 * @return char* 指向下一个字段起始位置的指针
 */
static char *skipelem(char *path, char *name)
{
    // 去掉分隔符？
    while (*path == '/')
    {
        path++;
    }
    if (*path == 0)
    {
        return NULL;
    }
    char *s = path;
    // 找到下一个分隔符
    while (*path != '/' && *path != 0)
    {
        path++;
    }
    // 计算名称长度
    int len = path - s;
    if (len > FAT32_MAX_FILENAME)
    {
        len = FAT32_MAX_FILENAME;
    }
    // 结尾为 0
    name[len] = 0;
    // 字符拷贝
    memmove(name, s, len);
    // 返回下一个名称的起始位置
    while (*path == '/')
    {
        path++;
    }
    return path;
}

/**
 * @brief 找到链接文件的 target meta
 *
 * @param link 链接 meta
 * @return DirMeta* 目标 meta
 */
static DirMeta *jumpToLinkDirMeta(DirMeta *link)
{
    char buf[FAT32_MAX_FILENAME];
    // 迭代直至找到非链接文件
    while (link && link->reserve == DT_LNK)
    {
        metaRead(link, 0, (u64)buf, 0, FAT32_MAX_FILENAME);
        link = metaName(AT_FDCWD, buf, true);
    }
    assert(link != NULL);
    return link;
}

/**
 * @brief 可能是根据路径查询出对应的目录条目
 *
 * @param fd 文件描述符（只是为了只是是否是相对路径）
 * @param path 文件对应的路径，可能是相对路径，也可能是绝对路径
 * @param parent 两种模式，可能一种是返回目录的 meta，一种是文件的 meta
 * @param name 只是一个临时数组，用于存储每一字段的值
 * @param jump 对于链接文件是否跳转
 * @return struct DirMeta* 查找到的 meta
 */
static DirMeta *lookupPath(int fd, char *path, int parent, char *name, bool jump)
{
    DirMeta *cur, *next;
    // 如果 path 并不是根目录且不是当前目录，则应该是打开的某个目录文件
    if (*path != '/' && fd != AT_FDCWD && fd >= 0 && fd < NOFILE)
    {
        if (myProcess()->ofile[fd] == 0)
        {
            return NULL;
        }
        cur = myProcess()->ofile[fd]->meta;
    }
    // 是绝对路径，则 cur 是根目录
    else if (*path == '/')
    {
        extern FileSystem *rootFileSystem;
        cur = &rootFileSystem->root;
    }
    // 是相对路径，cur 是当前路径
    else if (*path != '\0' && fd == AT_FDCWD)
    {
        cur = myProcess()->cwd;
    }
    else
    {
        return NULL;
    }

    while ((path = skipelem(path, name)) != 0)
    {
        cur = jumpToLinkDirMeta(cur);
        if (!(cur->attribute & ATTR_DIRECTORY))
        {
            return NULL;
        }
        // 这里更新了 cur 为挂载 meta，但是我没有弄明白为啥
        if (cur->head != NULL)
        {
            DirMeta *mountDirMeta = &cur->head->root;
            cur = mountDirMeta;
        }
        // 搜完了？奥，可能是搜到目录就可以停止了
        if (parent && *path == '\0')
        {
            return cur;
        }
        // 这里更新 cur，通过向下一级目录遍历的方式
        if ((next = dirlookup(cur, name)) == 0)
        {
            return NULL;
        }
        cur = next;
    }

    if (jump)
    {
        cur = jumpToLinkDirMeta(cur);
    }
    // 异常情况，parent 正确返回位置在上面
    if (parent)
    {
        return NULL;
    }
    return cur;
}

/**
 * @brief 根据 fs path 来查询对应的文件 meta
 *
 * @param fd 需要用到的文件描述符
 * @param path 待查找的文件的路径
 * @param jump 对于查找到的链接文件，是否找到其目标文件
 * @return DirMeta* 查找到的文件 meta
 */
DirMeta *metaName(int fd, char *path, bool jump)
{
    char name[FAT32_MAX_FILENAME + 1];
    return lookupPath(fd, path, 0, name, jump);
}

/**
 * @brief 根据 fs path 来查询对应的目录 meta
 *
 * @param fd 需要用到的文件描述符
 * @param path 路径
 * @param name 文件名
 * @return DirMeta* 查找到的目录 meta
 */
DirMeta *metaNameDir(int fd, char *path, char *name)
{
    return lookupPath(fd, path, 1, name, true);
}

/**
 * @brief 读取 Dentry 的名字到 buffer 中，因为有 sne 和 lne 两种，所以有两个分支
 *
 * @param buffer 结果 buffer
 * @param entry 待读取的条目
 */
static void readDentryName(char *buffer, Dentry *entry)
{
    // long entry branch
    if (entry->lne.attr == ATTR_LONG_NAME)
    {
        wchar temp[NELEM(entry->lne.name1)];
        memmove(temp, entry->lne.name1, sizeof(temp));
        snstr(buffer, temp, NELEM(entry->lne.name1));
        buffer += NELEM(entry->lne.name1);
        snstr(buffer, entry->lne.name2, NELEM(entry->lne.name2));
        buffer += NELEM(entry->lne.name2);
        snstr(buffer, entry->lne.name3, NELEM(entry->lne.name3));
    }
    else
    {
        // assert: only "." and ".." will enter this branch
        memset(buffer, 0, CHAR_SHORT_NAME + 2); // plus '.' and '\0'
        int i;
        for (i = 0; entry->sne.name[i] != ' ' && i < 8; i++)
        {
            buffer[i] = entry->sne.name[i];
        }
        if (entry->sne.name[8] != ' ')
        {
            buffer[i++] = '.';
        }
        for (int j = 8; j < CHAR_SHORT_NAME; j++, i++)
        {
            if (entry->sne.name[j] == ' ')
            {
                break;
            }
            buffer[i] = entry->sne.name[j];
        }
    }
}

/**
 * @brief 利用 dentry 信息构造 DirMeta
 *
 * @param meta 待填充的 meta
 * @param entry 目录项
 */
static void readDentryInfo(DirMeta *meta, Dentry *entry)
{
    meta->attribute = entry->sne.attr;
    meta->firstClus = ((u32)entry->sne.fst_clus_hi << 16) | entry->sne.fst_clus_lo;
    meta->inode.item[0] = meta->firstClus;
    meta->inodeMaxCluster = 1;
    meta->fileSize = entry->sne.file_size;
    meta->curClus = meta->firstClus;
    meta->clusCnt = 0;
    meta->reserve = entry->sne._nt_res;
}

/**
 * @brief 读出 dirMeta 对应偏移偏移下的特定 childMeta，并计算对应的 dentry 的数量
 *
 * @param dirMeta 目录 meta
 * @param childMeta 子文件 meta
 * @param off 偏移
 * @param count 是一个 file 对应的 dentry 的个数，也可能是空条目的数目
 * @return int -1 失败，0 空条目，1 文件条目
 */
static int dirMetaNext(DirMeta *dirMeta, DirMeta *childMeta, u32 off, int *count)
{
    if (!(dirMeta->attribute & ATTR_DIRECTORY))
        panic("dirMetaNext not dir");
    // 32 个字节一个条目
    if (off % 32)
        panic("dirMetaNext not align");

    Dentry entry;
    // cnt 是空条目的数目
    int cnt = 0;
    // 清空 childMeta 的文件名
    memset(childMeta->filename, 0, FAT32_MAX_FILENAME + 1);
    FileSystem *fs = dirMeta->fileSystem;
    for (int off2; (off2 = relocClus(fs, dirMeta, off, 0)) != -1; off += 32)
    {
        // 读出这个 fat 格式的目录项，如果已经到了结尾，那么就返回 -1
        // 这里虽然用了 lne 的解释形式，但是 sne 或者 lne 的第一个字节都是相同含义
        if (rwClus(fs, dirMeta->curClus, 0, 0, (u64)&entry, off2, 32) != 32 || entry.lne.order == END_OF_ENTRY)
        {
            return -1;
        }
        // 找到了空的条目
        if (entry.lne.order == EMPTY_ENTRY)
        {
            cnt++;
            continue;
        }
        // 如果是有空条目，那么就返回 0
        else if (cnt)
        {
            *count = cnt;
            return 0;
        }
        // 如果是长目录项
        if (entry.lne.attr == ATTR_LONG_NAME)
        {
            // lcnt 是长目录项数量
            int lcnt = entry.lne.order & ~LAST_LONG_ENTRY;
            // 如果是最后一个长目录项
            if (entry.lne.order & LAST_LONG_ENTRY)
            {
                *count = lcnt + 1; // plus the s-n-e;
                count = 0;         // 将 count 指针赋值为 0，避免了其他赋值
            }
            readDentryName(childMeta->filename + (lcnt - 1) * CHAR_LONG_NAME, &entry);
        }
        // 如果是短目录项
        else
        {
            if (count)
            {
                *count = 1;
                readDentryName(childMeta->filename, &entry);
            }
            readDentryInfo(childMeta, &entry);
            return 1;
        }
    }
    return -1;
}

/**
 * @brief 初始化 meta 结构，避免对于 FAT 表的查询
 *
 * @param fs 文件系统
 * @param parent 目录 meta
 */
void loadDirMetas(FileSystem *fs, DirMeta *parent)
{
    u32 off = 0;
    relocClus(fs, parent, 0, 0);
    DirMeta *meta;
    // 暂时分配一个
    dirMetaAlloc(&meta);
    int count;
    // 将 parent 下的所有 dentry 都取出
    int type;
    while ((type = dirMetaNext(parent, meta, off, &count) != -1))
    {
        if (type == 0)
        {
            continue;
        }
        meta->parent = parent;
        meta->off = off;
        meta->fileSystem = fs;
        // 头插法
        meta->nextBrother = parent->firstChild;
        parent->firstChild = meta;
        printk("name: %s, parent: %s\n", meta->filename, parent->filename);
        // 如果 meta 是目录文件，就递归处理
        if ((meta->attribute & ATTR_DIRECTORY) && (off > 32 || parent == &fs->root))
        {
            loadDirMetas(fs, meta);
        }
        dirMetaAlloc(&meta);
        off += count << 5;
    }
    // 释放一下
    dirMetaFree(meta);
}

/**
 * @brief fat 文件系统初始化，构造 superblock，root，bitmap，metas
 *
 * @param fs 文件系统
 * @return int 0 success, -1 fail
 */
int fatInit(FileSystem *fs)
{
    printk("[FAT32 init]fat init begin\n");
    Buf *b = fs->read(fs, 0);
    if (b == 0)
    {
        panic("");
    }
    // char *sp = (char *)(b->data);
    // for (int i = 0; i < 16; i++)
    // {
    //     for (int j = 0; j < 32; j++)
    //     {
    //         printk("%d ", sp[i * 16 + j]);
    //     }
    //     printk("\n");
    // }
    if (strncmp((char const *)(b->data + 82), "FAT32", 5))
    {
        panic("not FAT32 volume");
        return -1;
    }
    // 构造 superblock
    memmove(&fs->superBlock.BPB.bytsPerSec, b->data + 11, 2);
    fs->superBlock.BPB.secPerClus = *(b->data + 13);
    fs->superBlock.BPB.rsvdSecCnt = *(u16 *)(b->data + 14);
    fs->superBlock.BPB.numFATs = *(b->data + 16);
    fs->superBlock.BPB.totSec = *(u32 *)(b->data + 32);
    fs->superBlock.BPB.FATsz = *(u32 *)(b->data + 36);
    fs->superBlock.BPB.rootClus = *(u32 *)(b->data + 44);
    fs->superBlock.firstDataSec = fs->superBlock.BPB.rsvdSecCnt + fs->superBlock.BPB.numFATs * fs->superBlock.BPB.FATsz;
    fs->superBlock.dataSecCnt = fs->superBlock.BPB.totSec - fs->superBlock.firstDataSec;
    fs->superBlock.dataClusCnt = fs->superBlock.dataSecCnt / fs->superBlock.BPB.secPerClus;
    fs->superBlock.bytsPerClus = fs->superBlock.BPB.secPerClus * fs->superBlock.BPB.bytsPerSec;
    brelse(b);

    printk("[FAT32 init]read fat s\n");
    // 打印 superblock 信息
    printk("[FAT32 init]bytsPerSec: %d\n", fs->superBlock.BPB.bytsPerSec);
    printk("[FAT32 init]rootClus: %d\n", fs->superBlock.BPB.rootClus);
    printk("[FAT32 init]secPerClus: %d\n", fs->superBlock.BPB.secPerClus);
    printk("[FAT32 init]numFATs: %d\n", fs->superBlock.BPB.numFATs);
    printk("[FAT32 init]FATsz: %d\n", fs->superBlock.BPB.FATsz);
    printk("[FAT32 init]firstDataSec: %d\n", fs->superBlock.firstDataSec);

    // make sure that bytsPerSec has the same value with BUFFER_SIZE
    if (BUFFER_SIZE != fs->superBlock.BPB.bytsPerSec)
        panic("bytsPerSec != BUFFER_SIZE");

    // 构造根目录 root
    memset(&fs->root, 0, sizeof(fs->root));
    fs->root.attribute = (ATTR_DIRECTORY | ATTR_SYSTEM);
    memset(&fs->root.inode, -1, sizeof(Inode));
    fs->root.inode.item[0] = fs->root.firstClus = fs->root.curClus = fs->superBlock.BPB.rootClus;
    fs->root.inodeMaxCluster = 1;
    fs->root.filename[0] = '/';
    fs->root.fileSystem = fs;
    // ok
    // 构造 cluster bitmap
    int totalClusterNumber = fs->superBlock.BPB.FATsz * fs->superBlock.BPB.bytsPerSec / sizeof(u32);
    u64 *clusterBitmap = (u64 *)getFileSystemClusterBitmap(fs);
    int cnt = 0;
    // 映射页表
    extern u64 kernelPageDirectory[];
    do {
        Page *pp;
        if (pageAlloc(&pp) < 0)
        {
            panic("");
        }
        if (pageInsert(kernelPageDirectory, ((u64)clusterBitmap) + cnt, pp, PTE_READ_BIT | PTE_WRITE_BIT) < 0)
        {
            panic("");
        }
        cnt += PAGE_SIZE;
    } while (cnt * 8 < totalClusterNumber);

    printk("cnxcnx\n");
    // 填写 cluster bitmap
    u32 sec = fs->superBlock.BPB.rsvdSecCnt;
    u32 entryPerSec = fs->superBlock.BPB.bytsPerSec / sizeof(u32);

    printk("sz: %d\n", fs->superBlock.BPB.FATsz);
    // for (u32 i = 0; i < 1; i++, sec++)
    for (u32 i = 0; i < fs->superBlock.BPB.FATsz; i++, sec++)
    {
        b = fs->read(fs, sec);
        printk("sec: %d\n", sec);
        for (u32 j = 0; j < entryPerSec; j++)
        {
            if (((u32 *)(b->data))[j])
            {
                int no = i * entryPerSec + j;
                clusterBitmap[no >> 6] |= (1UL << (no & 63));
            }
        }
        brelse(b);
    }
    printk("cnxcnx\n");
    // 构造 dirMetas
    loadDirMetas(fs, &fs->root);

    printk("[FAT32 init]fat init end\n");
    return 0;
}
