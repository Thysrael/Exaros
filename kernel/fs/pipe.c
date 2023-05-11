#include <pipe.h>
#include <file.h>
#include <process.h>

Pipe pipeBuffer[MAX_PIPE];
u64 pipeBitMap[MAX_PIPE / 64];

/**
 * @brief 分配一个 pipe，本质是遍历 bitmap 来获得空闲 pipe
 *
 * @param p 管道
 * @return int 成功为 0
 */
int pipeAlloc(Pipe **p)
{
    for (int i = 0; i < MAX_PIPE / 64; i++)
    {
        if (~pipeBitMap[i])
        {
            int bit = LOW_BIT64(~pipeBitMap[i]);
            pipeBitMap[i] |= (1UL << bit);
            *p = &pipeBuffer[(i << 6) | bit];
            (*p)->nread = (*p)->nwrite = 0;
            return 0;
        }
    }
    panic("no pipe!");
}

/**
 * @brief 释放一个 pipe
 *
 * @param p 待释放的 pipe
 */
void pipeFree(Pipe *p)
{
    int off = p - pipeBuffer;
    pipeBitMap[off >> 6] &= ~(1UL << (off & 63));
}

/**
 * @brief 配一个管道（pipe）的文件描述符（file descriptor），并将它们存储在 f0 和 f1 两个指针中
 *
 * @param f0 管道的输出，只能读
 * @param f1 管道的输入，只能写
 * @return int 成功为 0，失败为 -1
 */
int pipeNew(File **f0, File **f1)
{
    Pipe *pi;

    pi = 0;
    *f0 = *f1 = 0;
    if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
        goto bad;
    pipeAlloc(&pi);
    pi->readopen = 1;
    pi->writeopen = 1;
    // f0 是读入端
    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = pi;
    // f1 是写入端
    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = pi;
    return 0;
// 如果分配没有成功，需要将各种分配好的空间释放掉
bad:
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    return -1;
}

/**
 * @brief 关闭管道的一侧，如果两侧均关闭，那么就关闭整个管道
 *
 * @param pi 管道
 * @param writable 是否是写侧
 */
void pipeClose(Pipe *pi, int writable)
{
    if (writable)
    {
        pi->writeopen = 0;
        wakeup(&pi->nread);
    }
    else
    {
        pi->readopen = 0;
        wakeup(&pi->nwrite);
    }
    if (pi->readopen == 0 && pi->writeopen == 0)
    {
        pipeFree(pi);
    }
}

/**
 * @brief 向管道写入内容
 *
 * @param pi 管道
 * @param addr 写入的内容的地址，是一个用户地址
 * @param n 写入的字节数
 * @return int 实际写入的字节数
 */
int pipeWrite(Pipe *pi, bool isUser, u64 addr, int n)
{
    int i = 0, cow;

    u64 *pageTable = myProcess()->pgdir;
    u64 pa = addr;
    if (isUser)
    {
        pa = va2PA(pageTable, addr, &cow);
        if (pa == NULL)
        {
            cow = 0;
            pa = pageout(pageTable, addr);
        }
        if (cow)
        {
            pa = cowHandler(pageTable, addr);
        }
    }

    while (i < n)
    {
        if (pi->readopen == 0)
        {
            panic("");
            return -1;
        }
        // DOC: pipewrite-full
        if (pi->nwrite == pi->nread + PIPESIZE)
        {
            wakeup(&pi->nread);
            sleep(&pi->nwrite, &pi->lock);
        }
        else
        {
            char ch;
            ch = *((char *)pa);
            pi->data[(pi->nwrite++) & (PIPESIZE - 1)] = ch;
            i++;
            if (isUser && (!((addr + i) & (PAGE_SIZE - 1))))
            {
                pa = va2PA(pageTable, addr + i, &cow);
                if (pa == NULL)
                {
                    cow = 0;
                    pa = pageout(pageTable, addr + i);
                }
                if (cow)
                {
                    pa = cowHandler(pageTable, addr);
                    // pa = va2PA(pageTable, addr, NULL);
                }
            }
            else
            {
                pa++;
            }
        }
    }
    wakeup(&pi->nread);
    assert(i != 0);
    return i;
}

/**
 * @brief 从管道中读出内容
 *
 * @param pi 管道
 * @param addr 读出内容存放的地址，是一个用户地址
 * @param n 内容大小
 * @return int
 */
int pipeRead(Pipe *pi, bool isUser, u64 addr, int n)
{
    int i;
    char ch;
    u64 *pageTable = myProcess()->pgdir;
    u64 pa = addr;
    if (isUser)
    {
        pa = va2PA(pageTable, addr, NULL);
        if (pa == NULL)
        {
            pa = pageout(pageTable, addr);
        }
    }

    while (pi->nread == pi->nwrite && pi->writeopen)
    {                                 // DOC: pipe-empty
        sleep(&pi->nread, &pi->lock); // DOC: piperead-sleep
    }
    for (i = 0; i < n;)
    { // DOC: piperead-copy
        if (pi->nread == pi->nwrite)
        {
            break;
        }
        ch = pi->data[(pi->nread++) & (PIPESIZE - 1)];
        *((char *)pa) = ch;
        i++;
        if (isUser && (!((addr + i) & (PAGE_SIZE - 1))))
        {
            pa = va2PA(pageTable, addr + i, NULL);
            if (pa == NULL)
            {
                pa = pageout(pageTable, addr + i);
            }
        }
        else
        {
            pa++;
        }
    }
    wakeup(&pi->nwrite); // DOC: piperead-wakeup
    return i;
}
