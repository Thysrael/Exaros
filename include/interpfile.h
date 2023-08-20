#ifndef _INTRPFILE_FILE_H_
#define _INTRPFILE_FILE_H_
#include "types.h"
#include "linux_struct.h"
typedef struct InterpFile
{
    bool used;    // 这个 Tmpfile 已经被占用了
    u64 offset;   // 目前文件的偏移，可以大于 BUFFER_SIZE ，我们在具体读写的时候实现取模操作
    u64 fileSize; // 这个文件的大小，同样可以大于 BUFFER_SIZE
} InterpFile;
void interpfileAlloc();
InterpFile *interpfileName(char *path);
int interpfileRead(bool isUser, u64 dst, u32 n);
int interpfileWrite();
void interpfileStat(struct kstat *st);
#endif