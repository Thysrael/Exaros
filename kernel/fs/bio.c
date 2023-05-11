#include <bio.h>
#include <types.h>
#include <driver.h>
#include <virtio.h>

/**
 * @brief 这是一个双向链表
 *
 */
struct
{
    Buf buf[NBUF];
    Buf head; // head 的 prev 是链表的最后一个节点，next 是链表的第一个节点
} bcache;

/**
 * @brief 利用头插法构造一个双向链表
 *
 */
void binit(void)
{
    // head 的 prev 和 next 都是 head 本身
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    // 遍历每一个节点，头插法插入
    for (Buf *b = bcache.buf; b < bcache.buf + NBUF; b++)
    {
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

/**
 * @brief 利用 (dev, blockno) 二元组查找，如果有，则返回对应的 Buf，
 * 如果没有，则给这个二元组分配一个新的 Buf
 *
 * @param dev 设备号
 * @param blockno 块号
 * @return Buf* 找到的 Buf 指针
 */
static Buf *bget(int dev, u32 blockno)
{
    Buf *b;
    if (dev >= 0)
    {
        // 沿链表顺序查找是否有 (dev, blockno)  对应的节点
        for (b = bcache.head.next; b != &bcache.head; b = b->next)
        {
            if (b->dev == dev && b->blockno == blockno)
            {
                b->refcnt++;
                return b;
            }
        }
    }

    // 如果没有被缓存
    // 那么逆序遍历链表，找出一个空的 Buf，这是 LRU 的一种体现
    // 越靠后说明其变为 ref == 0 的时间越长
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev)
    {
        // 找到一个没有被引用的，等级 (dev, blockno) 二元组
        if (b->refcnt == 0)
        {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            return b;
        }
    }
    panic("bget: no buffers");
}

/**
 * @brief 先通过 bget 找到 buf，然后依据 buf 的 valid 属性判断是否要读硬件
 *
 * @param dev 设备号
 * @param blockno 块号
 * @return Buf* 缓冲
 */
Buf *bread(int dev, u32 blockno)
{
    Buf *b;
    b = bget(dev, blockno);
    if (!b->valid)
    {
        virtioDiskRW(b, 0);
        b->valid = 1;
    }
    return b;
}

/**
 * @brief 将 b 中的内容写回磁盘
 *
 * @param b 有内容的 b
 */
void bwrite(Buf *b)
{
    virtioDiskRW(b, 1);
}

/**
 * @brief 降低一次引用次数，如果降低为 0，那么就将 buf 重新插入到链表的头部，
 * 这是 LRU 的一部分，越靠前表示 ref == 0 的时间越近
 *
 * @param b buf 块
 */
void brelse(Buf *b)
{
    b->refcnt--;
    if (b->refcnt == 0)
    {
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

/**
 * @brief buffer pin, 将缓冲区固定在内存中，以便其他进程无法修改或删除该缓冲区。
 * 方式就是增加一次引用
 *
 * @param b 缓冲块
 */
void bpin(Buf *b)
{
    b->refcnt++;
}

/**
 * @brief 降低一次引用
 *
 * @param b
 */
void bunpin(Buf *b)
{
    b->refcnt--;
}