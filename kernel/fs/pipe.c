#include <pipe.h>

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

int pipeNew(struct File **f0, struct File **f1)
{
    Pipe *pi;

    pi = 0;
    *f0 = *f1 = 0;
    if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
        goto bad;
    pipeAlloc(&pi);
    pi->readopen = 1;
    pi->writeopen = 1;
    initLock(&pi->lock, "pipe");
    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = pi;
    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = pi;
    return 0;

bad:
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    return -1;
}

void pipeClose(Pipe *pi, int writable)
{
    // acquireLock(&pi->lock);
    // printf("%x %x %x\n", pi->writeopen, pi->readopen, writable);
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
        // releaseLock(&pi->lock);
        pipeFree(pi);
    }
    // } else
    // releaseLock(&pi->lock);
}

void pipeOut(bool isUser, u64 dstva, char *src);
void pipeIn(bool isUser, char *dst, u64 srcva);

int pipeWrite(Pipe *pi, bool isUser, u64 addr, int n)
{
    int i = 0, cow;

    u64 *pageTable = myProcess()->pgdir;
    u64 pa = addr;
    if (isUser)
    {
        pa = vir2phy(pageTable, addr, &cow);
        if (pa == NULL)
        {
            cow = 0;
            pa = pageout(pageTable, addr);
        }
        if (cow)
        {
            pa = cowHandler(pageTable, addr);
            // pa = vir2phy(pageTable, addr, NULL);
        }
    }

    // acquireLock(&pi->lock);
    while (i < n)
    {
        if (pi->readopen == 0 /*|| pr->killed*/)
        {
            // releaseLock(&pi->lock);
            panic("");
            return -1;
        }
        if (pi->nwrite == pi->nread + PIPESIZE)
        { // DOC: pipewrite-full
            wakeup(&pi->nread);
            sleep(&pi->nwrite, &pi->lock);
        }
        else
        {
            char ch;
            // if (either_copyin(&ch, isUser, addr + i, 1) == -1) {
            //     break;
            // }
            // pipeIn(isUser, &ch, addr + i);
            ch = *((char *)pa);
            pi->data[(pi->nwrite++) & (PIPESIZE - 1)] = ch;
            i++;
            if (isUser && (!((addr + i) & (PAGE_SIZE - 1))))
            {
                pa = vir2phy(pageTable, addr + i, &cow);
                if (pa == NULL)
                {
                    cow = 0;
                    pa = pageout(pageTable, addr + i);
                }
                if (cow)
                {
                    pa = cowHandler(pageTable, addr);
                    // pa = vir2phy(pageTable, addr, NULL);
                }
            }
            else
            {
                pa++;
            }
        }
    }
    wakeup(&pi->nread);
    // releaseLock(&pi->lock);
    assert(i != 0);
    return i;
}

int pipeRead(Pipe *pi, bool isUser, u64 addr, int n)
{
    int i;
    char ch;
    u64 *pageTable = myProcess()->pgdir;
    u64 pa = addr;
    if (isUser)
    {
        pa = vir2phy(pageTable, addr, NULL);
        if (pa == NULL)
        {
            pa = pageout(pageTable, addr);
        }
    }

    // acquireLock(&pi->lock);
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
            pa = vir2phy(pageTable, addr + i, NULL);
            if (pa == NULL)
            {
                pa = pageout(pageTable, addr + i);
            }
        }
        else
        {
            pa++;
        }
        // if (either_copyout(isUser, addr + i, &ch, 1) == -1) {
        //     break;
        // }
        // pipeOut(isUser, addr + i, &ch);
    }
    wakeup(&pi->nwrite); // DOC: piperead-wakeup
    // releaseLock(&pi->lock);
    return i;
}
