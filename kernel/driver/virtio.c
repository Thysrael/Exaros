#include <virtio.h>
#include <types.h>
#include <mem_layout.h>
#include <driver.h>
#include <memory.h>
#include <string.h>
#include <lock.h>

/**
 * @brief Disk 结构，因为只有一个 disk block 所以只有一个全局变量
 *
 */
Disk disk;

/**
 * @brief 分为两个部分
 * 一个部分是通过 MMIO 向 virtio block 写入一定的信息，包括最开始的初始化通信和一些基础设置。
 * 另一个部分是初始化 disk 中初始化三个通信结构，分别分配了 3 个页，并初始化了一些辅助结构。
 */
void virtioDiskInit()
{
    printk("magic value: %lx\n", *VIRTIO_ADDRESS(VIRTIO_MMIO_MAGIC_VALUE));
    printk("version: %lx\n", *VIRTIO_ADDRESS(VIRTIO_MMIO_VERSION));
    printk("device id: %lx\n", *VIRTIO_ADDRESS(VIRTIO_MMIO_DEVICE_ID));
    printk("vendor id: %lx\n", *VIRTIO_ADDRESS(VIRTIO_MMIO_VENDOR_ID));
    initLock(&disk.vdiskLock, "vdiskLock");
    // 经过一下校验
    if (*VIRTIO_ADDRESS(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 || *VIRTIO_ADDRESS(VIRTIO_MMIO_VERSION) != 1 || *VIRTIO_ADDRESS(VIRTIO_MMIO_DEVICE_ID) != 2 || *VIRTIO_ADDRESS(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551)
    {
        panic("could not find virtio disk");
    }

    // status 应该就是启动的那个 status
    u32 status = 0;

    // set ACKNOWLEDGE status bit
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *VIRTIO_ADDRESS(VIRTIO_MMIO_STATUS) = status;

    // set DRIVER status bit
    status |= VIRTIO_CONFIG_S_DRIVER;
    *VIRTIO_ADDRESS(VIRTIO_MMIO_STATUS) = status;

    // negotiate features，对，此时切换域了
    u64 features = *VIRTIO_ADDRESS(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *VIRTIO_ADDRESS(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *VIRTIO_ADDRESS(VIRTIO_MMIO_STATUS) = status;

    // re-read status to ensure FEATURES_OK is set.
    status = *VIRTIO_ADDRESS(VIRTIO_MMIO_STATUS);
    if (!(status & VIRTIO_CONFIG_S_FEATURES_OK))
        panic("virtio disk FEATURES_OK unset");

    // legacy 设置页面大小
    *VIRTIO_ADDRESS(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;

    // initialize queue 0. 这是因为我们只有一个 disk 所以只需要维护一个队列即可
    *VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_SEL) = 0;

    // ensure queue 0 is not in use.
    if (*VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_READY))
        panic("virtio disk should not be ready");

    // check maximum queue size.
    u32 max = *VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0)
        panic("virtio disk has no queue 0");
    if (max < RING_SIZE)
        panic("virtio disk max queue too short");

    // set queue size.
    *VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_NUM) = RING_SIZE;

    // allocate and zero queue memory.
    memset(disk.pages, 0, sizeof(disk.pages));
    *VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_PFN) = ((u64)disk.pages) >> PAGE_SHIFT;
    disk.desc = (VirtqDesc *)disk.pages;
    disk.avail = (u16 *)(((char *)disk.desc) + RING_SIZE * sizeof(VirtqDesc));
    disk.used = (VringUsed *)(disk.pages + PAGE_SIZE);

    // queue is ready.
    *VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_READY) = 0x1;

    // all NUM descriptors start out unused.
    for (int i = 0; i < RING_SIZE; i++)
        disk.free[i] = 1;

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *VIRTIO_ADDRESS(VIRTIO_MMIO_STATUS) = status;
}

/**
 * @brief 找到一个空闲的 VirtqDesc，通过查询 free 的方式
 *
 * @return int 成功返回 desc 的 index，否则返回 -1
 */
static int allocDesc()
{
    for (int i = 0; i < RING_SIZE; i++)
    {
        // 1 表示 free，0 表示 unfree
        if (disk.free[i])
        {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

/**
 * @brief 表姐某个 desc 为空闲。并不会将一整条链的 desc 都释放掉，只是释放这一个节点
 *
 * @param i desc 的 index
 */
static void freeDesc(int i)
{
    if (i >= RING_SIZE)
        panic("free desc index greater than RING_SIZE");
    if (disk.free[i])
        panic("free desc has been freed");
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;
    wakeup(&disk.free[0]);
}

/**
 * @brief 遍历整条 desc 链表，逐个释放
 *
 * @param i 头节点的 index
 */
static void freeDescChain(int i)
{
    // 按照链表的顺序释放
    while (1)
    {
        int flag = disk.desc[i].flags;
        int nxt = disk.desc[i].next;
        freeDesc(i);
        // 只要还有后继节点
        if (flag & VRING_DESC_F_NEXT)
            i = nxt;
        else
            break;
    }
}

/**
 * @brief 分配 3 个 Desc，这是因为一次通信需要 3 个 Desc
 *
 * @param idx 一个数组，记录 3 个 desc 的 index
 * @return int 成功返回 0，失败返回 -1
 */
static int alloc3Desc(int *idx)
{
    for (int i = 0; i < 3; i++)
    {
        idx[i] = allocDesc();
        // 分配失败，那么就释放之前分配好的，然后说明分配失败
        if (idx[i] < 0)
        {
            for (int j = 0; j < i; j++)
                freeDesc(idx[j]);
            return -1;
        }
    }
    return 0;
}

/**
 * @brief 对磁盘进行读写
 *
 * @param b buffer，写入的数据或者读出的数据应该在这里面
 * @param write 1 是写入磁盘，0 是读出磁盘
 */
void virtioDiskRW(Buf *b, int write)
{
    // the spec's Section 5.2 says that legacy block operations use
    // three descriptors: one for type/reserved/sector, one for the
    // data, one for a 1-byte status result.
    acquireLock(&disk.vdiskLock);
    // 分配好的 3 个 desc 的 index
    int idx[3];
    while (1)
    {
        if (alloc3Desc(idx) == 0)
        {
            break;
        }
        sleep(&disk.free[0], &disk.vdiskLock);
    }

    // format the three descriptors.
    // qemu's virtio-blk.c reads them.
    // 似乎一次读写需要分配 3 个 desc

    // 首先加工第一个 desc，进而需要先加工出一个 req 来
    struct VirtBlockReq *req0 = &disk.ops[idx[0]];

    if (write)
        req0->type = VIRTIO_BLK_T_OUT; // write the disk
    else
        req0->type = VIRTIO_BLK_T_IN; // read the disk
    req0->reserved = 0;
    // 这里是因为 buffer（或者 block）和 sector 有一个换算结构，sector 是要读写的 sector 序号
    u64 sector = b->blockno * (BUFFER_SIZE / 512);
    // 设置 req0 的 sector
    req0->sector = sector;

    disk.desc[idx[0]].addr = (u64)req0;
    disk.desc[idx[0]].len = sizeof(VirtBlockReq);
    // 设置后继 req
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    // 加工第 2 个 desc，里面
    disk.desc[idx[1]].addr = (u64)b->data;
    disk.desc[idx[1]].len = BUFFER_SIZE;
    if (write)
        disk.desc[idx[1]].flags = 0; // device reads b->data
    else
        disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0; // device writes 0 on success

    // 加工第 3 个 desc
    disk.desc[idx[2]].addr = (u64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // record struct buf for virtioDiskIntrupt().
    b->disk = 1;
    disk.info[idx[0]].b = b;

    // 如上所言，告诉 device 它可以处理我们的这个请求了，'%' 体现了 ring 的特性
    disk.avail[2 + (disk.avail[1] % RING_SIZE)] = idx[0];

    __sync_synchronize();

    // tell the device another avail ring entry is available.
    disk.avail[1] = disk.avail[1] + 1;

    __sync_synchronize();
    // 通知 virio 通信
    *VIRTIO_ADDRESS(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number
    printk("I am here");
    // Wait for virtio_disk_intr() to say request has finished.
    while (b->disk == 1)
    {
        sleep(b, &disk.vdiskLock);
    }

    // 通信结束，释放链表
    disk.info[idx[0]].b = 0;
    freeDescChain(idx[0]);

    releaseLock(&disk.vdiskLock);
}

/**
 * @brief 处理 virtio 发生 interuption 的情况，似乎是通过 used 实现的
 *
 */
void virtioDiskIntrupt()
{
    acquireLock(&disk.vdiskLock);
    // the device won't raise another interrupt until we tell it
    // we've seen this interrupt, which the following line does.
    // this may race with the device writing new entries to
    // the "used" ring, in which case we may process the new
    // completion entries in this interrupt, and have nothing to do
    // in the next interrupt, which is harmless.
    *VIRTIO_ADDRESS(VIRTIO_MMIO_INTERRUPT_ACK) = *VIRTIO_ADDRESS(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

    __sync_synchronize();

    // the device increments disk.used->idx when it
    // adds an entry to the used ring.

    while ((disk.usedIndex % RING_SIZE) != (disk.used->idx % RING_SIZE))
    {
        __sync_synchronize();
        int id = disk.used->ring[disk.usedIndex % RING_SIZE].id;

        if (disk.info[id].status != 0)
            panic("virtioDiskIntrupt status");

        Buf *b = disk.info[id].b;
        b->disk = 0; // disk is done with buf
        wakeup(b);
        disk.usedIndex += 1;
    }

    releaseLock(&disk.vdiskLock);
}
