#include "segment.h"
#include "process.h"
#include <driver.h>
/**
 * @brief 构建了一个全局数组来完成 Segment 的分配和释放
 *
 */
SegmentMap segmentMaps[SEGMENT_MAP_NUMBER];
u64 segmentMapBitmap[SEGMENT_MAP_NUMBER / 64];

/**
 * @brief 分配一个空闲的 SegmentMap，本质是位图查找
 *
 * @return SegmentMap* 空闲的 segmentMap，失败返回 NULL
 */
SegmentMap *segmentMapAlloc()
{
    for (int i = 0; i < SEGMENT_MAP_NUMBER / 64; i++)
    {
        if (~segmentMapBitmap[i])
        {
            int bit = LOW_BIT64(~segmentMapBitmap[i]);
            segmentMapBitmap[i] |= (1UL << bit);
            return &segmentMaps[(i << 6) | bit];
        }
    }
    panic("segment run out!");
    return NULL;
}

/**
 * @brief 采用头插法在进程中增加一个 segment
 *
 * @param p 待插入的进程
 * @param sm 插入的 segment
 */
void segmentMapAppend(Process *p, SegmentMap *sm)
{
    sm->next = p->segmentMapHead;
    p->segmentMapHead = sm;
}

/**
 * @brief 释放 sm 的方法就是将位图取消掉，但是却没有取消链表，是因为最后链表会在头部清空
 *
 * @param sm
 */
static void segmentMapFree(SegmentMap *sm)
{
    int off = sm - segmentMaps;
    segmentMapBitmap[off >> 6] &= ~(1UL << (off & 63));
    return;
}

/**
 * @brief 通过遍历段列表，释放进程的每一个段
 *
 * @param p 进程
 */
void processSegmentMapFree(Process *p)
{
    for (SegmentMap *sm = p->segmentMapHead; sm; sm = sm->next)
    {
        segmentMapFree(sm);
    }
    p->segmentMapHead = NULL;
}