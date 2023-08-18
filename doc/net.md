# Net

## 零元素数组语法糖

这是一种 GNU GCC 提供的语法糖，表示一个不需要占据内存的指针（神秘），用这种方式，我们可以对于具有变长特性的数组进行很好的描述。

```c
typedef struct pbuf_t
{
    u32 count;        // 引用计数
    union
    {
        u8 payload[0]; // 载荷
        eth_t eth[0];  // 以太网帧
    };
} pbuf_t;
```

比如说这样的结构体，如果将 `pbuf_t` 分配 64B 空间，那么 payload 就指向了一个 60 个元素的 u8 数组空间，因为 `count` 占据 4 个字节。

## 数据结构

协议栈中传递的数据结构就是利用零数组语法糖构建的嵌套结构（说白了就是更加复杂的数组），从最外层的 `pbuf` 开始，这个结构类似于最底层的 buffer。

```c
typedef struct pbuf_t
{
    list_node_t node; // 列表节点
    size_t length;    // 载荷长度
    u32 count;        // 引用计数

    list_node_t tcpnode; // TCP 缓冲节点
    u8 *data;            // TCP 数据指针
    size_t total;        // TCP 总长度 头 + 数据长度
    size_t size;         // TCP 数据长度
    u32 seqno;           // TCP 序列号

    union
    {
        u8 payload[0]; // 载荷
        eth_t eth[0];  // 以太网帧
    };
} pbuf_t;

// 以太网帧
typedef struct eth_t
{
    eth_addr_t dst; // 目标地址
    eth_addr_t src; // 源地址
    u16 type;       // 类型，也就是其上的协议
    union
    {
        u8 payload[0]; // 载荷
        arp_t arp[0];  // arp 包
        ip_t ip[0];    // ip 包
    };

} _packed eth_t;

typedef struct arp_t
{
    u16 hwtype;       // 硬件类型
    u16 proto;        // 协议类型
    u8 hwlen;         // 硬件地址长度
    u8 protolen;      // 协议地址长度
    u16 opcode;       // 操作类型
    eth_addr_t hwsrc; // 源 MAC 地址
    ip_addr_t ipsrc;  // 源 IP 地址
    eth_addr_t hwdst; // 目的 MAC 地址
    ip_addr_t ipdst;  // 目的 IP 地址
} _packed arp_t;

typedef struct ip_t
{
    u8 header : 4;  // 头长度
    u8 version : 4; // 版本号
    u8 tos;         // type of service 服务类型
    u16 length;     // 数据包长度

    // 以下用于分片
    u16 id;         // 标识，每发送一个分片该值加 1
    u8 offset0 : 5; // 分片偏移量高 5 位，以 8字节 为单位
    u8 flags : 3;   // 标志位，1：保留，2：不分片，4：不是最后一个分片
    u8 offset1;     // 分片偏移量低 8 位，以 8字节 为单位

    u8 ttl;        // 生存时间 Time To Live，每经过一个路由器该值减一，到 0 则丢弃
    u8 proto;      // 上层协议，1：ICMP 6：TCP 17：UDP
    u16 chksum;    // 校验和
    ip_addr_t src; // 源 IP 地址
    ip_addr_t dst; // 目的 IP 地址

    union
    {
        u8 payload[0];       // 载荷
        icmp_t icmp[0];      // ICMP 协议
        icmp_echo_t echo[0]; // ICMP ECHO 协议
        udp_t udp[0];        // UDP 协议
        tcp_t tcp[0];        // TCP 协议
    };

} _packed ip_t;

typedef struct icmp_t
{
    u8 type;       // 类型
    u8 code;       // 状态码
    u16 chksum;    // 校验和
    u32 RESERVED;  // 保留
    u8 payload[0]; // 载荷
} _packed icmp_t;

typedef struct icmp_echo_t
{
    u8 type;       // 类型
    u8 code;       // 状态码
    u16 chksum;    // 校验和
    u16 id;        // 标识
    u16 seq;       // 序号
    u8 payload[0]; // 可能是特殊的字符串
} _packed icmp_echo_t;

typedef struct udp_t
{
    u16 sport;     // 源端口号
    u16 dport;     // 目的端口号
    u16 length;    // 长度
    u16 chksum;    // 校验和
    u8 payload[0]; // 载荷
} _packed udp_t;

typedef struct tcp_t
{
    u16 sport; // 源端口号
    u16 dport; // 目的端口号
    u32 seqno; // 该数据包发送的第一个字节的序号
    u32 ackno; // 确认序号，期待收到的下一个字节序号，表示之前的字节已收到

    u8 RSV : 4; // 保留
    u8 len : 4; // 首部长度，单位 4 字节

    union
    {
        u8 flags;
        struct
        {
            u8 fin : 1; // 终止 Finish
            u8 syn : 1; // 同步 Synchronize
            u8 rst : 1; // 重新建立连接 Reset
            u8 psh : 1; // 向上传递，不要等待 Push
            u8 ack : 1; // 确认标志 Acknowledgement
            u8 urg : 1; // 紧急标志 Urgent
            u8 ece : 1; // ECN-Echo Explicit Congestion Notification Echo
            u8 cwr : 1; // Congestion Window Reduced
        } _packed;
    };

    u16 window;    // 窗口大小
    u16 chksum;    // 校验和
    u16 urgent;    // 紧急指针
    u8 options[0]; // TCP 选项
} _packed tcp_t;
```

