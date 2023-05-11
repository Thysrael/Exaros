/**
 * @file dirmeta.h
 * @brief DirMeta 层，DirMeta 结构体是我们构造出来用于辅助 fat 目录项的结构，这一层有一个 cache 方便存取
 * @date 2023-05-10
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _DIRENT_H_
#define _DIRENT_H_

#include "types.h"
#include "fat.h"
#include "fs.h"
#include "inode.h"

#define DIRMETA_NUM 8192

typedef struct DirMeta
{
    char filename[FAT32_MAX_FILENAME + 1];
    u8 attribute;
    u32 firstClus; // 文件的第一簇
    u32 fileSize;  // 文件的字节大小

    u32 curClus;         // 文件的当前簇
    u32 inodeMaxCluster; // 应该是已经缓存过的 cluster
    u32 clusCnt;         // 簇的数量
    Inode inode;         // 用来检索已经用过的簇

    u8 reserve; // 保留字段，主要用于指示该文件是否是链接
    FileSystem *fileSystem;
    /* for OS */
    enum
    {
        ZERO = 10,
        OSRELEASE = 12,
        NONE = 15
    } dev;
    FileSystem *head;            // 构成链表
    u32 off;                     // 这个目录项在目录中偏移（单位应该是字节）
    struct DirMeta *parent;      // 父目录
    struct DirMeta *nextBrother; // 用于构造链表
    struct DirMeta *firstChild;
} DirMeta;

void dirMetaAlloc(DirMeta **d);
void dirMetaFree(DirMeta *d);
void dirMetaInit();

#endif