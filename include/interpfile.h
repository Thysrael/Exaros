#ifndef _INTRPFILE_FILE_H_
#define _INTRPFILE_FILE_H_
#include "types.h"

typedef struct InterpFile
{
    bool used;    // 这个 Tmpfile 已经被占用了
    u64 offset;   // 目前文件的偏移，可以大于 BUFFER_SIZE ，我们在具体读写的时候实现取模操作
    u64 fileSize; // 这个文件的大小，同样可以大于 BUFFER_SIZE
} InterpFile;

#endif