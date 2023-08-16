#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "types.h"
#include "lock.h"

typedef struct Process Process;

#define SOCKET_COUNT 128
#define PENDING_COUNT 128

typedef struct
{
    u16 family;
    u16 port;
    u32 addr;
    char zero[8];
} SocketAddr;

typedef struct Socket
{
    bool used; // socket 已经被占用了
    int type;
    Process *process;
    SocketAddr addr;                         // 本地地址
    SocketAddr target_addr;                  // 远端地址
    u64 head;                                // head 也是在 buffer 中的偏移量
    u64 tail;                                // tail is equal or greater than head，在 buffer 中的偏移量
    SocketAddr pending_queue[PENDING_COUNT]; // socketAddr 的集合，发挥了一个像 ring 一样的作用，似乎只被用于的 accept-connect 过程
    int pending_h, pending_t;
    struct Spinlock lock;
    int listening;
} Socket;

Socket *remote_find_peer_socket(const Socket *local_sock);
int createSocket(int domain, int type, int protocal);
int bindSocket(int fd, SocketAddr *sa);
int getSocketName(int fd, u64 va);
int getSocketOption(int sockfd, int level, int option_name, int *optval, u64 optlen);
int sendTo(Socket *sock, char *buf, u32 len, int flags, SocketAddr *dest);
int receiveFrom(Socket *s, u64 buf, u32 len, int flags, u64 srcAddr);
void socketFree(Socket *s);
int accept(int sockfd, SocketAddr *addr);
int connect(int sockfd, SocketAddr *addr);
int socket_read(Socket *sock, bool isUser, u64 addr, int n);
int socket_write(Socket *sock, bool isUser, u64 addr, int n);
int listen(int sockfd);

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
    SOL_SOCKET = 0x1, // level 的选项，和下面的 option name 选项不同

    SO_REUSEADDR = 0x0001, // 打开或关闭地址复用功能
    SO_KEEPALIVE = 0x0002, // 套接字保活
    SO_BROADCAST = 0x0004, // 允许或禁止发送广播数据
    SO_SNDBUF = 7,
    SO_RCVBUF = 8,
    SO_SNDTIMEO = 0x0010, // 设置发送超时时间
    SO_RCVTIMEO = 0x0020, // 设置接收超时时间
    SO_LINGER = 0x0040,   // close 或 shutdown 将等到所有套接字里排队的消息成功发送或到达延迟时间后才会返回. 否则, 调用将立即返回。
    SO_TCP_NODELAY = 0x2008,
    SO_TCP_QUICKACK = 0x2010
};
#endif