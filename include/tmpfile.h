/**
 * @file tmpfile.h
 * @brief 用于描述 /tmp/tmpfile_xxx 形式的文件，提高这样文件的读写带宽，本质是在内存中实现文件
 * @date 2023-08-09
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _TMP_FILE_H_
#define _TMP_FILE_H_
#include "types.h"
#include "linux_struct.h"
#include "fat.h"

typedef struct Tmpfile
{
    char filename[32]; // 文件名
    bool used;         // 这个 Tmpfile 已经被占用了
    u64 offset;        // 目前文件的偏移，可以大于 BUFFER_SIZE ，我们在具体读写的时候实现取模操作
    u64 fileSize;      // 这个文件的大小，同样可以大于 BUFFER_SIZE
    u16 _crt_date;     // 底下的时间信息都需要自己记录
    u16 _crt_time;
    u8 _crt_time_tenth;
    u16 _lst_wrt_date;
    u16 _lst_wrt_time;
    u16 _lst_acce_date;
} Tmpfile;

#define TMPFILE_SIZE 0x10000ULL // 64K, 16 pages, 不适合 libc-bench 中的 5 * 10^6 的测试
#define TMPFILE_SHIFT 16
#define TMPFILE_COUNT 128   // 因为总共空间是 30 位，所以可以有很多个 tmpfile，但是这里只声明 128 个
#define TMPFILE_NAME_LEN 14 // 这里的魔数只是随便写的, tmpfile_ 是 8 个字符，然后比较后 4 位

void tmpfileAlloc(char *name, Tmpfile **t);
void tmpfileClose(Tmpfile *t);
int tmpfileRead(Tmpfile *tmpfile, bool isUser, u64 dst, u32 n);
int tmpfileWrite(Tmpfile *tmpfile, bool isUser, u64 addr, int n);
Tmpfile *tmpfileName(char *name);
void tmpfileStat(Tmpfile *tmpfile, struct kstat *st);
void tmpfileSetTime(Tmpfile *tmpfile, TimeSpec ts[2]);
#endif