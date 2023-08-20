#include <interpfile.h>
#include <driver.h>
#include <file.h>
#include <trap.h>
#include <process.h>

extern char interruptsString[];
InterpFile interpfile;

void interpfileAlloc()
{
    // printk("interp alloc\n");
    interpfile.used = true;
    interpfile.fileSize = 0;
    interpfile.offset = 0;
}

InterpFile *interpfileName(char *path)
{
    interpfile.fileSize = 0;
    interpfile.offset = 0;
    return &interpfile;
}

void interpfileClose()
{
}

int interpfileRead(bool isUser, u64 dst, u32 n)
{
    // printk("[interpfileRead] expect read len is %d\n", n);
    interpfile.fileSize = updateInterruptsString();
    // printk("[interpfileRead] interpfile size is %d, interpfile offset is %d", interpfile.fileSize, interpfile.offset);
    if (interpfile.offset + n > interpfile.fileSize)
    {
        n = interpfile.fileSize - interpfile.offset;
    }
    // printk("[interpfileRead] acutal read len is %d\n", n);
    copyout(myProcess()->pgdir, dst, interruptsString + interpfile.offset, n);
    interpfile.offset += n;
    return n;
}

int interpfileWrite()
{
    return -1;
}

void interpfileStat(struct kstat *st)
{
    st->st_dev = 0; // DEV_SD = 0
    st->st_size = interpfile.fileSize;
    st->st_ino = 0;
    st->st_mode = REG_TYPE;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0; // What's this?
    st->st_blksize = 512;
    st->st_blocks = st->st_size / st->st_blksize;
    st->st_atime_sec = 0;
    st->st_atime_nsec = 0;
    st->st_mtime_sec = 0;
    st->st_mtime_nsec = 0;
    st->st_ctime_sec = 0;
    st->st_ctime_nsec = 0;
}