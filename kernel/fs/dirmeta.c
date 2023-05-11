#include <dirmeta.h>
#include <driver.h>

DirMeta dirMetas[DIRMETA_NUM]; // 构成链表的节点
DirMeta *dirMetaHead;          // 空闲链表的头

/**
 * @brief 从空闲链表中取出一个节点
 *
 * @param d 获得节点
 */
void dirMetaAlloc(DirMeta **d)
{
    assert(dirMetaHead != NULL);
    *d = dirMetaHead;
    dirMetaHead = dirMetaHead->nextBrother;
}

/**
 * @brief 释放空闲节点，采用头插法
 *
 * @param d 待释放节点
 */
void dirMetaFree(DirMeta *d)
{
    d->nextBrother = dirMetaHead;
    dirMetaHead = d;
}

/**
 * @brief 将数组穿成一个链表
 *
 */
void dirMetaInit()
{
    for (int i = 0; i < DIRMETA_NUM; i++)
    {
        dirMetas[i].nextBrother = dirMetaHead;
        dirMetaHead = dirMetas + i;
    }
}