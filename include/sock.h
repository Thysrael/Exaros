#ifndef _SOCK_H_
#define _SOCK_H_

#include <types.h>

/**
 * @brief address family, 对应的 socket 中的 domain 参数
 *
 */
enum
{
    AF_UNSPEC = 0, // 适用于指定主机名和服务名且适合任何协议族的地址。
    AF_LOCAL = 1,  // Local to host (pipes and file-domain).
    AF_INET = 2,   // ipv4
    AF_INET6 = 10, // ipv6
};

/**
 * @brief socket 的类型，对应 socket 中的 type 参数
 *
 */
enum
{
    SOCK_STREAM = 1, // 提供有序的、可靠的、双向的和基于连接的字节流，对应 TCP 等
    SOCK_DGRAM = 2,  // 支持无连接的、不可靠的和使用固定大小（通常很小）缓冲区的数据服务，对应 UDP 等
    SOCK_RAW = 3,    // 为用户提供了访问底层通信协议的能力
};

/**
 * @brief  协议的编号，是 socket 的 参数
 *
 */
enum
{
    PROTO_IP = 0,
    PROTO_ICMP = 1,
    PROTO_TCP = 6,
    PROTO_UDP = 17,
};

/**
 * @brief set socket option 需要用到
 *
 */
enum
{
    SOL_SOCKET = 0xffff, // level 的选项，和下面的 option name 选项不同

    SO_REUSEADDR = 0x0001, // 打开或关闭地址复用功能
    SO_KEEPALIVE = 0x0002, // 套接字保活
    SO_BROADCAST = 0x0004, // 允许或禁止发送广播数据
    SO_SNDTIMEO = 0x0010,  // 设置发送超时时间
    SO_RCVTIMEO = 0x0020,  // 设置接收超时时间
    SO_LINGER = 0x0040,    // close 或 shutdown 将等到所有套接字里排队的消息成功发送或到达延迟时间后才会返回. 否则, 调用将立即返回。
    SO_TCP_NODELAY = 0x2008,
    SO_TCP_QUICKACK = 0x2010
};

typedef enum SockType
{
    SOCK_TYPE_NONE = 0,
    SOCK_TYPE_PKT, // 似乎是过时，不要用
    SOCK_TYPE_RAW, // 可以收 ICMP
    SOCK_TYPE_TCP,
    SOCK_TYPE_UDP,
    SOCK_TYPE_NUM, // 最后这个不是 socket 的种类，而是 socket 的数量（神秘的 C 语言）
} SockType;

typedef struct Socket
{
    SockType type; // socket 类型
    int sndtimeo;  // 发送超时
    int rcvtimeo;  // 接收超时

} Socket;

/**
 * @brief 严格意义上讲，这是 socke_addr_in 结构体，
 * 普通的 SockeAddr 是 2 个字节的 family，后面是 14 字节的数据，需要根据 family 的结构判断后面的结构。
 * 因为我们只支持 ipv4，所以就可以使用这个了。ipv6 的
 *
 */
typedef struct
{
    u16 family;   // 与 socket() 中的 domain 保持一致
    u16 port;     // 端口号
    u32 addr;     // IP 地址
    char zero[8]; // 补零
} __attribute__((packed)) SocketAddr;

int sockRead(Socket *sock, bool isUser, u64 addr, int n);
int sockWrite(Socket *sock, bool isUser, u64 addr, int n);

#endif