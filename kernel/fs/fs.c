#include <fs.h>
#include <fat.h>
#include <string.h>

FileSystem fileSystem[32];
FileSystem *rootFileSystem;
/**
 * @brief 分配一个文件系统
 *
 * @param fs 分配好的 fs
 * @return int 成功为 0
 */
int fsAlloc(FileSystem **fs)
{
    // 通过遍历找到空闲的文件系统
    for (int i = 0; i < 32; i++)
    {
        if (!fileSystem[i].valid)
        {
            *fs = &fileSystem[i];
            memset(*fs, 0, sizeof(FileSystem));
            fileSystem[i].valid = true;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 初始化根文件系统
 *
 */
void initRootFileSystem()
{
    File *file = filealloc();
    // root 天然挂载，所以就随便分配一个
    rootFileSystem->image = file;
    file->type = FD_DEVICE;
    file->major = 0;
    file->readable = true;
    file->writable = true;
}