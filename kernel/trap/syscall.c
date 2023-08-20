#include <driver.h>
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
#include <mmap.h>
#include <thread.h>
#include <iovec.h>
#include <debug.h>
#include <fcntl.h>
#include <signal.h>
#include <io.h>
#include <futex.h>
#include <syslog.h>
#include <sysinfo.h>
#include <resource.h>
#include <shm.h>
#include <clock.h>
#include <tmpfile.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR] syscallPutchar,
    [SYSCALL_GET_PROCESS_ID] syscallGetProcessId,
    [SYSCALL_SCHED_YIELD] syscallYield,
    [SYSCALL_PROCESS_DESTORY] syscallProcessDestory,
    [SYSCALL_CLONE] syscallClone,
    [SYSCALL_PUT_STRING] syscallPutString,
    [SYSCALL_GET_PID] syscallGetProcessId,
    [SYSCALL_SET_TID_ADDRESS] syscallSetTidAddress,
    [SYSCALL_GET_PARENT_PID] syscallGetParentProcessId,
    [SYSCALL_WAIT] syscallWait,
    [SYSCALL_DEV] syscallDevice,
    [SYSCALL_DUP] syscallDup,
    [SYSCALL_EXIT] syscallExit,
    [SYSCALL_PIPE2] syscallPipe,
    [SYSCALL_WRITE] syscallWrite,
    [SYSCALL_READ] syscallRead,
    [SYSCALL_CLOSE] syscallClose,
    [SYSCALL_OPENAT] syscallOpenAt,
    [SYSCALL_GET_CPU_TIMES] syscallGetCpuTimes,
    [SYSCALL_GET_TIME_OF_DAY] syscallGetTime,
    // [SYSCALL_SLEEP_TIME] syscallSleepTime,
    [SYS_nanosleep] syscallNanosleep,
    [SYSCALL_DUP3] syscallDupAndSet,
    [SYSCALL_CHDIR] syscallChangeDir,
    [SYSCALL_CWD] syscallGetWorkDir,
    [SYSCALL_MKDIRAT] syscallMakeDirAt,
    [SYSCALL_BRK] syscallBrk,
    [SYSCALL_FSTAT] syscallGetFileState,
    [SYSCALL_MAP_MEMORY] syscallMapMemory,
    [SYSCALL_UNMAP_MEMORY] syscallUnMapMemory,
    [SYSCALL_EXEC] syscallExec,
    [SYSCALL_GET_DIRENT] syscallGetDirent,
    [SYSCALL_MOUNT] syscallMount,
    [SYSCALL_UMOUNT] syscallUmount,
    [SYSCALL_LINKAT] syscallLinkAt,
    [SYSCALL_UNLINKAT] syscallUnlinkAt,
    [SYSCALL_UNAME] syscallUname,
    [SYSCALL_SHUTDOWN] syscallShutdown,
    [SYSCALL_SIGNAL_PROCESS_MASK] syscallSignProccessMask,
    [SYSCALL_SIGNAL_ACTION] syscallSignalAction,
    [SYSCALL_GET_USER_ID] syscallGetUserId,
    [SYSCALL_GET_GROUP_ID] syscallGetGroupId,
    [SYSCALL_FSTATAT] syscallGetFileStateAt,
    [SYSCALL_IOCONTROL] syscallIOControl,
    [SYSCALL_WRITE_VECTOR] syscallWriteVector,
    [SYSCALL_READ_VECTOR] syscallReadVector,
    [SYSCALL_GET_TIME] syscallGetClockTime,
    [SYSCALL_EXIT_GROUP] doNothing,
    [SYSCALL_POLL] syscallPoll,
    [SYSCALL_fcntl] syscall_fcntl,
    [SYSCALL_GET_EFFECTIVE_USER_ID] doNothing,
    [SYSCALL_GET_THREAD_ID] syscallGetTheardId,
    [SYSCALL_GET_EFFECTIVE_GROUP_ID] doNothing,
    [SYSCALL_SET_TIMER] syscallSetTimer,
    [SYSCALL_SET_TIME] syscallSetTime,
    [SYSCALL_KILL] syscallKill,
    [SYSCALL_TKILL] syscallTkill,
    [SYSCALL_TGKILL] syscallTgkill,
    [SYSCALL_SIGRETURN] syscallSigreturn,
    [SYSCALL_SEND_FILE] syscallSendFile,
    [SYSCALL_PREAD] syscallPRead,
    [SYSCALL_PWRITE] syscallPWrite,
    [SYSCALL_SELECT] syscallSelect,
    [SYS_getresuid] syscallGetresuid,
    [SYS_getresgid] syscallGetresgid,
    [SYS_setpgid] syscallSetpgid,
    [SYS_getpgid] syscallGetpgid,
    [SYS_getsid] syscallGetsid,
    [SYS_setsid] syscallSetsid,
    [SYS_futex] syscallFutex,
    [SYS_syslog] syscallSyslog,
    [SYS_umask] syscallUmask,
    [SYS_sysinfo] syscallSysinfo,
    [SYS_rt_sigtimedwait] syscallRtSigtimedwait,
    [SYS_times] syscallTimes,
    [SYS_madvise] syscallMadvise,
    [SYS_mlock] syscallMlock,
    [SYS_mprotect] syscallMprotect,
    [SYS_msync] syscallMsync,
    [SYS_readlinkat] syscallReadlinkat,
    [SYS_renameat2] syscallRenameat2,
    [SYS_statfs] syscallStatfs,
    [SYS_fsync] syscallFsync,
    [SYS_prlimit64] syscallPrlimit64,
    [SYSCALL_SOCKET] syscallSocket,
    [SYSCALL_BIND] syscallBind,
    [SYSCALL_LISTEN] syscallListen,
    [SYSCALL_ACCEPT] syscallAccept,
    [SYSCALL_CONNECT] syscallConnect,
    [SYSCALL_GET_SOCKET_NAME] syscallGetSocketName,
    [SYSCALL_SEND_TO] syscallSendTo,
    [SYSCALL_RECEIVE_FROM] syscallReceiveFrom,
    [SYSCALL_SET_SOCKET_OPTION] syscallSetSocketOption,
    [SYSCALL_ACCESS] syscallAccess,
    [SYSCALL_LSEEK] syscallLseek,
    [SYSCALL_UTIMENSAT] syscallUtimensat,
    [SYSCALL_MEMORY_PROTECT] syscallMemoryProtect,
    [SYSCALL_FCHMOD_AT] syscallFchmodAt,
    [SYS_sched_setscheduler] syscallSchedSetscheduler,
    [SYS_sched_getscheduler] syscallSchedGetscheduler,
    [SYS_sched_getparam] syscallSchedGetparam,
    [SYS_sched_setaffinity] syscallSchedSetaffinity,
    [SYS_sched_getaffinity] syscallSchedGetaffinity,
    [SYSCALL_SYNC] syscallSync,
    [SYSCALL_FTRUNCATE] syscallFtruncate,
    [SYSCALL_SHM_GET] syscallSHMGet,
    [SYSCALL_SHM_CTL] syscallSHMCtrl,
    [SYSCALL_SHM_AT] syscallSHMAt,
    [SYSCALL_SHM_DT] syscallSHMDt,
    [SYSCALL_getrusage] syscallGetrusage,
    [SYS_clock_nanosleep] syscallClockNanosleep,
    [SYSCALL_INTERPFILE] syscallCreateInterpFile,
    [MAX_SYSCALL] 0,
};

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
int fdalloc(File *f)
{
    int fd;
    Process *p = myProcess();

    for (fd = 0; fd < p->fileDescription.hard; fd++)
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
        tf->a0 = -EMFILE;
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
    File *f, *f2;
    int fd = tf->a0, fdnew = tf->a1;
    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    if (fdnew < 0 || fdnew >= NOFILE)
    {
        tf->a0 = -1;
        return;
    }
    if ((f2 = myProcess()->ofile[fdnew]) != NULL)
    {
        fileclose(f2);
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
    // printk("[read] read fd is %d, len is %d\n", fd, len);
    tf->a0 = fileread(f, true, uva, len);
}

void syscallWrite(void)
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;
    CNX_DEBUG("len:%d,\n", len);

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
void syscallGetDirent()
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

static void tmpfileOpenAt()
{
    TMPF_DEBUG("It's tmpfile.\n");
    Trapframe *tf = getHartTrapFrame();
    int flags = tf->a2;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }

    Tmpfile *entryPoint;
    // 创建一个文件
    if (flags & O_CREATE)
    {
        tmpfileAlloc(path, &entryPoint);
    }
    // 否则是打开文件
    else
    {
        // 按照名字查找文件
        if ((entryPoint = tmpfileName(path)) == NULL)
        {
            tf->a0 = -ENOENT; /*must be -ENOENT */
            goto bad;
        }
    }

    File *file;
    int fd;
    // 分配出一个 file 和一个 fd
    if ((file = filealloc()) == NULL || (fd = fdalloc(file)) < 0)
    {
        // 分配失败 fd，但是没有分配失败 file 的情况
        if (file)
        {
            fileclose(file);
        }
        tf->a0 = -24;
        goto bad;
    }

    file->type = FD_TMPFILE;
    file->tmpfile = entryPoint;
    // 设置初始偏移量
    file->tmpfile->offset = (flags & O_APPEND) ? entryPoint->fileSize : 0;
    file->off = 0; // 我们不使用这个偏移量
    file->readable = !(flags & O_WRONLY);
    file->writable = (flags & O_WRONLY) || (flags & O_RDWR);

    tf->a0 = fd;
bad:
    return;
}

static void interpfileOpenAt()
{
    // printk("interpfile open at\n");
    Trapframe *tf = getHartTrapFrame();
    int flags = tf->a2;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }

    InterpFile *entryPoint;
    // 创建一个文件
    if (flags & O_CREATE)
    {
        interpfileAlloc(path, &entryPoint);
    }
    // 否则是打开文件
    else
    {
        // 按照名字查找文件
        if ((entryPoint = interpfileName(path)) == NULL)
        {
            tf->a0 = -ENOENT; /*must be -ENOENT */
            goto bad;
        }
    }

    File *file;
    int fd;
    // 分配出一个 file 和一个 fd
    if ((file = filealloc()) == NULL || (fd = fdalloc(file)) < 0)
    {
        // 分配失败 fd，但是没有分配失败 file 的情况
        if (file)
        {
            fileclose(file);
        }
        tf->a0 = -24;
        goto bad;
    }

    file->type = FD_INTERPFILE;
    file->interpfile = entryPoint;
    // 设置初始偏移量
    file->interpfile->offset = (flags & O_APPEND) ? entryPoint->fileSize : 0;
    file->off = 0; // 我们不使用这个偏移量
    file->readable = true;
    file->writable = false;

    tf->a0 = fd;
bad:
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

    // if (va2PA(myProcess()->pgdir, tf->a1, 0) == 0)
    // {
    //     passiveAlloc(myProcess()->pgdir, tf->a1);
    // }
    // printk("*1 %lx \n", va2PA(myProcess()->pgdir, tf->a1, 0));
    // if (tf->a1 != 0)
    //     printk("111 %c\n", *(char *)(va2PA(myProcess()->pgdir, tf->a1, 0)));

    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    // printk("[syscall openat] open path is %s\n", path);
    if (strncmp(path, "/tmp/tmpfile", 12) == 0)
    {
        tmpfileOpenAt();
        return;
    }
    if (strncmp(path, "/proc/interrupts", 16) == 0)
    {
        interpfileOpenAt();
        return;
    }
    DirMeta *entryPoint;
    // 如果是创建一个文件，那么用 metaCreate
    if (flags & O_CREATE)
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

    if (omode & O_CREATE)
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
    QS_DEBUG("[syscall] Device %d open\n", fd);
    return;

bad:
    tf->a0 = -1;
}

void syscallCreateInterpFile(void)
{
    //     Trapframe *tf = getHartTrapFrame();
    //     int fd;
    //     struct File *f;

    //     if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0)
    //     {
    //         if (f)
    //             fileclose(f);
    //         goto bad;
    //     }

    //     f->type = FD_DEVICE;
    //     f->off = 0;
    //     f->meta = 0;
    //     f->major = DEV_INTPFILE;
    //     f->readable = true;
    //     f->writable = false;

    //     tf->a0 = fd;
    //     extern int interpfilefd;
    //     interpfilefd = fd;
    //     printk("interp file create, fd is %d\n", interpfilefd);
    //     return;

    // bad:
    //     tf->a0 = -1;
}

/**
 * @brief 挂载一个文件系统镜像。文件系统未被挂载前，是以一个镜像文件的形式(image)存在于现有文件系统中的
 * 我们需要将它挂载到现有文件系统的某个挂载点（是一个目录）上去。
 * 挂载可能会发生多个镜像挂载到同一个挂载点上的现象，这个时候需要利用一个头插法链表来记录某个挂载点上的所有镜像
 * 此时挂载点应当呈现最近挂载的镜像的内容
 */
void syscallMount()
{
    Trapframe *tf = getHartTrapFrame();
    u64 imagePathUva = tf->a0, mountPathUva = tf->a1, typeUva = tf->a2, dataUva = tf->a4;
    int flag = tf->a3;
    char imagePath[FAT32_MAX_FILENAME], mountPath[FAT32_MAX_FILENAME], type[10], data[10];
    // vfat 是 FAT 文件系统的一种扩展版本
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
    // 当前挂载点已经有了挂载镜像了，这里的写法可能是冗余的
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

/**
 * @brief 给定挂载点目录，卸载上面挂载的镜像。
 * 同样有重复挂载的问题，如果仅仅是卸载完最近挂载的镜像，挂载点的内容应该是较早挂载的镜像的内容
 * 卸载必须按照逆向挂载顺序操作
 *
 */
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

/**
 * @brief 新建一个链接，本质是在 newPath 处创建一个文件，然后存入 oldPath
 *
 * @param oldDirFd 待链接的文件描述符
 * @param oldPath 待链接的文件文件
 * @param newDirFd 链接文件描述符
 * @param newPath 链接文件的路径
 * @return int 0 为成功
 */
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
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = myProcess()->processId;
}

void syscallSetTidAddress()
{
    Trapframe *tf = getHartTrapFrame();
    // copyout(myProcess()->pgdir, tf->a0, (char*)(&myProcess()->id), sizeof(u64));
    // printf("settid: %lx\n", tf->a0);
    // todo
    myThread()->clearChildTid = tf->a0;
    tf->a0 = myThread()->threadId;
}

void syscallGetParentProcessId()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = myProcess()->parentId;
}

void syscallYield()
{
    // yield();
    callYield();
}

// 销毁进程
void syscallProcessDestory()
{
    Trapframe *tf = getHartTrapFrame();
    u64 processid = tf->a0;
    Process *process;
    int ret;
    if ((ret = pid2Process(processid, &process, 1)) < 0)
    {
        tf->a0 = ret;
        return;
    }
    processDestory(process);
    tf->a0 = 0;
    return;
}

// 创建子进程 todo
void syscallClone()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = clone(tf->a0, tf->a1, tf->a2, tf->a3, tf->a4);
}

void syscallPutString()
{
    Trapframe *trapframe = getHartTrapFrame();
    // printf("hart %d, env %lx printf string:\n", hartId, myProcess()->id);
    u64 va = trapframe->a0;
    int len = trapframe->a1;
    u64 *pte;
    Page *page = pageLookup(myProcess()->pgdir, va, &pte);
    if (page == NULL)
    {
        panic("Syscall put string address error!\nThe virtual address is %x, the length is %x\n", va, len);
    }
    char *start = (char *)(page2PA(page) + (va & 0xfff));
    while (len--)
    {
        putchar(*start);
        start++;
    }
}

void syscallWait()
{
    Trapframe *trapframe = getHartTrapFrame();
    int pid = trapframe->a0;
    u64 addr = trapframe->a1;
    trapframe->a0 = wait(pid, addr);
}

void syscallExit()
{
    // printk("syscall exit begin\n");
    Trapframe *trapframe = getHartTrapFrame();
    Thread *th;
    int ret, ec = trapframe->a0;

    if ((ret = tid2Thread(0, &th, 1)) < 0)
    {
        panic("Process exit error\n");
        return;
    }

    // 为啥要 << 8?
    th->retValue = (ec << 8); // todo
    // printk("will thread destory\n");
    threadDestroy(th);
    // will not reach here
    panic("sycall exit error");
}

/**
 * @brief
 *
 */
void syscallGetCpuTimes()
{
    Trapframe *tf = getHartTrapFrame();
    int cow;
    CpuTimes *ct = (CpuTimes *)va2PA(myProcess()->pgdir, tf->a0, &cow);
    if (cow)
    {
        cowHandler(myProcess()->pgdir, tf->a0);
    }
    *ct = myProcess()->cpuTime;
    tf->a0 = (r_cycle() & 0x3FFFFFFF);
}

u64 timeOffset = 0;
/**
 * @brief 获取当前时间
 *
 */
// TODO timeval and timezone
void syscallGetTime()
{
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time() + timeOffset;
    TimeSpec ts;
    ts.second = time / 1000000;
    ts.microSecond = time % 1000000;
    copyout(myProcess()->pgdir, tf->a0, (char *)&ts, sizeof(TimeSpec));
    tf->a0 = 0;
}

// /**
//  * @brief 进程 sleep 一段时间
//  *
//  */
// void syscallSleepTime()
// {
//     Trapframe *tf = getHartTrapFrame();
//     TimeSpec ts;
//     copyin(myProcess()->pgdir, (char *)&ts, tf->a0, sizeof(TimeSpec));
//     myThread()->awakeTime = r_time() + ts.second * 1000000 + ts.microSecond;
//     kernelProcessCpuTimeEnd();
//     // yield();
//     callYield();
// }

void syscallNanosleep()
{
    Trapframe *tf = getHartTrapFrame();
    RealTimeSpec ts;
    copyin(myProcess()->pgdir, (char *)&ts, tf->a0, sizeof(TimeSpec));
    myThread()->awakeTime = r_time() + ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    kernelProcessCpuTimeEnd();
    tf->a0 = 0;
    callYield();
}

/**
 * @brief brk(u64 addr)
 * 修改数据段的大小
 * 当 addr = 0 时，返回 brkHeapTop
 * 当 addr > brkHeapTop 时，不支持
 * 当 addr < brkHeapTop 时，将 brkHeap 扩展到 addr
 * 注意这里的页分配策略
 */
void syscallBrk()
{
    Trapframe *tf = getHartTrapFrame();
    u64 addr;
    if (argaddr(0, &addr) < 0)
    {
        tf->a0 = -1;
        return;
    }
    // printk("syscallBrk arg addr is 0x%lx\n", addr);
    Process *p = myProcess();
    // if (addr != 0)
    // {
    //     addr &= ((1ul << 32) - 1);
    //     addr |= (0x3cul << 32);
    // }
    // printk("adjust addr: %lx\n", addr);
    if (addr == 0)
    {
        // printk("ask brk, addr is 0x%lx\n", p->brkHeapTop);
        tf->a0 = p->brkHeapTop;
        return;
    }
    else
    {
        if (addr < p->brkHeapTop)
        {
            printk("remove brk\n");
            u64 start = ALIGN_UP(addr, PAGE_SIZE); // 权限一致，需要新的页则直接申请
            u64 end = ALIGN_UP(p->brkHeapTop, PAGE_SIZE);
            while (start < end)
            {
                pageRemove(p->pgdir, start);
                start += PAGE_SIZE;
            }
            p->brkHeapTop = addr;
            tf->a0 = addr;
            return;
        }
        u64 start = ALIGN_UP(p->brkHeapTop, PAGE_SIZE); // 权限一致，需要新的页则直接申请
        u64 end = ALIGN_UP(addr, PAGE_SIZE);
        if (end > USER_BRK_HEAP_TOP)
        {
            // printk("end is 0x%lx, USER_BRK_HEAP_TOP is 0x%lx\n", end, USER_BRK_HEAP_BOTTOM);
            tf->a0 = p->brkHeapTop;
            return;
        }
        while (start < end)
        {
            u64 *pte = NULL;
            Page *page = pageLookup(p->pgdir, start, &pte);
            u64 perm = PTE_READ_BIT | PTE_WRITE_BIT; // 权限必然一致
            if (page == NULL)
            {
                if (pageAlloc(&page) < 0)
                {
                    tf->a0 = p->brkHeapTop;
                    return;
                }
                if (pageInsert(p->pgdir, start, page, perm | PTE_USER_BIT) < 0)
                {
                    tf->a0 = p->brkHeapTop;
                    return;
                }
            }
            else
            {
                panic("Syscall brk remap at 0x%x\n", start);
            }
            start += PAGE_SIZE;
        }
        p->brkHeapTop = addr;
        tf->a0 = addr;
        return;
    }
}

/**
 * @brief mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
 * 将文件或设备映射到内存中
 * addr:
 *  NULL: 自动申请空余的空间
 * prot:
 *  PROT_READ: 可读
 *  PROT_WRITE: 可写
 *  (PROT_EXEC): 可运行
 * flags:
 *  MAP_FILE: useless
 *  MAP_SHARED: 改动需要写回到文件
 *  MAP_PRIVATE: 改动不需要写回到文件
 *  (MAP_ANONYMOUS): 不读取文件
 * return:
 */
void syscallMapMemory()
{
    Trapframe *tf = getHartTrapFrame();
    u64 addr, length, prot, flags, offset;
    File *file;
    if ((argaddr(0, &addr) < 0)
        || (argaddr(1, &length) < 0)
        || (argaddr(2, &prot) < 0)
        || (argaddr(3, &flags) < 0)
        || (argaddr(5, &offset) < 0))
    {
        tf->a0 = -1;
        return;
    }
    if (length == 0)
    {
        tf->a0 = -EINVAL;
        return;
    }

    u64 perm = 0;
    if (PROT_EXEC(prot))
        perm |= PTE_EXECUTE_BIT | PTE_READ_BIT;
    if (PROT_READ(prot))
        perm |= PTE_READ_BIT;
    if (PROT_WRITE(prot))
        perm |= PTE_WRITE_BIT | PTE_READ_BIT;

    /* TODO */
    // 将 fd 从 offset 开始长度为 length 的块以 perm 权限映射到
    // 自定的位置

    argfd(4, 0, &file);
    // // printf("mmap: %lx %lx %lx %lx %d\n", start, len, prot, flags, fd);
    // // printf("heap bottom: %lx\n", myProcess()->heapBottom);

    // trapframe->a0 =
    //     do_mmap(fd, start, len, perm, /*'type' currently not used */ flags, off);
    // // printf("mmap return value: %d\n", trapframe->a0);
    // return;
    // printk("\n** mmap size is 0x%lx **\n", length);
    Process *p = myProcess();
    int alloc = (addr == NULL);
    // printk("** alloc is %d **\n", alloc);
    // 这个分支似乎永远不会执行
    if (alloc == 0)
    {
        tf->a0 = do_mmap(file, addr, length, perm, flags, offset);
        return;
    }
    addr = p->mmapHeapTop;
    u64 start = p->mmapHeapTop; // mmapHeapTop 必然是页对齐的
    u64 end = ALIGN_UP(start + length, PAGE_SIZE);
    if (end > USER_MMAP_HEAP_TOP)
    {
        tf->a0 = -1;
        return;
    }
    p->mmapHeapTop = end;
    while (start < end)
    {
        u64 *pte = NULL;
        Page *page = pageLookup(p->pgdir, start, &pte);
        if (page == NULL)
        {
            if (pageAlloc(&page) < 0)
            {
                tf->a0 = -1;
                return;
            }
            if (pageInsert(p->pgdir, start, page, perm | PTE_USER_BIT) < 0)
            {
                tf->a0 = -1;
                return;
            }
        }
        else
        {
            // 可以考虑 overwrite 或者返回 -1
            panic("Syscall mmap remap at 0x%lx\n", start);
        }
        start += PAGE_SIZE;
    }

    if (MAP_ANONYMOUS(flags))
    {
        tf->a0 = addr;
        return;
    }
    int fd;
    if (argfd(4, &fd, &file) < 0)
    {
        tf->a0 = -1;
        return;
    }

    // 忽略 MAP_SHARED & MAP_PRIVATE
    // 均当作 MAP_PRIVATE 处理
    file->off = offset;
    if (fileread(file, true, addr, length) < 0)
    {
        tf->a0 = -1;
        return;
    }
    tf->a0 = addr;
    return;
}

/**
 * @brief munmap(addr, length)
 * 将文件或设备取消映射到内存中
 * addr:
 *
 */
void syscallUnMapMemory()
{
    Trapframe *tf = getHartTrapFrame();
    u64 addr, length;
    if ((argaddr(0, &addr) < 0) || (argaddr(1, &length) < 0))
    {
        tf->a0 = -1;
        return;
    }
    if ((addr & (PAGE_SIZE - 1))) // 返回的 addr 应该是按页对齐的（和 mmap 一致）
    {
        tf->a0 = -1;
        return;
    }

    u64 start = addr; // mmapHeapTop 必然是页对齐的
    u64 end = ALIGN_UP(start + length, PAGE_SIZE);
    if (end > USER_MMAP_HEAP_TOP)
    {
        tf->a0 = -1;
        return;
    }
    Process *p = myProcess();
    while (start < end)
    {
        if (pageRemove(p->pgdir, start) < 0)
        {
            tf->a0 = -1;
            return;
        }
        start += PAGE_SIZE;
    }
    tf->a0 = 0;
    return;
}

#define MAX_PATH_LEN 128
#define MAX_ARG 32
/**
 * @brief 功能：执行一个指定的程序；
 * 输入：
 * path: 待执行程序路径名称，
 * argv: 程序的参数，
 * envp: 环境变量的数组指针
 * 返回值：成功不返回，失败返回-1
 */
void syscallExec()
{
    Trapframe *tf = getHartTrapFrame();
    char path[MAX_PATH_LEN], *argv[MAX_ARG];
    u64 u_argvs_addr, u_argv_addr;
    // 将参数取出
    if (argstr(0, path, MAX_PATH_LEN) < 0 || argaddr(1, &u_argvs_addr))
    {
        tf->a0 = -1;
        return;
    }
    // 清空 argv
    memset(argv, 0, sizeof(argv));
    // 填写 argv
    for (int i = 0;; i++)
    {
        if (i >= NELEM(argv))
        {
            goto bad;
        }
        if (fetchaddr(u_argvs_addr + (sizeof(u64) * i), (u64 *)&u_argv_addr) < 0)
        {
            goto bad;
        }
        // 参数结束
        if (u_argv_addr == 0)
        {
            argv[i] = 0;
            break;
        }
        Page *page;
        if (pageAlloc(&page))
        {
            goto bad;
        }
        argv[i] = (char *)page2PA(page);
        if (argv[i] == 0)
            goto bad;
        if (fetchstr(u_argv_addr, argv[i], PAGE_SIZE) < 0)
            goto bad;
    }

    // 真正的执行
    // // 输出

    // printk("\npath: %s\n", path);
    for (int i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    {
        // if (argv[i] > 0)
        //     printk("%d : %s\n", i, argv[i]);
        if (i == 1 && argv[i][0] == 'p' && argv[i][1] == 't' && argv[i][8] == 'c' && argv[i][9] == 'a') // 跳过 pthread
        {
            myThread()->retValue = 0;
            myThread()->clearChildTid = 0;

            threadDestroy(myThread());
        }
    }

    int ret = exec(path, argv);

    // 释放给参数开的空间
    for (int i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    {
        pageFree(pa2Page((u64)argv[i]));
    }
    tf->a0 = ret;
    // printk("sucessfully exec %s, th: %lx, epc: %lx\n", path, myThread()->threadId, tf->epc);
    // printTrapframe(tf);
    return;
bad:
    for (int i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    {
        pageFree(pa2Page((u64)argv[i]));
    }
    tf->a0 = -1;
    return;
}

/**
 * @brief 打印系统信息
 *
 */
void syscallUname()
{
    struct utsname
    {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
        char domainname[65];
    } uname;
    strncpy(uname.sysname, "ExarOs", 65);
    strncpy(uname.nodename, "my_node", 65);
    strncpy(uname.release, "0.1.0", 65);
    strncpy(uname.version, "0.1.0", 65);
    strncpy(uname.machine, "Risc-V virt", 65);
    strncpy(uname.domainname, "Beijing", 65);
    Trapframe *tf = getHartTrapFrame();
    copyout(myProcess()->pgdir, tf->a0, (char *)&uname, sizeof(struct utsname));
}

void syscallShutdown()
{
#ifdef FAT_DUMP
    extern FileSystem *rootFileSystem;
    dumpDirMetas(rootFileSystem, &(rootFileSystem->root));
#endif
    SBI_CALL_0(SBI_SHUTDOWN);
    return;
}

void syscallGetUserId()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetGroupId()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetFileStateAt(void)
{
    Trapframe *tf = getHartTrapFrame();
    int dirfd = tf->a0 /*, flags = tf->a2*/;
    u64 uva = tf->a2;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    // printf("path: %s\n", path);
    DirMeta *entryPoint = metaName(dirfd, path, true);
    if (entryPoint == NULL)
    {
        tf->a0 = -ENOENT;
        return;
    }

    struct kstat st;
    // elock(entryPoint);
    metaStat(entryPoint, &st);
    // eunlock(entryPoint);
    if (copyout(myProcess()->pgdir, uva, (char *)&st, sizeof(struct kstat)) < 0)
    {
        tf->a0 = -1;
        return;
    }
    tf->a0 = 0;
}

/**
 * @brief 设置对指定信号的处理方法
 *
 */
void syscallSignalAction()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = rt_sigaction(tf->a0, tf->a1, tf->a2);
    return;
}

void syscallSignProccessMask()
{
    Trapframe *tf = getHartTrapFrame();
    u64 how = tf->a0;
    u64 setAddr = tf->a1;
    u64 oldSetAddr = tf->a2;
    u64 setSize = tf->a3;
    Process *p = myProcess();
    SignalSet set, oldSet;
    if (setAddr)
        copyin(p->pgdir, (char *)&set, setAddr, setSize);
    tf->a0 = rt_sigprocmask(how, &set, &oldSet, setSize);
    if (oldSetAddr != 0)
        copyout(p->pgdir, oldSetAddr, (char *)&oldSet, setSize);
}

#define TIOCGWINSZ 0x5413

struct WinSize
{
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
} winSize = {24, 80, 0, 0};

void syscallIOControl()
{
    Trapframe *tf = getHartTrapFrame();
    // printf("fd: %d, cmd: %d, argc: %lx\n", tf->a0, tf->a1, tf->a2);
    if (tf->a1 == TIOCGWINSZ)
        copyout(myProcess()->pgdir, tf->a2, (char *)&winSize, sizeof(winSize));
    tf->a0 = 0;
}

void syscallWriteVector()
{
    Trapframe *tf = getHartTrapFrame();
    struct File *f;
    int fd = tf->a0;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        goto bad;
    }

    int cnt = tf->a2;
    if (cnt < 0 || cnt >= IOVMAX)
    {
        goto bad;
    }
    // static u64 call_cnt = 0;
    // printk("[write vector] fd is %d, cnt is %d, time is %lu\n", fd, cnt, call_cnt++);
    struct Iovec vec[IOVMAX];
    struct Process *p = myProcess();

    if (copyin(p->pgdir, (char *)vec, tf->a1, cnt * sizeof(struct Iovec)) != 0)
    {
        goto bad;
    }

    u64 len = 0;
    for (int i = 0; i < cnt; i++)
    {
        // printk("\t[write vector] i: %d, len: %d\n", i, vec[i].iovLen);
        if (vec[i].iovLen > 0)
            len += filewrite(f, true, (u64)vec[i].iovBase, vec[i].iovLen);
    }
    tf->a0 = len;
    return;

bad:
    tf->a0 = -1;
}

void syscallReadVector()
{
    Trapframe *tf = getHartTrapFrame();
    struct File *f;
    int fd = tf->a0;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        goto bad;
    }

    int cnt = tf->a2;
    if (cnt < 0 || cnt >= IOVMAX)
    {
        goto bad;
    }
    // printk("[read vector] fd is %d, cnt is %d\n", fd, cnt);
    struct Iovec vec[IOVMAX];
    struct Process *p = myProcess();

    if (copyin(p->pgdir, (char *)vec, tf->a1, cnt * sizeof(struct Iovec)) != 0)
    {
        goto bad;
    }

    u64 len = 0;
    for (int i = 0; i < cnt; i++)
    {
        // printk("\t[read vector] i: %d, len: %d\n", i, vec[i].iovLen);
        len += fileread(f, true, (u64)vec[i].iovBase, vec[i].iovLen);
    }
    tf->a0 = len;
    return;

bad:
    tf->a0 = -1;
}

void syscallGetClockTime()
{
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time();
    TimeSpec ts;
    ts.second = time / 1000000;
    ts.microSecond = time % 1000000 * 1000; // todo
    // printk("kernel time: %ld second: %ld ms: %ld\n", time, ts.second, ts.microSecond);
    copyout(myProcess()->pgdir, tf->a1, (char *)&ts, sizeof(TimeSpec));
    tf->a0 = 0;
}

void doNothing()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallPoll()
{
    Trapframe *tf = getHartTrapFrame();
    struct pollfd
    {
        int fd;
        short events;
        short revents;
    };
    struct pollfd p;
    u64 startva = tf->a0;
    int n = tf->a1;
    int cnt = 0;
    for (int i = 0; i < n; i++)
    {
        copyin(myProcess()->pgdir, (char *)&p, startva, sizeof(struct pollfd));
        p.revents = 0;
        copyout(myProcess()->pgdir, startva, (char *)&p, sizeof(struct pollfd));
        startva += sizeof(struct pollfd);
        cnt += p.revents != 0;
    }
    // tf->a0 = cnt;
    tf->a0 = 1;
}

void syscall_fcntl(void)
{
    Trapframe *tf = getHartTrapFrame();
    struct File *f;
    int fd = tf->a0 /*, cmd = tf->a1, flag = tf->a2*/;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
    {
        tf->a0 = -1;
        return;
    }

    switch (tf->a1)
    {
    case FCNTL_GETFD:
        tf->a0 = 1;
        return;
    case FCNTL_SETFD:
        tf->a0 = 0;
        return;
    case FCNTL_GET_FILE_STATUS:
        tf->a0 = 04000;
        return;
    case FCNTL_DUPFD_CLOEXEC:
        fd = fdalloc(f);
        filedup(f);
        tf->a0 = fd;
        return;
    case FCNTL_SETFL:
        // printf("set file flag, bug not impl. flag :%x\n", tf->a2);
        tf->a0 = 0;
        return;
    default:
        panic("%d\n", tf->a1);
        break;
    }

    // printf("syscall_fcntl fd:%x cmd:%x flag:%x\n", fd, cmd, flag);
    tf->a0 = 0;
    return;
}

void syscallGetTheardId()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = myThread()->threadId;
}

void syscallSetTime()
{
    Trapframe *tf = getHartTrapFrame();
    TimeSpec ts;
    printk("settimee\n");
    copyin(myProcess()->pgdir, (char *)&ts, tf->a0, sizeof(TimeSpec));
    printk("%d %d\n", ts.microSecond, ts.second);
    timeOffset = (ts.second * 1000000 + ts.microSecond) - r_time();
    tf->a0 = 0;
}

void syscallSetTimer()
{
    Trapframe *tf = getHartTrapFrame();
    // printk("set Timer: %lx %lx %lx\n", tf->a0, tf->a1, tf->a2);
    IntervalTimer time = getTimer();
    if (tf->a2)
    {
        copyout(myProcess()->pgdir, tf->a2, (char *)&time, sizeof(IntervalTimer));
    }
    if (tf->a1)
    {
        copyin(myProcess()->pgdir, (char *)&time, tf->a1, sizeof(IntervalTimer));
        setTimer(time);
    }
    tf->a0 = 0;
}
void syscallKill()
{
    Trapframe *tf = getHartTrapFrame();
    int pid = tf->a0;
    int sig = tf->a1;
    tf->a0 = kill(pid, sig);
    return;
}

void syscallTkill()
{
    Trapframe *tf = getHartTrapFrame();
    int tid = tf->a0;
    int sig = tf->a1;
    tf->a0 = tkill(tid, sig);
    return;
}

void syscallTgkill()
{
    Trapframe *tf = getHartTrapFrame();
    u64 tgid = tf->a0;
    u64 tid = tf->a1;
    u64 sig = tf->a2;
    tf->a0 = tgkill(tgid, tid, sig);
    return;
}

void syscallSigreturn()
{
    rt_sigreturn();
    return;
}

typedef struct
{
    u64 bits[2];
} FdSet;

/**
 * @brief 在一段指定的时间内，监听用户感兴趣的文件描述符上可读、可写和异常 fd 集合
 *
 * @param nfd 文件描述符的个数
 * @param readset 查询的是否可读 fd 集合，同时也是返回值，返回确实可读的 fd 集合，若为 NULL，则表示不查询
 * @param writeset 查询的是否可写 fd 集合，同时也是返回值，返回确实可写的 fd 集合，若为 NULL，则表示不查询
 * @param exceptset 查询的是否异常 fd 集合，同时也是返回值，返回确实异常的 fd 集合，若为 NULL，则表示不查询
 * @param timeout 超时时间，如果一直没有“就绪”的 fd，那么就会等待 timeout 的时间，然后返回 0，如果有就绪的，那么立刻返回。若为 NULL，则无限期等待
 *
 * @return 执行失败返回 -1，超时返回 0，否则返回已经就绪的 fd 的个数
 *
 */
// void syscallSelect()
// {
//     Trapframe *tf = getHartTrapFrame();
//     // printf("thread %lx get in select, epc: %lx\n", myThread()->id, tf->epc);
//     int nfd = tf->a0;
//     // assert(nfd <= 128);
//     u64 read = tf->a1, write = tf->a2, except = tf->a3, timeout = tf->a4;
//     // printk("read: %lx, write: %lx, execept: %lx, timeout: %lx\n", read, write, except, timeout);
//     // assert(timeout != 0);
//     // cnt 是已经就绪的 fd 的个数
//     int cnt = 0;
//     struct File *file = NULL;
//     // printf("[%s] \n", __func__);

//     FdSet readSet_ready;
//     if (read)
//     {
//         FdSet readSet;
//         copyin(myProcess()->pgdir, (char *)&readSet, read, sizeof(FdSet));
//         readSet_ready = readSet;
//         for (int i = 0; i < nfd; i++)
//         {
//             file = NULL;
//             u64 cur = i < 64 ? readSet.bits[0] & (1UL << i) : readSet.bits[1] & (1UL << (i - 64));
//             if (!cur)
//                 continue;
//             // printk("[select] selecting read fd %d type %d\n", i, myProcess()->ofile[i]->type);
//             file = myProcess()->ofile[i];
//             if (!file)
//                 continue;

//             int ready_to_read = 1;
//             switch (file->type)
//             {
//             case FD_PIPE:
//                 // printk("[select] pipe:%lx nread: %d nwrite: %d\n", file->pipe, file->pipe->nread, file->pipe->nwrite);
//                 if (file->pipe->nread == file->pipe->nwrite)
//                 {
//                     ready_to_read = 0;
//                 }
//                 else
//                 {
//                     ready_to_read = 1;
//                 }
//                 break;
//             case FD_SOCKET:
//                 // printf("socketid %d head %d tail %d\n", file->socket-sockets, file->socket->head, file->socket->tail);
//                 if (file->socket->used != 0 && file->socket->head == file->socket->tail && (!file->socket->listening || (file->socket->listening && file->socket->pending_h == file->socket->pending_t)))
//                 {
//                     ready_to_read = 0;
//                 }
//                 else
//                 {
//                     ready_to_read = 1;
//                 }
//                 break;
//             case FD_DEVICE:
//                 if (hasChar())
//                 {
//                     ready_to_read = 1;
//                 }
//                 else
//                 {
//                     ready_to_read = 0;
//                 }
//                 break;
//             default:
//                 ready_to_read = 1;
//                 break;
//             }

//             if (ready_to_read)
//             {
//                 ++cnt;
//             }
//             else
//             {
//                 // printk("fd %d not ready\n", i);
//                 if (i < 64)
//                     readSet_ready.bits[0] &= ~cur;
//                 else
//                     readSet_ready.bits[1] &= ~cur;
//             }
//         }
//     }
//     // 这里默认写是永远就绪的，应该是不太对的，比如说对于管道，如果 buffer 满了，那么就是不能写的了
//     if (write)
//     {
//         FdSet writeSet;
//         copyin(myProcess()->pgdir, (char *)&writeSet, write, sizeof(FdSet));
//         for (int i = 0; i < nfd; i++)
//         {
//             // printk("[select] selecting write fd %d type %d\n", i, myProcess()->ofile[i]->type);
//             if (i >= 64)
//             {
//                 cnt += !!((1UL << (i - 64)) & writeSet.bits[1]);
//             }
//             else
//             {
//                 cnt += !!((1UL << (i)) & writeSet.bits[0]);
//             }
//         }
//         copyout(myProcess()->pgdir, write, (char *)&write, sizeof(FdSet));
//     }

//     // 这个默认没有异常
//     if (except)
//     {
//         // FdSet set;
//         // copyin(myProcess()->pgdir, (char*)&set, except, sizeof(FdSet));
//         u8 zero = 0;
//         memsetOut(myProcess()->pgdir, except, zero, nfd);
//         // memset(&set, 0, sizeof(FdSet));
//         // copyout(myProcess()->pgdir, except, (char*)&set, sizeof(FdSet));
//     }

//     // 说明还没有可以处理的文件描述符，那么需要考虑等待
//     if (cnt == 0)
//     {
//         // TODO: 按照原先的写法，这里说的是只要 timeout != NULL（有限期限），那么除非 timeout == 0，否则都当成无限期处理
//         // 所以会一直等待（利用 yield 和 epc -= 4）
//         // 有没有可能将 timeout != 0 的情况真正实现
//         // 或者退而求其次，将 timeout != 0 的情况视为 timeout == 0，这样可能可以过点
//         // 或许可以参考 AVX 的实现，我懒得找了

//         // 这里最后似乎处理成了所有的 timeout == 0 了
//         if (timeout)
//         {
//             struct TimeSpec ts;
//             copyin(myProcess()->pgdir, (char *)&ts, timeout, sizeof(struct TimeSpec));
//             // 需要立刻结束
//             if (ts.microSecond == 0 && ts.second == 0)
//             {
//                 // printk("directly return\n");
//                 goto selectFinished;
//             }
//             // 需要判断是否结束
//             else
//             {
//                 // 当 timeout 有一定值的时候，我们调度 10 次进行等待
//                 static int times = 10;
//                 if (times)
//                 {
//                     times--;
//                     // printk("still need return %d times\n", times);
//                     tf->epc -= 4;
//                     callYield();
//                 }
//                 else
//                 {
//                     // printk("times == 0, return\n");
//                     times = 10;
//                     goto selectFinished;
//                 }
//             }
//             // copyout(myProcess()->pgdir, read, (char *)&readSet_ready, sizeof(FdSet));
//         }
//         // 对应 timeout == NULL，需要无限期等待
//         else
//         {
//             // printk("timeout == NULL, need wait pernamently\n");
//             tf->epc -= 4;
//             callYield();
//         }
//     }
//     // cnt != 0，此时可以返回
//     else
//     {
//         // 修改了这里
//         goto selectFinished;
//     }
//     return;
// selectFinished:
//     if (read)
//     {
//         copyout(myProcess()->pgdir, read, (char *)&readSet_ready, sizeof(FdSet));
//     }

//     tf->a0 = cnt;
//     return;
// }

void syscallSelect()
{
    Trapframe *tf = getHartTrapFrame();
    // printf("thread %lx get in select, epc: %lx\n", myThread()->id, tf->epc);
    int nfd = tf->a0;
    // assert(nfd <= 128);
    u64 read = tf->a1, write = tf->a2, except = tf->a3, timeout = tf->a4;
    // printk("read: %lx, write: %lx, execept: %lx, timeout: %lx\n", read, write, except, timeout);
    // assert(timeout != 0);
    // cnt 是已经就绪的 fd 的个数
    int cnt = 0;
    struct File *file = NULL;
    // printf("[%s] \n", __func__);

    FdSet readSet_ready;
    if (read)
    {
        FdSet readSet;
        copyin(myProcess()->pgdir, (char *)&readSet, read, sizeof(FdSet));
        readSet_ready = readSet;
        for (int i = 0; i < nfd; i++)
        {
            file = NULL;
            u64 cur = i < 64 ? readSet.bits[0] & (1UL << i) : readSet.bits[1] & (1UL << (i - 64));
            if (!cur)
                continue;
            // printk("selecting read fd %d type %d\n", i, myProcess()->ofile[i]->type);
            file = myProcess()->ofile[i];
            if (!file)
                continue;

            int ready_to_read = 1;
            switch (file->type)
            {
            case FD_PIPE:
                // printk("[select] pipe:%lx nread: %d nwrite: %d\n", file->pipe, file->pipe->nread, file->pipe->nwrite);
                if (file->pipe->nread == file->pipe->nwrite)
                {
                    ready_to_read = 0;
                }
                else
                {
                    ready_to_read = 1;
                }
                break;
            case FD_SOCKET:
                // printf("socketid %d head %d tail %d\n", file->socket-sockets, file->socket->head, file->socket->tail);
                if (file->socket->used != 0 && file->socket->head == file->socket->tail && (!file->socket->listening || (file->socket->listening && file->socket->pending_h == file->socket->pending_t)))
                {
                    ready_to_read = 0;
                }
                else
                {
                    ready_to_read = 1;
                }
                break;
            case FD_DEVICE:
                if (hasChar())
                {
                    ready_to_read = 1;
                }
                else
                {
                    ready_to_read = 0;
                }
                break;
            default:
                ready_to_read = 1;
                break;
            }

            if (ready_to_read)
            {
                ++cnt;
            }
            else
            {
                // printk("fd %d not ready\n", i);
                if (i < 64)
                    readSet_ready.bits[0] &= ~cur;
                else
                    readSet_ready.bits[1] &= ~cur;
            }
        }
        // 修改了这里
        copyout(myProcess()->pgdir, read, (char *)&readSet_ready, sizeof(FdSet));
    }
    // 这里默认写是永远就绪的，应该是不太对的，比如说对于管道，如果 buffer 满了，那么就是不能写的了
    if (write)
    {
        FdSet writeSet;
        copyin(myProcess()->pgdir, (char *)&writeSet, write, sizeof(FdSet));
        for (int i = 0; i < nfd; i++)
        {
            if (i >= 64)
            {
                cnt += !!((1UL << (i - 64)) & writeSet.bits[1]);
            }
            else
            {
                cnt += !!((1UL << (i)) & writeSet.bits[0]);
            }
        }
        copyout(myProcess()->pgdir, write, (char *)&write,
                sizeof(FdSet));
    }

    // 这个默认没有异常
    if (except)
    {
        // FdSet set;
        // copyin(myProcess()->pgdir, (char*)&set, except, sizeof(FdSet));
        u8 zero = 0;
        memsetOut(myProcess()->pgdir, except, zero, nfd);
        // memset(&set, 0, sizeof(FdSet));
        // copyout(myProcess()->pgdir, except, (char*)&set, sizeof(FdSet));
    }

    // 说明还没有可以处理的文件描述符，那么需要考虑等待
    if (cnt == 0)
    {
        // TODO: 按照原先的写法，这里说的是只要 timeout != NULL（有限期限），那么除非 timeout == 0，否则都当成无限期处理
        // 所以会一直等待（利用 yield 和 epc -= 4）
        // 有没有可能将 timeout != 0 的情况真正实现
        // 或者退而求其次，将 timeout != 0 的情况视为 timeout == 0，这样可能可以过点
        // 或许可以参考 AVX 的实现，我懒得找了

        // 这里最后似乎处理成了所有的 timeout == 0 了
        if (timeout)
        {
            // struct TimeSpec ts;
            // copyin(myProcess()->pgdir, (char *)&ts, timeout, sizeof(struct TimeSpec));
            // if (ts.microSecond == 0 && ts.second == 0)
            // {
            //     goto selectFinish;
            // }
            // copyout(myProcess()->pgdir, read, (char *)&readSet_ready, sizeof(FdSet));
        }
        // 这里的 epc -= 4 说的是多次重复执行 syscallSelect,重复检验是否 直到就绪为止，我忘记香老师为啥要注释掉这个了
        // tf->epc -= 4;
        // yield();
        callYield();
    }
    // selectFinish:
    // 原来只在这里有 copyout，是错误的，这里应该冗余了

    // printk("select end cnt %d\n", cnt);
    tf->a0 = cnt;
}

void syscallPRead()
{
    Trapframe *tf = getHartTrapFrame();
    int fd = tf->a0;
    File *file = myProcess()->ofile[fd];
    if (file == 0)
    {
        goto bad;
    }
    u32 off = file->off;
    tf->a0 = metaRead(file->meta, true, tf->a1, tf->a3, tf->a2);
    file->off = off;
    return;
bad:
    tf->a0 = -1;
}

void syscallPWrite()
{
    Trapframe *tf = getHartTrapFrame();
    int fd = tf->a0;
    File *file = myProcess()->ofile[fd];
    if (file == 0)
    {
        goto bad;
    }
    if (file->type != FD_ENTRY)
    {
        panic("Exaros doesn't support this function\n");
    }
    u32 off = file->off;
    tf->a0 = metaWrite(file->meta, true, tf->a1, tf->a3, tf->a2);
    file->off = off;
    return;
bad:
    tf->a0 = -1;
}

void syscallSendFile()
{
    Trapframe *tf = getHartTrapFrame();
    int outFd = tf->a0, inFd = tf->a1;
    if (outFd < 0 || outFd >= NOFILE)
    {
        goto bad;
    }
    if (inFd < 0 || inFd >= NOFILE)
    {
        goto bad;
    }
    File *outFile = myProcess()->ofile[outFd];
    File *inFile = myProcess()->ofile[inFd];
    if (outFile == NULL || inFile == NULL)
    {
        goto bad;
    }
    u32 offset;
    if (tf->a2)
    {
        copyin(myProcess()->pgdir, (char *)&offset, tf->a2, sizeof(u32));
        inFile->off = offset;
    }
    u8 buf[512];
    u32 count = tf->a3, size = 0;
    while (count > 0)
    {
        int len = MIN(count, 512);
        int r = fileread(inFile, false, (u64)buf, len);
        if (r > 0)
            r = filewrite(outFile, false, (u64)buf, r);
        size += r;
        if (r != len)
        {
            break;
        }
        count -= len;
    }
    if (tf->a2)
    {
        copyout(myProcess()->pgdir, tf->a2, (char *)&inFile->off, sizeof(u32));
    }
    tf->a0 = size;
    return;
bad:
    tf->a0 = -1;
    return;
}

void syscallGetresuid()
{
    // 虚假的用户 / 组管理系统
    Trapframe *tf = getHartTrapFrame();
    u64 ruid = tf->a0;
    u64 euid = tf->a1;
    u64 suid = tf->a2;
    int id = 0;
    copyout(myProcess()->pgdir, ruid, (char *)&id, sizeof(int));
    copyout(myProcess()->pgdir, euid, (char *)&id, sizeof(int));
    copyout(myProcess()->pgdir, suid, (char *)&id, sizeof(int));
    tf->a0 = 0;
    return;
}

void syscallGetresgid()
{
    // 虚假的用户 / 组管理系统
    Trapframe *tf = getHartTrapFrame();
    u64 rgid = tf->a0;
    u64 egid = tf->a1;
    u64 sgid = tf->a2;
    int id = 0;
    copyout(myProcess()->pgdir, rgid, (char *)&id, sizeof(int));
    copyout(myProcess()->pgdir, egid, (char *)&id, sizeof(int));
    copyout(myProcess()->pgdir, sgid, (char *)&id, sizeof(int));
    tf->a0 = 0;
    return;
}

void syscallSetpgid()
{
    Trapframe *tf = getHartTrapFrame();
    int pid = tf->a0;
    int pgid = tf->a1;

    if (pid == 0)
    {
        pid = myProcess()->processId;
    }
    if (pgid == 0)
    {
        pgid = pid;
    }

    if (pgid < 0)
    {
        tf->a0 = -EINVAL;
        return;
    }
    tf->a0 = 0;
    return;
}

void syscallGetpgid()
{
    Trapframe *tf = getHartTrapFrame();
    int pid = tf->a0;
    if (pid == 0)
    {
        pid = myProcess()->processId;
    }
    extern ProcessList usedProcesses;
    Process *np = NULL;
    LIST_FOREACH(np, &usedProcesses, link)
    {
        if (np->processId == pid)
        {
            tf->a0 = np->pgid;
            return;
        }
    }
    tf->a0 = -ESRCH;
    return;
}

void syscallGetsid()
{
    Trapframe *tf = getHartTrapFrame();
    int pid = tf->a0;
    int mpid = myProcess()->processId;
    if (pid == 0)
    {
        tf->a0 = mpid;
        return;
    }

    extern ProcessList usedProcesses;
    Process *np = NULL;
    LIST_FOREACH(np, &usedProcesses, link)
    {
        if (np->processId == pid)
        {
            tf->a0 = pid == mpid ? pid : -EPERM;
            return;
        }
    }
    tf->a0 = -ESRCH;
    return;
}

void syscallSetsid()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
    return;
}

void syscallFutex()
{
    Trapframe *tf = getHartTrapFrame();
    int op = tf->a1, val = tf->a2, userVal;
    u64 time = tf->a3;
    u64 uaddr = tf->a0, newAddr = tf->a4;
    struct TimeSpec t;
    op &= (FUTEX_PRIVATE_FLAG - 1);
    switch (op)
    {
    case FUTEX_WAIT:
        // printk("FUTEX_WAIT\n");
        copyin(myProcess()->pgdir, (char *)&userVal, uaddr, sizeof(int));
        if (time)
        {
            if (copyin(myProcess()->pgdir, (char *)&t, time, sizeof(struct TimeSpec)) < 0)
            {
                panic("copy time error!\n");
            }
        }
        if (userVal != val)
        {
            tf->a0 = -1;
            return;
        }
        futexWait(uaddr, myThread(), time ? &t : 0);
        break;
    case FUTEX_WAKE:
        // printk("FUTEX_WAKE\n");
        // printf("val: %d\n", val);
        futexWake(uaddr, val);
        break;
    case FUTEX_REQUEUE:
        // printk("FUTEX_REQUEUE\n");
        // printf("val: %d\n", val);
        futexRequeue(uaddr, val, newAddr);
        break;
    default:
        panic("Futex type not support!\n");
    }
    tf->a0 = 0;
}

void syscallSyslog()
{
    Trapframe *tf = getHartTrapFrame();
    int type = tf->a0;
    u64 bufp = tf->a1;
    u32 len = tf->a2;
    char tmp[] = "Syslog is not important QwQ.";
    switch (type)
    {
    case SYSLOG_ACTION_READ_ALL:
        copyout(myProcess()->pgdir, bufp, (char *)&tmp, MIN(sizeof(tmp), len));
        tf->a0 = MIN(sizeof(tmp), len);
        return;
    case SYSLOG_ACTION_SIZE_BUFFER:
        tf->a0 = sizeof(tmp);
        return;
    case SYSLOG_ACTION_CLOSE:
    case SYSLOG_ACTION_OPEN:
    case SYSLOG_ACTION_READ:
    case SYSLOG_ACTION_READ_CLEAR:
    case SYSLOG_ACTION_CLEAR:
    case SYSLOG_ACTION_CONSOLE_OFF:
    case SYSLOG_ACTION_CONSOLE_ON:
    case SYSLOG_ACTION_CONSOLE_LEVEL:
    case SYSLOG_ACTION_SIZE_UNREAD:
    default:
        panic("unknown syslog type: %d\n", type);
        break;
    }
}

void syscallUmask()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
    return;
}

void syscallSysinfo()
{
    Trapframe *tf = getHartTrapFrame();
    u64 addr = tf->a0;
    struct sysinfo info;
    sysinfo(&info);
    copyout(myProcess()->pgdir, addr, (char *)&info, sizeof(struct sysinfo));
    tf->a0 = 0;
}

void syscallRtSigtimedwait()
{
    TimeSpec ts;
    Trapframe *tf = getHartTrapFrame();
    Process *p = myProcess();
    if (tf->a2)
    {
        copyin(p->pgdir, (char *)&ts, tf->a2, sizeof(TimeSpec));
    }
    SignalSet signalSet;
    copyin(p->pgdir, (char *)&signalSet, tf->a0, tf->a3);
    SignalInfo info;
    copyin(p->pgdir, (char *)&info, tf->a0, sizeof(SignalInfo));
    tf->a0 = rt_sigtimedwait(&signalSet, &info, tf->a2 ? &ts : 0);
}

// TODO struct tms, not TimeSpec
void syscallTimes()
{
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time();
    TimeSpec ts;
    ts.second = time / 1000000;
    ts.microSecond = time % 1000000;
    copyout(myProcess()->pgdir, tf->a1, (char *)&ts, sizeof(TimeSpec));
    tf->a0 = 0;
}

void syscallMadvise()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallMlock()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallMprotect()
{
    Trapframe *tf = getHartTrapFrame();

    u64 start = ALIGN_DOWN(tf->a0, PAGE_SIZE);
    u64 end = ALIGN_UP(tf->a0 + tf->a1, PAGE_SIZE);

    u64 perm = 0;
    if (PROT_EXEC(tf->a2)) { perm |= PTE_EXECUTE_BIT | PTE_READ_BIT; }
    if (PROT_READ(tf->a2)) { perm |= PTE_READ_BIT; }
    if (PROT_WRITE(tf->a2)) { perm |= PTE_WRITE_BIT | PTE_READ_BIT; }
    while (start < end)
    {
        u64 *pte;
        Page *page;
        page = pageLookup(myProcess()->pgdir, start, &pte);
        if (page == NULL)
        {
            passiveAlloc(myProcess()->pgdir, start);
            page = pageLookup(myProcess()->pgdir, start, &pte); // CHL_CHANGED
        }
        *pte = (*pte & ~(PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT)) | perm; // CHL_CHANGED
        // else
        // {
        //     *pte = (*pte & ~(PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT)) | perm;
        // }
        start += PAGE_SIZE;
    }
    tf->a0 = 0;
}

void syscallMsync()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallReadlinkat()
{
    Trapframe *tf = getHartTrapFrame();
    int dirFd = tf->a0;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    tf->a0 = -1;
    DirMeta *entryPoint = metaName(dirFd, path, false);
    if (entryPoint == NULL || entryPoint->reserve != DT_LNK)
    {
        goto bad;
    }
    // char kbuf[FAT32_MAX_FILENAME];
    // eread(entryPoint, false, (u64)kbuf, 0, entryPoint->file_size);
    // ewrite(entryPoint, true, (u64)buf, 0, MIN(size, sizeof(kbuf)));
    tf->a0 = 0;
    return;
bad:
    tf->a0 = -1;
}

// void syscallRenameat2()
// {
//     Trapframe *tf = getHartTrapFrame();
// #ifndef DEBUG_FS
//     int oldFd = tf->a0, newFd = tf->a2;
//     char old[FAT32_MAX_PATH], new[FAT32_MAX_PATH];
//     if (argstr(1, old, FAT32_MAX_PATH) < 0 || argstr(3, new, FAT32_MAX_PATH) < 0)
//     {
//         tf->a0 = -1;
//         return;
//     }

//     DirMeta *src = NULL, *dst = NULL, *pdst = NULL;
//     int srclock = 0;
//     char *name;
//     if ((src = ename(oldFd, old, true)) == NULL || (pdst = enameparent(newFd, new, old)) == NULL || (name = formatname(old)) == NULL)
//     {
//         goto fail; // src doesn't exist || dst parent doesn't exist || illegal
//                    // new name
//     }
//     for (DirMeta *ep = pdst; ep != NULL; ep = ep->parent)
//     {
//         if (ep == src)
//         { // In what universe can we move a directory into its child?
//             goto fail;
//         }
//     }

//     u32 off;
//     elock(src); // must hold child's lock before acquiring parent's, because we
//                 // do so in other similar cases
//     srclock = 1;
//     elock(pdst);
//     dst = dirlookup(pdst, name, &off);
//     if (dst != NULL)
//     {
//         eunlock(pdst);
//         if (src == dst)
//         {
//             goto fail;
//         }
//         else if (src->attribute & dst->attribute & ATTR_DIRECTORY)
//         {
//             if (!isDirEmpty(dst))
//             { // it's ok to overwrite an empty dir
//                 eunlock(dst);
//                 goto fail;
//             }
//         }
//         else
//         { // src is not a dir || dst exists and is not an dir
//             goto fail;
//         }
//     }

//     if (dst)
//     {
//         eremove(dst);
//         eunlock(dst);
//     }
//     memmove(src->filename, name, FAT32_MAX_FILENAME);
//     emake(pdst, src, off);
//     if (src->parent != pdst)
//     {
//         eunlock(pdst);
//         elock(src->parent);
//     }
//     eremove(src);
//     eunlock(src->parent);
//     DirMeta *psrc = src->parent; // src must not be root, or it won't
//                                  // pass the for-loop test
//     src->parent = edup(pdst);
//     src->off = off;
//     src->valid = 1;
//     eunlock(src);

//     eput(psrc);
//     if (dst)
//     {
//         eput(dst);
//     }
//     eput(pdst);
//     eput(src);
// #endif
//     tf->a0 = 0;
//     return;

// #ifndef DEBUG_FS
// fail:
//     tf->a0 = -1;
//     return;
// #endif
// }

void syscallStatfs()
{
    Trapframe *tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    if (argstr(0, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    FileSystemStatus fss;
    memset(&fss, 0, sizeof(FileSystemStatus));
    tf->a0 = getFsStatus(path, &fss);
    if (tf->a0 == 0)
    {
        // printf("bjoweihgre8i %ld\n", tf->a1);
        copyout(myProcess()->pgdir, tf->a1, (char *)&fss, sizeof(FileSystemStatus));
    }
}

void syscallFsync()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallPrlimit64()
{
    Trapframe *tf = getHartTrapFrame();
    u64 pid = tf->a0, resouce = tf->a1, newVa = tf->a2, oldVa = tf->a3;
    if (pid)
    {
        panic("Resource limit not current process!\n");
    }
    // printf("resource limit: pid: %lx, resource: %d, new: %lx, old: %lx\n", pid, resouce, newVa, oldVa);
    struct ResourceLimit newLimit;
    Process *process = myProcess();
    if (newVa && copyin(process->pgdir, (char *)&newLimit, newVa, sizeof(struct ResourceLimit)) < 0)
    {
        tf->a0 = -1;
    }
    switch (resouce)
    {
    case RLIMIT_NOFILE:
        if (newVa)
        {
            process->fileDescription.hard = newLimit.hard;
            process->fileDescription.soft = newLimit.soft;
        }
        if (oldVa)
        {
            copyout(process->pgdir, oldVa, (char *)&process->fileDescription, sizeof(struct ResourceLimit));
        }
        break;
    }
    // releaseLock(&process->lock);
    tf->a0 = 0;
}

void syscallSocket()
{
    // printk("syscall socket begin.\n");
    Trapframe *tf = getHartTrapFrame();
    int domain = tf->a0, type = tf->a1, protocal = tf->a2;
    tf->a0 = createSocket(domain, type, protocal);
    // printf("[%s] socket at fd = %d\n",__func__, tf->a0);
    // printk("syscall socket end.\n");
}

void syscallBind()
{
    // printk("syscall bind begin.\n");
    Trapframe *tf = getHartTrapFrame();
    // printk("len : %d, sizeof(SocketAddr) is %d\n", tf->a2, sizeof(SocketAddr));
    // wrong: assert(tf->a2 == sizeof(SocketAddr));
    SocketAddr sa;
    copyin(myProcess()->pgdir, (char *)&sa, tf->a1, tf->a2);
    tf->a0 = bindSocket(tf->a0, &sa);
    // printk("syscall bind end.\n");
}

void syscallGetSocketName()
{
    // printk("syscall get socket name begin.\n");
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = getSocketName(tf->a0, tf->a1);
    // printk("syscall get socket name end.\n");
}

void syscallSetSocketOption()
{
    // printk("syscall set socket option begin.\n");
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
    // printk("syscall set socket option end.\n");
}

void syscallFchmodAt()
{
    // printk("syscall FchmodAt begin.\n");
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
    // printk("syscall FchmodAt end.\n");
}

void syscallSendTo()
{
    // printk("syscall sentTo begin.\n");
    static char buf[PAGE_SIZE];
    Trapframe *tf = getHartTrapFrame();
    // assert(tf->a5 == sizeof(SocketAddr));
    SocketAddr sa;
    copyin(myProcess()->pgdir, (char *)&sa, tf->a4, tf->a5);
    u32 len = MIN(tf->a2, PAGE_SIZE);
    copyin(myProcess()->pgdir, buf, tf->a1, len);
    tf->a0 = sendTo(myProcess()->ofile[tf->a0]->socket, buf, tf->a2, tf->a3, &sa);
    // printk("syscall sentTo end.\n");
}

void syscallReceiveFrom()
{
    // printk("syscall receiveFrom begin.\n");
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = receiveFrom(myProcess()->ofile[tf->a0]->socket, tf->a1, tf->a2, tf->a3, tf->a4);
    // printk("syscall receiveFrom end.\n");
}

void syscallListen()
{
    // printk("syscall listen begin.\n");
    Trapframe *tf = getHartTrapFrame();
    int sockfd = tf->a0;
    // printf("[%s] fd %d\n",__func__, sockfd);
    tf->a0 = listen(sockfd);
    // printk("syscall listen end.\n");
}

void syscallConnect()
{
    // printk("syscall connect begin.\n");
    Trapframe *tf = getHartTrapFrame();
    int sockfd = tf->a0;
    SocketAddr sa;
    // printk("address is 0x%lx, addr len is %d, %x\n", tf->a1, tf->a2, sa.port);
    copyin(myProcess()->pgdir, (char *)&sa, tf->a1, tf->a2);
    // printk("address is 0x%lx, addr len is %d, %x\n", tf->a1, tf->a2, sa.port);
    tf->a0 = connect(sockfd, &sa);
    // printk("syscall connect end.\n");
}

void syscallAccept()
{
    // printk("syscall accept begin.\n");
    Trapframe *tf = getHartTrapFrame();
    int sockfd = tf->a0;
    SocketAddr sa;
    tf->a0 = accept(sockfd, &sa);
    copyout(myProcess()->pgdir, tf->a1, (char *)&sa, sizeof(sa));
    // printk("syscall accept end.\n");
}

/**
 * @brief 使用 lseek 系统调用可以改变文件的当前文件偏移量（current file offset，cfo）
 * @param fd 文件描述符
 * @param offset 特定的偏移量，可正可负
 * @param mode SEEK_SET，文件偏移量将被设置为 offset
 *             SEEK_CUR，文件偏移量将被设置为 cfo 加上 offset
 *             SEEK_END，文件偏移量将被设置为文件长度加上 offset
 *
 * @return 改变后的 cfo
 */
void syscallLseek()
{
    Trapframe *tf = getHartTrapFrame();
    int fd = tf->a0, mode = tf->a2;
    u64 offset = tf->a1;
    if (fd < 0 || fd >= NOFILE)
    {
        goto bad;
    }
    File *file = myProcess()->ofile[fd];
    if (file == 0)
    {
        goto bad;
    }
    u64 off = offset;
    switch (mode)
    {
    case SEEK_SET:
        break;
    case SEEK_CUR:
        if (file->type == FD_TMPFILE)
        {
            off += file->tmpfile->offset;
        }
        else if (file->type == FD_INTERPFILE)
        {
            off += file->interpfile->offset;
        }
        // 对应的是 FD_ENTRY 的情况
        else
        {
            off += file->off;
        }
        break;
    case SEEK_END:
        if (file->type == FD_ENTRY)
        {
            off += file->meta->fileSize;
        }
        else if (file->type == FD_PIPE)
        {
            off += file->pipe->nwrite % PIPESIZE;
        }
        else if (file->type == FD_TMPFILE)
        {
            off += file->tmpfile->fileSize;
            TMPF_DEBUG("lseek the offset at the end %d\n", off);
        }
        else if (file->type == FD_INTERPFILE)
        {
            off += file->interpfile->fileSize;
        }
        else
        {
            panic("Exaros doesn't support this file seek end\n");
        }
        break;
    default:
        goto bad;
    }
    // if (file->type == FD_ENTRY && off > file->ep->file_size) {
    //     char zero = 0;
    //     file->off = file->ep->file_size;
    //     for (int i = file->ep->file_size; i < off; i++) {
    //         filewrite(file, false, (u64)&zero, 1);
    //     }
    // }
    file->off = off;
    if (file->type == FD_TMPFILE)
    {
        file->tmpfile->offset = off;
    }
    else if (file->type == FD_INTERPFILE)
    {
        file->interpfile->offset = off;
    }
    // if (file->type != FD_ENTRY) {
    //     file->off = off;
    // } else {
    //     file->off = (off >= file->ep->file_size ? file->ep->file_size : off);
    // }
    tf->a0 = off;
    return;
bad:
    tf->a0 = -1;
}

void tmpfileUtimensat(Tmpfile *tmpfile)
{
    Trapframe *tf = getHartTrapFrame();
    if (tf->a2)
    {
        TimeSpec ts[2];
        copyin(myProcess()->pgdir, (char *)ts, tf->a2, sizeof(ts));
        tmpfileSetTime(tmpfile, ts);
    }
}

void syscallUtimensat()
{
    Trapframe *tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    int dirFd = tf->a0;
    DirMeta *de;
    if (tf->a1)
    {
        if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
        {
            tf->a0 = -1;
            return;
        }
        if ((dirFd < 0 && dirFd != AT_FDCWD) || dirFd >= NOFILE)
        {
            tf->a0 = -EBADF;
            return;
        }
        // if ((de = metaName(dirFd, path, true)) == NULL)
        // {
        //     tf->a0 = -ENOTDIR;
        //     return;
        // }
        if ((i64)(de = metaNamePatch(dirFd, path, true)) < 0)
        {
            // printk("eno: %ld\n", (u64)de);
            tf->a0 = (u64)de;
            return;
        }
    }
    else
    {
        File *f;
        if (dirFd < 0 || dirFd >= NOFILE || (f = myProcess()->ofile[dirFd]) == NULL)
        {
            tf->a0 = -EBADF;
            return;
        }
        de = f->meta;
        if (f->type == FD_TMPFILE)
        {
            TMPF_DEBUG("tmpfile utimensat\n");
            // FIXME: 这是一种特殊情况，并没有很好的考虑所有的特殊情况，比如前面的那一种
            // 关闭 tmpfile 不是很困难，只需要在 open 的时候避免 tmfile 的生成即可
            tmpfileUtimensat(f->tmpfile);
            tf->a0 = 0;
            return;
        }
    }

    // 用于设置时间
    if (tf->a2)
    {
        TimeSpec ts[2];
        copyin(myProcess()->pgdir, (char *)ts, tf->a2, sizeof(ts));
        eSetTime(de, ts);
    }
    tf->a0 = 0;
}

void syscallAccess()
{
    Trapframe *tf = getHartTrapFrame();
    int dirfd = tf->a0;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0)
    {
        tf->a0 = -1;
        return;
    }
    DirMeta *entryPoint = metaName(dirfd, path, true);
    if (entryPoint == NULL)
    {
        tf->a0 = -ENOENT;
        return;
    }
    tf->a0 = 0;
}

void syscallMemoryProtect()
{
    Trapframe *tf = getHartTrapFrame();
    // printf("mprotect va: %lx, length: %lx prot:%lx\n", tf->a0, tf->a1, tf->a2);

    u64 start = ALIGN_DOWN(tf->a0, PAGE_SIZE);
    u64 end = ALIGN_UP(tf->a0 + tf->a1, PAGE_SIZE);

    u64 perm = 0;
    if (tf->a2 & PROT_EXEC_BIT)
    {
        perm |= PTE_EXECUTE_BIT | PTE_READ_BIT;
    }
    if (tf->a2 & PROT_READ_BIT)
    {
        perm |= PTE_READ_BIT;
    }
    if (tf->a2 & PROT_WRITE_BIT)
    {
        perm |= PTE_WRITE_BIT | PTE_READ_BIT;
    }
    while (start < end)
    {
        u64 *pte;
        Page *page;
        page = pageLookup(myProcess()->pgdir, start, &pte);
        if (!page)
        {
            passiveAlloc(myProcess()->pgdir, start);
        }
        else
        {
            *pte = (*pte & ~(PTE_READ_BIT | PTE_WRITE_BIT | PTE_EXECUTE_BIT)) | perm;
        }
        start += PAGE_SIZE;
    }
    tf->a0 = 0;
}

void syscallSchedSetscheduler()
{
    Trapframe *tf = getHartTrapFrame();
    u64 pid = tf->a0;
    int policy = tf->a1;
    u64 addr = tf->a2;
    sched_param param;
    copyin(myProcess()->pgdir, (char *)&param, addr, sizeof(sched_param));
    tf->a0 = sched_setscheduler(pid, policy, &param);
    return;
}

void syscallSchedGetscheduler()
{
    Trapframe *tf = getHartTrapFrame();
    u64 pid = tf->a0;
    tf->a0 = sched_getscheduler(pid);
    return;
}

void syscallSchedGetparam()
{
    Trapframe *tf = getHartTrapFrame();
    u64 pid = tf->a0;
    u64 addr = tf->a1;
    sched_param param;
    tf->a0 = sched_getparam(pid, &param);
    copyout(myProcess()->pgdir, addr, (char *)&param, sizeof(sched_param));
    return;
}

void syscallSchedSetaffinity()
{
    Trapframe *tf = getHartTrapFrame();
    u64 pid = tf->a0;
    u64 cpusetsize = tf->a1;
    u64 addr = tf->a2;
    cpu_set_t cpuset;
    copyin(myProcess()->pgdir, (char *)&cpuset, addr, cpusetsize);
    tf->a0 = sched_setaffinity(pid, cpusetsize, &cpuset);
    return;
}

void syscallSchedGetaffinity()
{
    Trapframe *tf = getHartTrapFrame();
    u64 pid = tf->a0;
    u64 cpusetsize = tf->a1;
    u64 addr = tf->a2;
    cpu_set_t cpuset;
    tf->a0 = sched_getaffinity(pid, cpusetsize, &cpuset);
    copyout(myProcess()->pgdir, addr, (char *)&cpuset, cpusetsize);
    return;
}
void syscallSync()
{
    Trapframe *tf = getHartTrapFrame();
    asm volatile("fence iorw, iorw");
    tf->a0 = 0;
}

void syscallFtruncate()
{
    Trapframe *tf = getHartTrapFrame();
    File *f;
    int fd = tf->a0;
    int len = tf->a1;

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

    tf->a0 = fileTrunc(f, len);
}

void syscallSHMGet()
{
    // int shmid = shmget(key_t key, size_t size, int shmflg);
    Trapframe *tf = getHartTrapFrame();
    // key_t 是 s32
    int key = tf->a0;
    u64 size = tf->a1;
    int shmflg = tf->a2;

    tf->a0 = shmAlloc(key, size, shmflg);
}

void syscallSHMCtrl()
{
    Trapframe *tf = getHartTrapFrame();
    if (tf->a0 == 114)
    {
        printk("syscall SHMCtrl");
        long long *cp = (long long *)0xf00002000;
        printk("flag is %ld\n", *cp);
        long long userFlag;
        copyin(myProcess()->pgdir, (char *)&userFlag, 0x3bfffff000, sizeof(long long));
        printk("user flag is %ld\n", userFlag);
    }
    tf->a0 = 0;
}

void syscallSHMAt()
{
    // void* shmaddr = void *shmat(int shmid, const void *shmaddr, int shmflg);
    Trapframe *tf = getHartTrapFrame();
    int shmid = tf->a0;
    u64 shmaddr = tf->a1;
    int shmflg = tf->a2;
    tf->a0 = shmAt(shmid, shmaddr, shmflg);
    // char *src = "hello, world.\n";
    // copyout(myProcess()->pgdir, tf->a0, src, 10);
    // char *cp = (char *)SHM_BASE;
    // for (int i = 0; i < 10; i++)
    // {
    //     printk("%c", cp[i]);
    // }
    // panic("");
}

void syscallSHMDt()
{
    Trapframe *tf = getHartTrapFrame();
    SHM_DEBUG("hello\n");
    tf->a0 = 0;
}

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN (-1)
#define RUSAGE_THREAD 1
struct timespec2
{
    long tv_sec;  // seconds
    long tv_nsec; // nanoseconds
};
struct rusage
{
    struct timespec2 ru_utime; /* user CPU time used */
    struct timespec2 ru_stime; /* system CPU time used */
    long ru_maxrss;            /* maximum resident set size */
    long ru_ixrss;             /* integral shared memory size */
    long ru_idrss;             /* integral unshared data size */
    long ru_isrss;             /* integral unshared stack size */
    long ru_minflt;            /* page reclaims (soft page faults) */
    long ru_majflt;            /* page faults (hard page faults) */
    long ru_nswap;             /* swaps */
    long ru_inblock;           /* block input operations */
    long ru_oublock;           /* block output operations */
    long ru_msgsnd;            /* IPC messages sent */
    long ru_msgrcv;            /* IPC messages received */
    long ru_nsignals;          /* signals received */
    long ru_nvcsw;             /* voluntary context switches */
    long ru_nivcsw;            /* involuntary context switches */
};
void syscallGetrusage()
{
    Trapframe *tf = getHartTrapFrame();
    // int who;
    // u64 addr;
    // struct rusage rs;

    // if (argint(0, &who) < 0)
    // {
    //     tf->a0 = -1;
    //     return;
    // }

    // if (argaddr(1, &addr) < 0)
    // {
    //     tf->a0 = -1;
    //     return;
    // }

    // rs.ru_utime = (struct timespec2){myProcess()->utime, 0};
    // rs.ru_stime = (struct timespec2){myProcess()->ktime, 0};

    // switch (who)
    // {
    // case RUSAGE_SELF:
    // case RUSAGE_THREAD:
    //     // TODO
    //     rs.ru_nvcsw = 0;
    //     rs.ru_nivcsw = 0;
    //     break;
    // default:
    //     break;
    // }
    // if (either_copyout(1, addr, &rs, sizeof(rs)) < 0)
    // {
    //     tf->a0 = -1;
    //     return;
    // }
    tf->a0 = 0;
}

/* 虚假的 clock_nanosleep */
void syscallClockNanosleep()
{
    Trapframe *tf = getHartTrapFrame();
    int clock = tf->a0;
    int flags = tf->a1;
    u64 ts_addr = tf->a2;
    // u64 remain_addr = tf->a2;
    RealTimeSpec ts;
    if (clock != CLOCK_REALTIME)
    {
        // printk("unsupported clock %d\n", clock);
        // tf->a0 = -1;
    }
    copyin(myProcess()->pgdir, (char *)&ts, ts_addr, sizeof(RealTimeSpec));
    if (flags == 0)
    {
        myThread()->awakeTime = r_time() + ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
        kernelProcessCpuTimeEnd();
        tf->a0 = 0;
        callYield();
    }
    else if (flags == 1)
    {
        if (r_time() > ts.tv_sec * 1000000 + ts.tv_nsec / 1000)
        {
            tf->a0 = 0;
            return;
        }
        myThread()->awakeTime = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
        kernelProcessCpuTimeEnd();
        tf->a0 = 0;
        callYield();
    }
    else
    {
        printk("unsupported flag\n");
        tf->a0 = -1;
    }
}

void syscallRenameat2()
{
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = -1;
}
