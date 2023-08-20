#include <interpfile.h>
#include <driver.h>
#include <file.h>
#include <trap.h>
#include <process.h>

extern char interruptsString[];
InterpFile interpfile;

void interpfileAlloc()
{
    interpfile.used = true;
    interpfile.fileSize = 0;
    interpfile.offset = 0;
}

InterpFile *interpfileName(char *path)
{
    if (interpfile.used)
    {
        return &interpfile;
    }
    else
    {
        return NULL;
    }
}

int interpfileRead(bool isUser, u64 dst, u32 n)
{
    interpfile.fileSize = updateInterruptsString();
    if (interpfile.offset + n > interpfile.fileSize)
    {
        n = interpfile.fileSize - interpfile.offset;
    }
    copyout(myProcess()->pgdir, dst, interruptsString + interpfile.offset, n);

    return n;
}

int interpfileWrite()
{
    return -1;
}

void tmpfileStat(struct kstat *st)
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