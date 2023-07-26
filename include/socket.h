#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "types.h"

typedef struct Process Process;

#define SOCKET_COUNT 128
#define PENDING_COUNT 128

typedef struct
{
    u16 family;
    u16 port;
    u32 addr;
    char zero[24];
} SocketAddr;

typedef struct Socket
{
    bool used;
    Process *process;
    SocketAddr addr;        /* local addr */
    SocketAddr target_addr; /* remote addr */
    u64 head;
    u64 tail;               // tail is equal or greater than head
    SocketAddr pending_queue[PENDING_COUNT];
    int pending_h, pending_t;
    struct Spinlock lock;
    int listening;
} Socket;

#endif