#include <types.h>
#include <inode.h>

Inode inodes[INODE_NUM];
u64 inodeBitmap[INODE_NUM / 64];

/**
 * @brief 遍历 bitmap，分配一个空的 inode 编号
 *
 * @return int 一个 inode 编号
 */
int inodeAlloc()
{
    for (int i = 0; i < sizeof(inodeBitmap); i++)
    {
        if (~inodeBitmap[i])
        {
            int bit = LOW_BIT64(~inodeBitmap[i]);
            inodeBitmap[i] |= (1UL << bit);
            int ret = (i << 6) | bit;
            return ret;
        }
    }
    return -1;
}

/**
 * @brief 释放这个 inode 编号
 *
 * @param x 待释放的 inode 编号
 */
void inodeFree(int x)
{
    assert(inodeBitmap[x >> 6] & (1UL << (x & 63)));
    inodeBitmap[x >> 6] &= ~(1UL << (x & 63));
}

/**
 * @brief 释放 meta 对应的 inode 节点，对于不同层级采用不同方式
 *
 * @param meta 待释放的 meta
 */
void metaFreeInode(DirMeta *meta)
{
    // 释放二级 inode
    for (int i = INODE_SECOND_ITEM_BASE; i < INODE_THIRD_ITEM_BASE; i++)
    {
        if (meta->inodeMaxCluster > INODE_SECOND_LEVEL_BOTTOM + (i - INODE_SECOND_ITEM_BASE) * INODE_ITEM_NUM)
        {
            inodeFree(meta->inode.item[i]);
            continue;
        }
        break;
    }
    // 释放三级 inode
    for (int i = INODE_THIRD_ITEM_BASE; i < INODE_ITEM_NUM; i++)
    {
        int base = INODE_THIRD_LEVEL_BOTTOM + (i - INODE_SECOND_ITEM_BASE) * INODE_ITEM_NUM * INODE_ITEM_NUM;
        if (meta->inodeMaxCluster > base)
        {
            for (int j = 0; j < INODE_ITEM_NUM; j++)
            {
                if (meta->inodeMaxCluster > base + j * INODE_ITEM_NUM)
                {
                    inodeFree(inodes[meta->inode.item[i]].item[j]);
                    continue;
                }
                break;
            }
            inodeFree(meta->inode.item[i]);
            continue;
        }
        break;
    }
    // 清零
    meta->inodeMaxCluster = 0;
    return;
}

/**
 * @brief 将 meta->curClus 通过查询 inode 的方式更新
 *
 * @param meta 需要更新的 meta
 * @param pos 簇的位置
 */
void metaFindInode(DirMeta *meta, int pos)
{
    assert(pos < meta->inodeMaxCluster);
    meta->clusCnt = pos;
    // 一级 inode
    if (pos < INODE_SECOND_LEVEL_BOTTOM)
    {
        meta->curClus = meta->inode.item[pos];
        return;
    }
    // 二级 inode
    if (pos < INODE_THIRD_LEVEL_BOTTOM)
    {
        int idx = INODE_SECOND_ITEM_BASE + (pos - INODE_SECOND_LEVEL_BOTTOM) / INODE_ITEM_NUM;
        meta->curClus = inodes[meta->inode.item[idx]].item[(pos - INODE_SECOND_LEVEL_BOTTOM) % INODE_ITEM_NUM];
        return;
    }
    // 三级 inode
    if (pos < INODE_THIRD_LEVEL_TOP)
    {
        int idx1 = INODE_THIRD_ITEM_BASE + (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM / INODE_ITEM_NUM;
        int idx2 = (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM % INODE_ITEM_NUM;
        meta->curClus = inodes[inodes[meta->inode.item[idx1]].item[idx2]].item[(pos - INODE_THIRD_LEVEL_BOTTOM) % INODE_ITEM_NUM];
        return;
    }
    panic("");
}