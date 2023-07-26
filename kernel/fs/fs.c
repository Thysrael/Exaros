#include <fs.h>
#include <fat.h>
#include <string.h>
#include <driver.h>
#include <syscall.h>

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
    // void testMeta();
    // testMeta();

    // 创建 dev 文件夹
    DirMeta *meta = metaCreate(AT_FDCWD, "/var/tmp/XXX", T_FILE, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/dev", T_DIR, O_RDONLY);
    // vda1 是系统分区，也就是操作系统所在的分区。通常被挂载到根目录 / 下
    // vda2 则是数据分区，通常用于存储用户数据和应用程序等。数据分区通常被挂载到 /data 或 /home 目录下
    // 新建这两个文件是为了挂载测试的需要，`/dev/vda2` 是镜像文件
    meta = metaCreate(AT_FDCWD, "/dev/vda2", T_DIR, O_RDONLY);
    meta->head = rootFileSystem;
    // meta = metaCreate(AT_FDCWD, "/dev/shm", T_DIR, O_RDONLY);   // share memory
    meta = metaCreate(AT_FDCWD, "/dev/null", T_CHAR, O_RDONLY); // share memory
    meta->dev = NONE;
    // meta = metaCreate(AT_FDCWD, "/tmp", T_DIR, O_RDONLY);       // share memory
    // meta = metaCreate(AT_FDCWD, "/dev/zero", T_CHAR, O_RDONLY);
    // meta->dev = ZERO;
    // meta = metaCreate(AT_FDCWD, "/dev/tty", T_CHAR, O_RDONLY);
    // meta = metaCreate(AT_FDCWD, "/dev/rtc", T_CHAR, O_RDONLY);
    // meta = metaCreate(AT_FDCWD, "/dev/rtc0", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/dev/misc", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/dev/misc/rtc", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc/meminfo", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc/mounts", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc/localtime", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc/sys", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc/sys/kernel", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/proc/sys/kernel/osrelease", T_CHAR, O_RDONLY);
    // meta->dev = OSRELEASE;
    meta = metaCreate(AT_FDCWD, "/proc/self", T_DIR, O_RDONLY);
    // if (do_linkat(AT_FDCWD, "/", AT_FDCWD, "/proc/self/exe") < 0)
    // {
    //     panic("");
    // }
    meta = metaCreate(AT_FDCWD, "/etc", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/etc/adjtime", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/etc/localtime", T_CHAR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/var", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/var/tmp", T_DIR, O_RDONLY);
    meta = metaCreate(AT_FDCWD, "/var/tmp/XXX", T_FILE, O_RDONLY);
}