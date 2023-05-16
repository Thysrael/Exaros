/**
 * @file buffer.h
 * @brief 一个 buffer
 * @date 2023-04-30
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _BIO_H_
#define _BIO_H_

#include "types.h"

#define BUFFER_SIZE 512
#define NBUF 128

typedef struct FileSystem FileSystem;
/**
 * @brief 看上去像是一个 disk 专用的 buffer（没准还有 console，不然也不会用 disk 那个域）
 * 具有双向链表的结构，buffer 的大小是 BSIZE 512
 *
 */
typedef struct Buf
{
    int valid;        // valid == 1 时表示此时 buffer 中含有正确的缓存内容， 等于 0 表示需要重新到硬盘中读取
    int disk;         // does disk "own" buf? 缓冲区的内容已经被修改需要被重新写入磁盘。
    int dev;          // device? 应该是说 (device, blockno) 这个二元组标志了一个块
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