/**
 * @file buffer.h
 * @brief buffer 缓冲层，避免了对于同一个 block 的频繁读写
 * @date 2023-04-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _BIO_H_
#define _BIO_H_

#include "types.h"

#define BUFFER_SIZE 512
#define NBUF 16

typedef struct FileSystem FileSystem;
/**
 * @brief disk 专用的 buffer
 * 具有双向链表的结构，buffer 的大小是 BUFFER_SIZE 512
 *
 */
typedef struct Buf
{
    int valid;        // valid == 1 时表示此时 buffer 中含有正确的缓存内容， 等于 0 表示需要重新到硬盘中读取
    int disk;         // 当 disk == 1，表示内容正在从磁盘上读取，读取完后设置为 0。
    int dev;          // 应该是说 (device, blockno) 这个二元组标志了一个块
    u32 blockno;      // 块号，所谓的 block 似乎应该对应一个 buffer
    u32 refcnt;       // 被使用的次数
    struct Buf *prev; // LRU cache list
    struct Buf *next;
    u8 data[BUFFER_SIZE];
} Buf;

void binit(void);

Buf *mountBlockRead(FileSystem *fs, u64 blockNum);
Buf *blockRead(FileSystem *fs, u64 blockNum);

Buf *bread(int dev, u32 blockno);
void bwrite(Buf *b);
void brelse(Buf *b);
void bpin(Buf *b);
void bunpin(Buf *b);

#endif