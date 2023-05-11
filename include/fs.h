/**
 * @file fs.h
 * @brief 文件系统层，这一层是由于一个 OS 可以识别并挂载多个文件系层，不同的文件系统以链表的形式组织
 * @date 2023-05-10
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _FS_H_
#define _FS_H_

#include "types.h"
#include "bio.h"
#include "fat.h"
#include "dirmeta.h"
#include "file.h"
#include "mem_layout.h"

typedef struct FileSystem
{
    bool valid;              // 表示这个 FAT 文件系统是否被分配，true 为被占用
    char name[64];           // FAT 系统名字
    SuperBlock superBlock;   // 超级块，记录着许多 FAT 的重要信息
    DirMeta root;            // 根目录对应的 meta
    File *image;             // 可能是挂载前的设备路径
    struct FileSystem *next; // 用于组成链表
    int deviceNumber;
    Buf *(*read)(FileSystem *fs, u64 blockNum);
} FileSystem;

/**
 * @brief 获得文件系统的 cluster bitmap
 * 应该是一个 fs 对应一个 bitmap
 *
 * @param fs 文件系统
 * @return u64 bitmap 的首地址
 */
static inline u64 getFileSystemClusterBitmap(FileSystem *fs)
{
    extern FileSystem fileSystem[];
    return FILE_SYSTEM_CLUSTER_BITMAP_BASE + ((fs - fileSystem) << 10) * PAGE_SIZE;
}

#endif