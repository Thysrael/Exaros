#include <syscall.h>
#include <file.h>
#include <process.h>
#include <types.h>
#include <trap.h>
#include <linux_struct.h>
#include <fat.h>
#include <sysarg.h>
#include <string.h>
#include <memory.h>
#include <pipe.h>
#include <fs.h>

void syscallPutchar()
{
    putchar(getHartTrapFrame()->a0);
};

/**
 * @brief 分配一个 fd ，并将其与某个 File 关联
 *
 * @param f 需要关联的 File
 * @return int 成功返回 fd，否则返回 -1
 */
static int fdalloc(File *f)
{
    int fd;
    Process *p = myProcess();

    for (fd = 0; fd < NOFILE; fd++)
    {
        if (p->ofile[fd] == 0)
        {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

/**
 * @brief 复制文件描述符，本质是分配出一个新的 fd，然后将其与原来 fd 对应的文件关联起来
 *
 */
void syscallDup(void)
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    // 待复制文件描述符
    int fd = tf->a0;
    // 找出与之关联的 f
    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    // 将 f 与新的 fd 关联起来
    if ((fd = fdalloc(f)) < 0)
    {
        tf->a0 = -1;
        return;
    }

    filedup(f);
    tf->a0 = fd;
}

/**
 * @brief 24号，逻辑基本和 dup 类似
 *
 */
void syscallDupAndSet(void)
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    int fd = tf->a0, fdnew = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    if (fdnew < 0 || fdnew >= NOFILE || myProcess()->ofile[fdnew] != NULL)
    {
        tf->a0 = -1;
        return;
    }

    myProcess()->ofile[fdnew] = f;
    filedup(f);
    tf->a0 = fdnew;
}

/**
 * @brief 利用 fd 查询出对应的 File 即可
 *
 */
void syscallRead(void)
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    if (len < 0)
    {
        tf->a0 = -1;
        return;
    }

    tf->a0 = fileread(f, true, uva, len);
}

void syscallWrite(void)
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    if (len < 0)
    {
        tf->a0 = -1;
        return;
    }

    tf->a0 = filewrite(f, true, uva, len);
}

/**
 * @brief 关闭文件的本质是根据 fd 将关联的 file 取消掉
 *
 */
void syscallClose(void)
{
    Trapframe *tf = getHartTrapFrame();
    int fd = tf->a0;
    File *f;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    myProcess()->ofile[fd] = 0;
    fileclose(f);
    tf->a0 = 0;
}

void syscallGetFileState(void)
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    int fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    tf->a0 = filestat(f, uva);
}

/**
 * @brief 给定一个目录的 fd，获得这个目录的所有目录项
 *
 */
void syscallGetDirMeta()
{
    File *dir;
    int fd, n;
    // addr 是 buffer，要将结果保存到这里
    u64 addr;

    if (argfd(0, &fd, &dir) < 0 || argaddr(1, &addr) < 0 || argint(2, &n) < 0)
    {
        getHartTrapFrame()->a0 = -2;
        return;
    }
    Process *p = myProcess();
    static char buf[512];
    struct linux_dirent64 *dir64 = (struct linux_dirent64 *)buf;

    if (dir->type == FD_ENTRY)
    {
        int nread = 0;
        // 遍历 dir 的每一个节点
        while (dir->curChild)
        {
            DirMeta *cur = dir->curChild;
            int len = strlen(cur->filename);
            // prefix 是结构体除去文件名的部分
            int prefix = ((u64)dir64->d_name - (u64)dir64);
            if (n < prefix + len + 1)
            {
                getHartTrapFrame()->a0 = nread;
                return;
            }
            dir64->d_ino = 0;
            dir64->d_off = 0; // This maybe wrong;
            dir64->d_reclen = len + prefix + 1;
            dir64->d_type = (cur->attribute & ATTR_DIRECTORY) ? DT_DIR : DT_REG;
            // 拷贝其他部分
            if (copyout(p->pgdir, addr, (char *)dir64, prefix) != 0)
            {
                getHartTrapFrame()->a0 = -4;
                return;
            }
            // 拷贝文件名
            if (copyout(p->pgdir, addr + prefix, cur->filename, len + 1) != 0)
            {
                getHartTrapFrame()->a0 = -114;
                return;
            }
            // 记录这个条目
            addr += prefix + len + 1;
            nread += prefix + len + 1;
            n -= prefix + len + 1;
            // 进行迭代，遍历下一个节点
            dir->curChild = dir->curChild->nextBrother;
        }

        getHartTrapFrame()->a0 = nread;
        return;
    }
    getHartTrapFrame()->a0 = -5;
    return;
}

/**
 * @brief 在指定目录打开或者创建文件，最终是生成一个 File
 *
 */
void syscallOpenAt(void)
{
    Trapframe *tf = getHartTrapFrame();
    int startFd = tf->a0, flags = tf->a2, mode = tf->a3;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }

    DirMeta *entryPoint;
    // 如果是创建一个文件，那么勇 metaCreate
    if (flags & O_CREATE_GLIBC)
    {
        entryPoint = metaCreate(startFd, path, T_FILE, mode);
        if (entryPoint == NULL)
        {
            tf->a0 = -1;
            goto bad;
        }
    }
    // 否则是打开文件
    else
    {
        // 按照名字查找文件
        if ((entryPoint = metaName(startFd, path, true)) == NULL)
        {
            tf->a0 = -2; /*must be -ENOENT */
            goto bad;
        }
        if (!(entryPoint->attribute & ATTR_DIRECTORY) && (flags & O_DIRECTORY))
        {
            tf->a0 = -1;
            goto bad;
        }
        if ((entryPoint->attribute & ATTR_DIRECTORY) && (flags & 0xFFF) != O_RDONLY)
        {
            tf->a0 = -1;
            goto bad;
        }
    }

    // 分配出一个 file
    File *file;
    int fd;
    if ((file = filealloc()) == NULL || (fd = fdalloc(file)) < 0)
    {
        if (file)
        {
            fileclose(file);
        }
        tf->a0 = -24;
        goto bad;
    }
    // 如果需要截断
    if (!(entryPoint->attribute & ATTR_DIRECTORY) && (flags & O_TRUNC))
    {
        metaTrunc(entryPoint);
    }

    file->type = FD_ENTRY;
    file->off = (flags & O_APPEND) ? entryPoint->fileSize : 0;
    file->meta = entryPoint;
    file->readable = !(flags & O_WRONLY);
    file->writable = (flags & O_WRONLY) || (flags & O_RDWR);
    if (entryPoint->attribute & ATTR_DIRECTORY)
        file->curChild = entryPoint->firstChild;
    else
        file->curChild = NULL;

    tf->a0 = fd;
bad:
    return;
}

// todo: support the mode
// todo: change the directory? whether we should add the ref(eput)
/**
 * @brief 逻辑基本上和 syscallOpenAt 一样
 *
 */
void syscallMakeDirAt(void)
{
    Trapframe *tf = getHartTrapFrame();
    int dirFd = tf->a0, mode = tf->a2;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    DirMeta *entryPoint;
    bool flag = true;
    for (int i = 0; path[i]; i++)
    {
        if (path[i] != '/')
        {
            flag = false;
            break;
        }
    }
    if (flag)
    {
        tf->a0 = -1;
        return;
    }

    if ((entryPoint = metaCreate(dirFd, path, T_DIR, mode)) == 0)
    {
        goto bad;
    }

    tf->a0 = 0;
    return;

bad:
    tf->a0 = -1;
}

/**
 * @brief 通过修改进程中的 cwd 实现
 *
 */
void syscallChangeDir(void)
{
    Trapframe *tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    DirMeta *meta;

    struct Process *process = myProcess();

    if (fetchstr(tf->a0, path, FAT32_MAX_PATH) < 0 || (meta = metaName(AT_FDCWD, path, true)) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    if (!(meta->attribute & ATTR_DIRECTORY))
    {
        tf->a0 = -1;
        return;
    }

    process->cwd = meta;
    tf->a0 = 0;
}

/**
 * @brief 利用 Process 获得当前工作路径
 *
 */
void syscallGetWorkDir(void)
{
    Trapframe *tf = getHartTrapFrame();
    u64 uva = tf->a0;
    int n = tf->a1;

    if (uva == 0)
    {
        panic("Alloc addr not implement for cwd\n");
    }

    int len = getAbsolutePath(myProcess()->cwd, 1, uva, n);

    if (len < 0)
    {
        tf->a0 = -1;
        return;
    }

    tf->a0 = uva;
}

void syscallPipe(void)
{
    Trapframe *tf = getHartTrapFrame();
    u64 fdarray = tf->a0; // user pointer to array of two integers
    struct File *rf, *wf;
    int fd0, fd1;
    struct Process *p = myProcess();

    if (pipeNew(&rf, &wf) < 0)
    {
        goto bad;
    }

    fd0 = -1;
    if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0)
    {
        if (fd0 >= 0)
            p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        goto bad;
    }

    if (copyout(p->pgdir, fdarray, (char *)&fd0, sizeof(fd0)) < 0 || copyout(p->pgdir, fdarray + sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0)
    {
        p->ofile[fd0] = 0;
        p->ofile[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        goto bad;
    }

    tf->a0 = 0;
    return;

bad:
    tf->a0 = -1;
}

/**
 * @brief 与创建一个普通文件类似，都是创建一个 file
 *
 */
void syscallDevice(void)
{
    Trapframe *tf = getHartTrapFrame();
    int fd, omode = tf->a1;
    int major = tf->a0;
    struct File *f;

    if (omode & O_CREATE_GLIBC)
    {
        panic("dev file on FAT");
    }

    if (major < 0 || major >= NDEV)
    {
        goto bad;
    }

    if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0)
    {
        if (f)
            fileclose(f);
        goto bad;
    }

    f->type = FD_DEVICE;
    f->off = 0;
    f->meta = 0;
    f->major = major;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    tf->a0 = fd;
    return;

bad:
    tf->a0 = -1;
}

/**
 * @brief 还是有一些看不懂
 *
 */
void syscallMount()
{
    Trapframe *tf = getHartTrapFrame();
    u64 imagePathUva = tf->a0, mountPathUva = tf->a1, typeUva = tf->a2, dataUva = tf->a4;
    int flag = tf->a3;
    char imagePath[FAT32_MAX_FILENAME], mountPath[FAT32_MAX_FILENAME], type[10], data[10];
    if (fetchstr(typeUva, type, 10) < 0 || strncmp(type, "vfat", 4))
    {
        tf->a0 = -1;
        return;
    }
    DirMeta *imageMeta, *mountMeta;
    if (fetchstr(imagePathUva, imagePath, FAT32_MAX_PATH) < 0 || (imageMeta = metaName(AT_FDCWD, imagePath, true)) == NULL)
    {
        tf->a0 = -1;
        return;
    }
    if (fetchstr(mountPathUva, mountPath, FAT32_MAX_PATH) < 0 || (mountMeta = metaName(AT_FDCWD, mountPath, true)) == NULL)
    {
        tf->a0 = -1;
        return;
    }
    if (dataUva && fetchstr(dataUva, data, 10) < 0)
    {
        tf->a0 = -1;
        return;
    }
    assert(flag == 0);
    FileSystem *fs;
    if (fsAlloc(&fs) < 0)
    {
        tf->a0 = -1;
        return;
    }

    File *file = filealloc();
    file->off = 0;
    file->readable = true;
    file->writable = true;
    if (imageMeta->head)
    {
        file->type = imageMeta->head->image->type;
        file->meta = imageMeta->head->image->meta;
    }
    else
    {
        file->type = FD_ENTRY;
        file->meta = imageMeta;
    }
    fs->name[0] = 'm';
    fs->name[1] = 0;
    fs->image = file;
    fs->read = mountBlockRead;
    fatInit(fs);
    // 头插法
    fs->next = mountMeta->head;
    mountMeta->head = fs;
    tf->a0 = 0;
}

void syscallUmount()
{
    Trapframe *tf = getHartTrapFrame();
    u64 mountPathUva = tf->a0;
    int flag = tf->a1;
    char mountPath[FAT32_MAX_FILENAME];
    DirMeta *mountMeta;

    if (fetchstr(mountPathUva, mountPath, FAT32_MAX_PATH) < 0 || (mountMeta = metaName(AT_FDCWD, mountPath, true)) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    assert(flag == 0);

    if (mountMeta->head == NULL)
    {
        tf->a0 = -1;
        return;
    }
    // 从头部移除
    mountMeta->head->valid = 0;
    mountMeta->head = mountMeta->head->next;

    tf->a0 = 0;
}

int do_linkat(int oldDirFd, char *oldPath, int newDirFd, char *newPath)
{
    DirMeta *entryPoint, *targetPoint = NULL;

    if ((entryPoint = metaName(oldDirFd, oldPath, true)) == NULL)
    {
        goto bad;
    }

    if ((targetPoint = metaCreate(newDirFd, newPath, T_FILE, O_RDWR)) == NULL)
    {
        goto bad;
    }

    char buf[FAT32_MAX_PATH];
    if (getAbsolutePath(entryPoint, 0, (u64)buf, FAT32_MAX_PATH) < 0)
    {
        goto bad;
    }

    int len = strlen(buf);
    if (metaWrite(targetPoint, 0, (u64)buf, 0, len + 1) != len + 1)
    {
        goto bad;
    }

    targetPoint->reserve = DT_LNK;

    return 0;
bad:
    if (targetPoint)
    {
        metaRemove(targetPoint);
    }
    return -1;
}

int do_unlinkat(int fd, char *path)
{
    DirMeta *entryPoint;
    if ((entryPoint = metaName(fd, path, true)) == NULL)
    {
        goto bad;
    }

    metaTrunc(entryPoint);
    entryPoint->reserve = 0;
    metaRemove(entryPoint);

    return 0;

bad:
    return -1;
}

/**
 * @brief 本质是创建一个新的文件，然后进行链接和记录
 *
 */
void syscallLinkAt()
{
    Trapframe *tf = getHartTrapFrame();
    int oldDirFd = tf->a0, newDirFd = tf->a2, flags = tf->a4;

    assert(flags == 0);
    char oldPath[FAT32_MAX_PATH], newPath[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, oldPath, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    if (fetchstr(tf->a3, newPath, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }

    tf->a0 = do_linkat(oldDirFd, oldPath, newDirFd, newPath);
}

void syscallUnlinkAt()
{
    Trapframe *tf = getHartTrapFrame();
    int dirFd = tf->a0 /*, flags = tf->a2*/;

    // assert(flags == 0);
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }

    tf->a0 = do_unlinkat(dirFd, path);
}

void syscallGetProcessId()
{
}
void syscallYield()
{
}
void syscallProcessDestory()
{
}
void syscallClone()
{
}
void syscallPutString()
{
}
void syscallGetParentProcessId()
{
}
void syscallWait()
{
}
void syscallExit()
{
}
void syscallGetCpuTimes()
{
}
void syscallGetTime()
{
}
void syscallSleepTime()
{
}
void syscallBrk()
{
}
void syscallSetBrk()
{
}
void syscallMapMemory()
{
}
void syscallUnMapMemory()
{
}
void syscallExec()
{
}
void syscallUname()
{
}
void syscallGetDirent()
{
}