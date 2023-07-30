/**
 * @file file.h
 * @brief 文件层，在 linux “一切皆文件”的设计思想下， File 结构体不只是“文件”
 * @date 2023-05-10
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _FILE_H_
#define _FILE_H_

#include "types.h"
#include "dirmeta.h"
#include "pipe.h"
#include "socket.h"

#define NDEV 4
#define DEV_SD 0
#define DEV_CONSOLE 1
#define NFILE 512 // Number of fd that all process can open

// map major device number to device functions.
// dev switch
struct devsw
{
    int (*read)(int isUser, u64 dst, u64 start, u64 len);
    int (*write)(int isUser, u64 src, u64 start, u64 len);
};

/**
 * @brief 文件结构体，目前只看懂了引用数量和类型，
 * 可能他有一些结构只是为了某些特定 type 服务的
 * 因为一切皆文件，所以“DirMeta”只是文件的一种，所以没有办法在 File 中集成一些 DirMeta 对应的元数据
 *
 */
typedef struct File
{
    enum
    {
        FD_NONE,
        FD_PIPE,
        FD_ENTRY,
        FD_DEVICE,
        FD_SOCKET
    } type;
    int ref; // reference count
    char readable;
    char writable;
    Pipe *pipe;    // FD_PIPE
    DirMeta *meta; // FD_ENTRY, 用于存储文件的信息
    u32 off;       // FD_ENTRY
    short major;   // FD_DEVICE
    Socket *socket;
    DirMeta *curChild; // current child for getDirmeta
} File;

/* File types.  */
#define DIR_TYPE 0040000  /* Directory.  */
#define CHR_TYPE 0020000  /* Character device.  */
#define BLK_TYPE 0060000  /* Block device.  */
#define REG_TYPE 0100000  /* Regular file.  */
#define FIFO_TYPE 0010000 /* FIFO.  */
#define LNK_TYPE 0120000  /* Symbolic link.  */
#define SOCK_TYPE 0140000 /* Socket.  */

/* these are defined by POSIX and also present in glibc's dirent.h */
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14

#define AT_FDCWD -100 // AT_FDCWD 常量的作用就是表示当前进程的工作目录

// 打开方式
#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002
#define O_CREATE 0x40
#define O_CREATE_GLIBC 0100
#define O_CREATE_GPP 0x200
#define O_APPEND 02000
#define O_TRUNC 01000
#define O_DIRECTORY 0x0200000

void fileinit();
File *filealloc();
void filedup(File *f);
void fileclose(File *f);
int filestat(File *f, u64 addr);
int fileread(File *f, bool isUser, u64 addr, int n);
int filewrite(File *f, bool isUser, u64 addr, int n);
int fileTrunc(File *f, int n);
int getAbsolutePath(DirMeta *d, int isUser, u64 buf, int maxLen);
u64 do_mmap(struct File *fd, u64 start, u64 len, int perm, int flags, u64 off);
#endif