#include <fs.h>
#include <fat.h>
#include <string.h>
#include <driver.h>

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
    if (rootFileSystem == NULL)
    {
        fsAlloc(&rootFileSystem);
    }
    strncpy(rootFileSystem->name, "fat32", 6);
    rootFileSystem->read = blockRead;
    File *file = filealloc();
    // root 天然挂载，所以就随便分配一个
    rootFileSystem->image = file;
    file->type = FD_DEVICE;
    file->major = 0;
    file->readable = true;
    file->writable = true;

    dirMetaInit();
    // fatinit 用到了 dirmeta ，要先初始化 dirmeta
    fatInit(rootFileSystem);
    // test meta
    void testMeta();
    testMeta();

    // 创建 dev 文件夹
    DirMeta *meta = metaCreate(AT_FDCWD, "/dev", T_DIR, O_RDONLY);
    // vda1 是系统分区，也就是操作系统所在的分区。通常被挂载到根目录 / 下
    // vda2 则是数据分区，通常用于存储用户数据和应用程序等。数据分区通常被挂载到 /data 或 /home 目录下
    // 新建这两个文件是为了挂载测试的需要，`/dev/vda2` 是镜像文件
    meta = metaCreate(AT_FDCWD, "/dev/vda2", T_DIR, O_RDONLY);
    meta->head = rootFileSystem;
}