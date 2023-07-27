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
    Process *process;
    SocketAddr addr;                         // 本地地址
    SocketAddr target_addr;                  // 远端地址
    u64 head;                                // head 也是在 buffer 中的偏移量
    u64 tail;                                // tail is equal or greater than head，在 buffer 中的偏移量
    SocketAddr pending_queue[PENDING_COUNT]; // socketAddr 的集合，发挥了一个像 ring 一样的作用
    int pending_h, pending_t;
    struct Spinlock lock;
    int listening;
} Socket;

Socket *remote_find_peer_socket(const Socket *local_sock);
int createSocket(int domain, int type, int protocal);
int bindSocket(int fd, SocketAddr *sa);
int getSocketName(int fd, u64 va);
int sendTo(Socket *sock, char *buf, u32 len, int flags, SocketAddr *dest);
int receiveFrom(Socket *s, u64 buf, u32 len, int flags, u64 srcAddr);
void socketFree(Socket *s);
int accept(int sockfd, SocketAddr *addr);
int connect(int sockfd, SocketAddr *addr);
int socket_read(Socket *sock, bool isUser, u64 addr, int n);
int socket_write(Socket *sock, bool isUser, u64 addr, int n);
int listen(int sockfd);

#endif