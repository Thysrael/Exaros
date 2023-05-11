/**
 * @file inode.h
 * @brief 每个 Inode 中记录了 64 个 32 位的整数。前 8 个整数直接记录该位置的簇号，
 * 之后的 16 个整数记录一个 Inode 资源池的索引，表示二级 Inode。
 * 剩余的 40 个整数表示三级 Inode 索引。
 * 在这里，每个文件的大小最多包含 8 + 16 x 64 + 40 x 64 x 64 个簇
 * @date 2023-05-11
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _INODE_H_
#define _INODE_H_

#include "types.h"
#include "dirmeta.h"

#define INODE_NUM 1024
#define INODE_ITEM_NUM 64

typedef struct Inode
{
    u32 item[INODE_ITEM_NUM];
} Inode;

#define INODE_SECOND_ITEM_BASE 8 // 二级索引开始位置
#define INODE_THIRD_ITEM_BASE 24 // 三级索引开始位置

#define INODE_SECOND_LEVEL_BOTTOM INODE_SECOND_ITEM_BASE
#define INODE_SECOND_LEVEL_TOP (INODE_SECOND_LEVEL_BOTTOM + (INODE_THIRD_ITEM_BASE - INODE_SECOND_ITEM_BASE) * INODE_ITEM_NUM)
#define INODE_THIRD_LEVEL_BOTTOM INODE_SECOND_LEVEL_TOP
#define INODE_THIRD_LEVEL_TOP (INODE_THIRD_LEVEL_BOTTOM + (INODE_ITEM_NUM - INODE_THIRD_ITEM_BASE) * INODE_ITEM_NUM * INODE_ITEM_NUM)

int inodeAlloc();
void inodeFree(int i);
void metaFreeInode(DirMeta *meta);
void metaFindInode(DirMeta *meta, int pos);

#endif