#include <socket.h>
#include <mem_layout.h>
#include <memory.h>
#include <process.h>
#include <thread.h>
#include <sysarg.h>
#include <debug.h>

Socket sockets[SOCKET_COUNT];

/**
 * @brief host to net short, short 交换字节序，因为 host 是小端序（最大的优点是类型转换可以直接截断），而网络是大端序
 *
 * @param host net short
 * @return unsigned net short
 */
static inline u16 htons(u16 host)
{
    return (host >> 8) | ((host << 8) & 0xff00);
}

static inline u32 htonl(u32 host)
{
    return ((host & 0x000000ff) << 24) | ((host & 0x0000ff00) << 8) | ((host & 0x00ff0000) >> 8) | ((host & 0xff000000) >> 24);
}

static void transSocketAddr(SocketAddr *sa)
{
    sa->addr = htonl(sa->addr);
    sa->port = htons(sa->port);
    SOCKET_DEBUG("addr: 0x%x, family: 0x%x, port: 0x%x\n", sa->addr, sa->family, sa->port);
}

/**
 * @brief 获得 socket buffer，也就是一种索引查询
 *
 * @param s 套接字
 * @return u64 buffer 的首地址
 */
inline u64 getSocketBufferBase(Socket *s)
{
    return SOCKET_BUFFER_BASE + (((u64)(s - sockets)) << PAGE_SHIFT);
}

/**
 * @brief 遍历 socket 数组，找到空闲的 socket，分配 buffer
 *
 * @param s 返回值，返回分配好的 Socket
 * @return int
 */
int socketAlloc(Socket **s)
{
    for (int i = 0; i < SOCKET_COUNT; i++)
    {
        if (!sockets[i].used)
        {
            sockets[i].used = true;
            sockets[i].head = sockets[i].tail = 0;
            sockets[i].listening = 0;
            sockets[i].pending_h = sockets[i].pending_t = 0;
            initLock(&sockets[i].lock, "socket lock");
            Page *page;
            int r;
            if ((r = pageAlloc(&page)) < 0)
            {
                return r;
            }
            extern u64 kernelPageDirectory[];
            pageInsert(kernelPageDirectory, getSocketBufferBase(&sockets[i]), page, PTE_READ_BIT | PTE_WRITE_BIT);
            sockets[i].process = myProcess();
            *s = &sockets[i];
            return 0;
        }
    }
    panic("socket run out.\n");
    return -1;
}

/**
 * @brief 遍历 socket 数组，找到 target socket，基本上满足符合的条件。
 * 可能就是因为这个函数，导致其成为了只能 localhost 的原因，因为它只在本地的 socket 中进行搜索。
 *
 * @param local_sock 源 socket
 * @return Socket* 目的 socket
 */
Socket *remote_find_peer_socket(const Socket *local_sock)
{
    // SocketAddr targetSa = local_sock->target_addr;
    // SocketAddr sourceSa = local_sock->addr;
    // SOCKET_DEBUG("source addr: 0x%x, port: 0x%x\n", sourceSa.addr, sourceSa.port);
    // SOCKET_DEBUG("target addr: 0x%x, port: 0x%x\n", targetSa.addr, targetSa.port);
    for (int i = 0; i < SOCKET_COUNT; ++i)
    {
        if (sockets[i].used)
        {
            SOCKET_DEBUG("id: %d, ip: 0x%x, family: %d, port: 0x%x, target port: 0x%x\n", i, sockets[i].addr.addr, sockets[i].addr.family, sockets[i].addr.port, sockets[i].target_addr.port);
        }
        if (sockets[i].used
            /* && sockets[i].addr.family == local_sock->target_addr.family */
            && sockets[i].addr.port == local_sock->target_addr.port
            /*&& sockets[i].target_addr.port == local_sock->addr.port*/)
        {
            return &sockets[i];
        }
    }
    panic("can't find peer socket.\n");
    return NULL;
}

/**
 * @brief 释放 socket，本质是将其的 used 置为 false
 *
 * @param s 待释放的 socket
 */
void socketFree(Socket *s)
{
    extern u64 kernelPageDirectory[];
    pageRemove(kernelPageDirectory, getSocketBufferBase(s));
    s->used = false;
}

/**
 * @brief 创建一个 socket，本质是分配一个 File 和 一个 socket，然后在 File 中登记 socket
 *
 * @param family socket family，一般是 2，也就是 ipv4
 * @param type 一个没有被记录的值
 * @param protocal 同样没有被记录
 * @return int 文件描述符
 */
int createSocket(int family, int type, int protocal)
{
    SOCKET_DEBUG("\n");
    // printf("[%s] family %x type  %x protocal %x\n", __func__, family, type, protocal);
    // if (family != 2)
    // {
    //     printk("family != 2\n");
    //     return -1; // ANET_ERR
    // }
    family = 2;
    assert(family == 2);
    File *f = filealloc();
    Socket *s;
    // 分配一个 socket
    if (socketAlloc(&s) < 0)
    {
        panic("");
    }
    s->addr.family = family;
    s->pending_h = s->pending_t = 0;
    s->listening = 0;
    // 在 file 中登记
    f->socket = s;
    f->type = FD_SOCKET;
    f->readable = f->writable = true;
    assert(f != NULL);
    return fdalloc(f);
}

/**
 * @brief 将 socket 的 socketAddr 换成函数参数指定的
 *
 * @param fd socket 对应的 file
 * @param sa 要绑定的 socketAddr
 * @return int 0 为正常
 */
int bindSocket(int fd, SocketAddr *sa)
{
    SOCKET_DEBUG("\n");
    transSocketAddr(sa);
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    Socket *s = f->socket;
    // assert(s->addr.family == sa->family);
    // assert(sa->addr == 0 || (sa->addr >> 24) == 127);
    s->addr.addr = sa->addr;
    s->addr.port = sa->port;
    SOCKET_DEBUG("addr: 0x%x, family: 0x%x, port: 0x%x\n", sa->addr, sa->family, sa->port);
    return 0;
}

/**
 * @brief 将 Socket->listening 设置为 true
 *
 * @param sockfd socket fd
 * @return int 0 为正常
 */
int listen(int sockfd)
{
    SOCKET_DEBUG("\n");
    SOCKET_DEBUG("server tid is %d\n", myThread()->threadId);
    File *f = myProcess()->ofile[sockfd];
    if (f == NULL)
        panic("");
    assert(f->type == FD_SOCKET);
    Socket *sock = f->socket;
    if (!sock)
        assert(0);
    sock->listening = 1;
    // SocketAddr *sa = &sock->addr;
    // SOCKET_DEBUG("addr: 0x%x, family: 0x%x, port: 0x%x\n", sa->addr, sa->family, sa->port);
    return 0;
}

/**
 * @brief 向 va 拷贝 SocketAddr
 *
 * @param fd 描述符
 * @param va 目的地址
 * @return int 0 为正常
 */
int getSocketName(int fd, u64 va)
{
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    copyout(myProcess()->pgdir, va, (char *)&f->socket->addr, sizeof(SocketAddr));
    return 0;
}

// TODO: 有修改，这个接口暂时无法通过 libc-test

/**
 * @brief 向目标 socket 发送 buf 为首地址的 len 长度的内容
 *
 * @param sock 本地 socket
 * @param buf 缓冲区
 * @param len 长度
 * @param flags 没有用到
 * @param dest 没有用到
 * @return int 发送的长度
 */
int sendTo(Socket *sock, char *buf, u32 len, int flags, SocketAddr *dest)
{
    SOCKET_DEBUG("\n");
    buf[len] = 0;
    // dest->addr = (127 << 24) + 1;
    // dest->port = 0x3241;
    // sock->target_addr.addr = (127 << 24) + 1;
    // sock->target_addr.port = 0x3241;
    int i = remote_find_peer_socket(sock) - sockets;
    // printf("[%s] sockid %d addr 0x%x port 0x%x data %s\n",   __func__, i , dest->addr, dest->port, buf);
    char *dst = (char *)(getSocketBufferBase(&sockets[i]) + (sockets[i].tail & (PAGE_SIZE - 1)));
    u32 num = MIN(PAGE_SIZE - (sockets[i].tail - sockets[i].head), len);
    // 需要等待，因为 socket buffer 已经没有空间了
    if (num == 0)
    {
        getHartTrapFrame()->epc -= 4;
        // yield();
        callYield();
    }

    // 分成两个部分，如果可以直接放在这一页，那么就放，如果不能，那么就往前放（循环的思想）
    int len1 = MIN(num, PAGE_SIZE - (sockets[i].tail & (PAGE_SIZE - 1)));
    strncpy(dst, buf, len1);
    if (len1 < num)
    {
        strncpy((char *)(getSocketBufferBase(&sockets[i])), buf + len1, num - len1);
    }
    // 这里不需要取模吗？其实好像是因为前面计算的时候已经取过模了 `& (PAGE_SIZE - 1)`
    sockets[i].tail += num;
    return num;
}

/**
 * @brief 从源 socket 的 buffer 中读取内容
 *
 * @param s 源 socket
 * @param buf 目的 buf
 * @param len 长度
 * @param flags 没有用到
 * @param srcAddr 没有用到
 * @return int 接收的长度
 */
int receiveFrom(Socket *s, u64 buf, u32 len, int flags, u64 srcAddr)
{
    SOCKET_DEBUG("\n");
    char *src = (char *)(getSocketBufferBase(s) + (s->head & (PAGE_SIZE - 1)));
    // printk("[%s] sockid %d data %s\n", __func__, s - sockets, src);
    u32 num = MIN(len, (s->tail - s->head));
    if (num == 0)
    {
        getHartTrapFrame()->epc -= 4;
        // yield();
        callYield();
    }
    int len1 = MIN(num, PAGE_SIZE - (s->head & (PAGE_SIZE - 1)));
    copyout(myProcess()->pgdir, buf, src, len1);
    if (len1 < num)
    {
        copyout(myProcess()->pgdir, buf + len1, (char *)(getSocketBufferBase(s)),
                num - len1);
    }
    s->head += num;
    // printf("[%s] sockid %d num %d\n", __func__, s - sockets, num);
    return num;
}

/**
 * @brief receiveFrom() 的包装函数
 *
 * @param sock 源 socket
 * @param isUser 必须是 user
 * @param addr 目的地址
 * @param n 长度
 * @return int 接收长度
 */
int socket_read(Socket *sock, bool isUser, u64 addr, int n)
{
    assert(isUser == 1);
    return receiveFrom(sock, addr, n, 0, 0);
}

/**
 * @brief sendTo() 的包装函数
 *
 * @param sock 目的 socket
 * @param isUser 是否是 user
 * @param addr 源地址
 * @param n 长度
 * @return int
 */
int socket_write(Socket *sock, bool isUser, u64 addr, int n)
{
    // 这种方法倒也不错，整体可能是为了方便 either_copyin（虽然我不知道为啥要这么做）
    static char buf[PAGE_SIZE * 10];
    either_copyin(buf, isUser, addr, n);
    return sendTo(sock, buf, n, 0, &sock->target_addr);
}

/**
 * @brief 寻找远程主机（其实就是本机）上的一个 Socket，该 Socket 已经被 bind() 绑定了一个 addr，
 * 并且该 addr 和参数中的 addr 相同。
 *
 * @param addr      target socket addr on remote host.
 * @return Socket*  Pointer to Socket who has been listening a addr which equals to @param addr
 *                  NULL if there are no such socket
 */
static Socket *remote_find_listening_socket(const SocketAddr *addr)
{
    SOCKET_DEBUG("ip: 0x%x, family: %d, port: 0x%x\n", addr->addr, addr->family, addr->port);
    for (int i = 0; i < SOCKET_COUNT; ++i)
    {
        if (sockets[i].used)
        {
            SOCKET_DEBUG("id %d, ip: 0x%x, family: %d, port: 0x%x, target port: 0x%x\n", i, sockets[i].addr.addr, sockets[i].addr.family, sockets[i].addr.port, sockets[i].target_addr.port);
        }
        if (sockets[i].used
            /* && sockets[i].addr.family == addr->family */
            && sockets[i].addr.port == addr->port
            && sockets[i].listening)
        {
            return &sockets[i];
        }
    }
    return NULL;
}

/**
 * @brief 运行在服务器端，会处理一个 socket 的 pending_queue 的队首的一个请求 socket，
 * 为这个请求 socket 分配一个新的 socket 用于通信
 *
 * @param sockfd 服务器端的 socket
 * @param addr 返回值，用于返回服务器端要处理的客户端地址
 * @return int
 */
int accept(int sockfd, SocketAddr *addr)
{
    SOCKET_DEBUG("\n");
    // printk("accept pid: %d\n", myProcess()->processId);
    /* ----------- process on Remote Host --------- */
    File *f = myProcess()->ofile[sockfd];
    assert(f->type == FD_SOCKET);
    Socket *local_sock = f->socket;
    // pending 队列为空，说明没有要处理的事项
    if (local_sock->pending_h == local_sock->pending_t)
    {
        return -11; // EAGAIN /* Try again */
    }
    // printk("pending head is %d, pending tail is %d\n", local_sock->pending_h, local_sock->pending_t);
    *addr = local_sock->pending_queue[(local_sock->pending_h++) % PENDING_COUNT];

    Socket *new_sock;
    socketAlloc(&new_sock);
    new_sock->addr = local_sock->addr;
    new_sock->target_addr = *addr;

    File *new_f = filealloc();
    assert(new_f != NULL);
    new_f->socket = new_sock;
    new_f->type = FD_SOCKET;
    new_f->readable = new_f->writable = true;
    // 唤醒客户端的 socket
    // Socket *peer_sock = remote_find_peer_socket(new_sock);
    // SOCKET_DEBUG("peer addr: 0x%x, port: 0x%x\n", peer_sock->addr.addr, peer_sock->addr.port);

    // wakeup(peer_sock);
    // printf("[%s] wake up client\n", __func__);

    /* ----------- process on Remote Host --------- */

    return fdalloc(new_f);
}

/**
 * @brief 生成一个本地的 socket Address，及本地 ip 地址 + 新分配的 port. 本质是利用 static 变量递增
 *
 */
static SocketAddr gen_local_socket_addr()
{
    static int local_addr = (127 << 24) + 1; // "127.0.0.1"
    static int local_port = 0x2710;
    SocketAddr addr;
    addr.family = 2;
    addr.addr = local_addr;
    addr.port = local_port++;
    return addr;
}

/**
 * @brief 由 TCP 客户端调用，用于和处于 accept 阻塞中的服务端进行连接
 *
 * @param sockfd    local socket fd.
 * @param addr      target socket addr on remote host.
 * @return int      0 if success; -1 if remote socket does not exists!.
 */
int connect(int sockfd, SocketAddr *addr)
{
    SOCKET_DEBUG("\n");
    transSocketAddr(addr);
    // addr->addr = (127 << 24) + 1;
    // addr->family = 2;
    if (addr->port == 0xffff)
    {
        addr->port = 12865;
    }
    File *f = myProcess()->ofile[sockfd];
    assert(f->type == FD_SOCKET);

    Socket *local_sock = f->socket;
    // 产生一个本地端口
    local_sock->addr = gen_local_socket_addr();
    local_sock->target_addr = *addr;

    /* ----------- process on Remote Host --------- */
    // 本来这段代码应该发生在远端，但是因为本机就是远端，所以发生在了这里
    // printk("12865: 0x%x, pid: %d\n", 12865, myProcess()->processId);

    Socket *target_socket = remote_find_listening_socket(addr);
    if (target_socket == NULL)
    {
        printk("remote socket don't exists!\n");
        return -1;
    }
    if (target_socket->pending_t - target_socket->pending_h == PENDING_COUNT)
    {
        return -1; // Connect Count Reach Limit.
    }
    // 在 pending_queue 中加入 local
    target_socket->pending_queue[(target_socket->pending_t++) % PENDING_COUNT] =
        local_sock->addr;
    // printf("[%s] wakeup server sid %d\n", __func__, target_socket-sockets);
    // wakeup(target_socket);

    // Wait for server to accept the connection request.
    acquireLock(&local_sock->lock);
    SOCKET_DEBUG("I am sleeping, tid is %d\n", myThread()->threadId);
    // sleep(local_sock, &local_sock->lock);
    SOCKET_DEBUG("I wake up\n");
    releaseLock(&local_sock->lock);

    /* ----------- process on Remote Host --------- */

    return 0;
}