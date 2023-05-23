/**
 * @file virtio.h
 * @brief virtio mmio 的寄存器偏移量，与 virtio 通信的数据结构
 * @date 2023-04-29
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include "types.h"
#include "mem_layout.h"
#include "bio.h"
#include "process.h"

// virtio mmio control registers, mapped starting at 0x10001000.
// 这里记录的是所有寄存器的偏移
#define VIRTIO_MMIO_MAGIC_VALUE 0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION 0x004     // version; should be 1
#define VIRTIO_MMIO_DEVICE_ID 0x008   // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID 0x00c   // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_QUEUE_SEL 0x030        // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034    // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM 0x038        // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_READY 0x044      // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050     // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060 // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064    // write-only
#define VIRTIO_MMIO_STATUS 0x070           // read/write
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080   // physical address for descriptor table, write-only
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW 0x090  // physical address for available ring, write-only
#define VIRTIO_MMIO_DRIVER_DESC_HIGH 0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW 0x0a0  // physical address for used ring, write-only
#define VIRTIO_MMIO_DEVICE_DESC_HIGH 0x0a4

// 利用的是 mmio 的原理，从 VIRTIO 这个地址开始访问 virtio 的寄存器
// 这里读出的都是地址
#define VIRTIO_ADDRESS(r) ((volatile u32 *)(VIRTIO + (r)))

// status register bits, from qemu virtio_config.h
// 属于 Device Status Field，用于指示启动流程进行了到了哪一步，在 2.1 有讲解
#define VIRTIO_CONFIG_S_ACKNOWLEDGE 1
#define VIRTIO_CONFIG_S_DRIVER 2
#define VIRTIO_CONFIG_S_DRIVER_OK 4
#define VIRTIO_CONFIG_S_FEATURES_OK 8

// device feature bits
// 0 to 23, and 50 to 127 Feature bits for the specific device type
// 24 to 40 Feature bits reserved for extensions to the queue and feature negotiation mechanisms
// 为 block device 的诸多设置，在 5.2 节
#define VIRTIO_BLK_F_RO 5          /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI 7        /* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE 11 /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ 12         /* support more than one vq */
// 在 6.3 节
#define VIRTIO_F_ANY_LAYOUT 27
// 在 6 节
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

// this many virtio descriptors.
// must be a power of two.
#define RING_SIZE 8

// 是一个 DMA (直接存储器访问)描述符，同时是一个链表节点
// 他们往 VirtqDesc 表中存放 IO 请求，
// 用 VirtAvail 告诉 QEMU 进程  VirtqDesc 表中哪些项是可用的，
// QEMU 将 IO 请求递交给硬件执行后，用 VringUsed 表来告诉前端 VirtqDesc 表中哪些项已经被递交，可以释放这些项了
// 相比于 blk_req，这个似乎更像一个请求
typedef struct VirtqDesc
{
    u64 addr;  // 存储 IO 请求（VirtBlockReq）在虚拟机内的内存地址，或者其他的东西的地址
    u32 len;   // 表示这个 IO 请求在内存中的长度
    u16 flags; // 指示这一行的数据是可读、可写（VRING_DESC_F_WRITE），是否是一个请求的最后一项（VRING_DESC_F_NEXT）
    u16 next;  // 每个 IO 请求都有可能包含了 VirtqDesc 表中的多行，next 域就指明了下一个 VirtqDesc 的 index
} VirtqDesc;

// 为 VirtDesc 中 flags 的属性
#define VRING_DESC_F_NEXT 1  // 是否还有链表后继的 desc
#define VRING_DESC_F_WRITE 2 // device writes (vs read)

// the (entire) avail ring, from the spec.
// 似乎是一个环一样的数据结构，用来存储一定的描述符,指明 vring_desc 中哪些项是可用的;
// 存储的是每个 IO 请求在 vring_desc 中连接成的链表的表头位置
typedef struct VringAvail
{
    u16 flags; // always zero
    // idx 应该是服务于下面紧邻这的这个 ring 的，整个结构体构成了一个环形队列的数据结构
    u16 idx;             // driver will write ring[idx] next, 指向的是 ring 数组中下一个可用的空闲位置
    u16 ring[RING_SIZE]; // descriptor RING_SIZEbers of chain heads, 通过 next 域连接起来的链表的表头在 vring_desc 表中的位置
    u16 unused;
} VringAvail;

// one entry in the "used" ring, with which the
// device tells the driver about completed requests.
typedef struct VringUsedElem
{
    u32 id;  // index of start of completed descriptor chain, 表示处理完成的 IO request 在 vring_desc 表中的组成的链表的头结点位置
    u32 len; // 链表的长度
} VringUsedElem;

// 和 VringAvail 很像，也是一个环结构，说明 vring_desc 中哪些项已经被递交到硬件。
typedef struct VringUsed
{
    u16 flags; // always zero
    u16 idx;   // device increments when it adds a ring[] entry
    VringUsedElem ring[RING_SIZE];
} VringUsed;

// 是 VirtBlockReq 的 type 属性，指示“读或者写”磁盘
#define VIRTIO_BLK_T_IN 0  // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

// 一个 virtio 的请求，这里的请求的意思是从 OS 的角度去看的，
// 我们最终会将某个 req 的地址填写到对应的 desc 中
typedef struct VirtBlockReq
{
    u32 type;     // 0 为读磁盘，1 为写磁盘
    u32 reserved; // 保留位，似乎会被设置为 0
    u64 sector;
} VirtBlockReq;

/**
 * @brief 里面有 VirtDesc, VringAvail, VringUsed 三个结构用于和 virtio 通信
 * 有一些其他结构用于辅助通信
 *
 */
typedef struct Disk
{
    // a set (not a ring) of DMA descriptors, with which the
    // driver tells the device where to read and write individual
    // disk operations. there are NUM descriptors.
    // most commands consist of a "chain" (a linked list) of a couple of
    // these descriptors.
    VirtqDesc *desc;

    // a ring in which the driver writes descriptor numbers
    // that the driver would like the device to process.  it only
    // includes the head descriptor of each chain. the ring has
    // NUM elements.
    VringAvail *avail;

    // a ring in which the device writes descriptor numbers that
    // the device has finished processing (just the head of each chain).
    // there are NUM used ring entries.
    VringUsed *used;

    // 辅助结构，用于指示 desc 是否空闲
    char free[RING_SIZE]; // is a descriptor free?，用 1 表示 free
    u16 usedIndex;        // we've looked this far in used[2..NUM].

    // track info about in-flight operations,
    // for use when completion interrupt arrives.
    // indexed by first descriptor index of chain.
    // 似乎是用来处理一些异常情况的
    struct
    {
        struct Buf *b; // 记录用于参与通信的 buffer
        char status;   // 是一个状态符，可能 virtio 会写这个东西
    } info[RING_SIZE];

    // 一个 index 到对应的 VirtBlockReq 的映射
    VirtBlockReq ops[RING_SIZE];

    // 锁，因为 IO 需要时间
    Spinlock vdiskLock;
} Disk;

void virtioDiskRW(Buf *b, int write);
void virtioDiskInit();
void virtioDiskIntrupt();

#endif