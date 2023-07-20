/**
 * @file exec.h
 * @brief 懒加载需要的函数的数据结构
 * @date 2023-07-18
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include "dirmeta.h"

typedef struct Process Process;
/**
 * @brief 这个结构用于记录 segment 的从文件到内存的映射关系
 *
 */
typedef struct SegmentMap
{
    DirMeta *src;  // 源文件
    u64 srcOffset; // 源文件偏移
    u64 loadAddr;  // 在内存中的加载地址
    u32 len;       // segment 的长度
    u32 flag;      // 用于指示这个段是否是 bss 的，也就是不需要懒加载的。此外还可以当做 PTE 的 flag
    struct SegmentMap *next;
} SegmentMap;

#define MAP_ZERO 1
#define SEGMENT_MAP_NUMBER 1024
SegmentMap *segmentMapAlloc();
void segmentMapAppend(Process *p, SegmentMap *sm);
void processSegmentMapFree(Process *p);

#endif