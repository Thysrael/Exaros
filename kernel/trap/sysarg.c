#include <process.h>
#include <types.h>
#include <sysarg.h>
#include <driver.h>
#include <string.h>
#include <memory.h>
#include <file.h>
#include <trap.h>

/**
 * @brief 从第 n 个参数寄存器中取得 fd 的值，并找到对应的 File 结构体
 *
 * @param n 第 n 个参数寄存器
 * @param pfd fd 指针
 * @param pf File 指针
 * @return int 成功为 0， 失败 -1
 */
int argfd(int n, int *pfd, File **pf)
{
    int fd;
    File *f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

/**
 * @brief 从特定地址取出一个 u64 来
 *
 * @param addr 地址
 * @param ip
 * @return int
 */
int fetchaddr(u64 addr, u64 *ip)
{
    struct Process *p = myProcess();
    if (copyin(p->pgdir, (char *)ip, addr, sizeof(*ip)) != 0)
        return -1;
    return 0;
}

/**
 * @brief 从当前进程的的 uva 地址取出最大长度为 max 的字符串并保存在 buf 中
 *
 * @param uva 用户地址
 * @param buf 字符串目的地址
 * @param max 最大长度
 * @return int 字符串长度或者为 - 1
 */
int fetchstr(u64 uva, char *buf, int max)
{
    struct Process *p = myProcess();
    int err = copyInstr(p->pgdir, buf, uva, max);
    if (err < 0)
        return err;
    return strlen(buf);
}

/**
 * @brief 获得参数寄存器中的值（a0 ~ a5）
 *
 * @param n an
 * @return u64 寄存器中的值
 */
static u64 argraw(int n)
{
    Trapframe *trapframe = getHartTrapFrame();
    switch (n)
    {
    case 0:
        return trapframe->a0;
    case 1:
        return trapframe->a1;
    case 2:
        return trapframe->a2;
    case 3:
        return trapframe->a3;
    case 4:
        return trapframe->a4;
    case 5:
        return trapframe->a5;
    }
    panic("argraw");
    return -1;
}

/**
 * @brief 取得 an 中的寄存器的值，并保存在 ip 中
 *
 * @param n 第 n 号 a 寄存器
 * @param ip 寄存器中的值的指针
 * @return int 应该是是否成功
 */
int argint(int n, int *ip)
{
    *ip = argraw(n);
    return 0;
}

/**
 * @brief 读取参数寄存器中的地址
 *
 * @param n 第 n 号 a 寄存器
 * @param ip 寄存器中的值的指针
 * @return int 应该是是否成功
 */
int argaddr(int n, u64 *ip)
{
    *ip = argraw(n);
    return 0;
}

/**
 * @brief 从第 n 个参数寄存器中取出字符串地址，并将地址指向的字符串拷贝到 buf 中，最大长度为 max
 *
 * @param n 第 n 个参数寄存器
 * @param buf 字符串内容 buf
 * @param max 最大长度
 * @return int 字符串长度
 */
int argstr(int n, char *buf, int max)
{
    u64 addr;
    if (argaddr(n, &addr) < 0)
        return -1;
    return fetchstr(addr, buf, max);
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int copyInstr(u64 *pagetable, char *dst, u64 srcva, u64 max)
{
    u64 n, va0, pa0;
    int got_null = 0, cow;

    while (got_null == 0 && max > 0)
    {
        va0 = ALIGN_DOWN(srcva, PAGE_SIZE);
        pa0 = va2PA(pagetable, va0, &cow);
        if (pa0 == 0)
        {
            printk("pa0=0!");
            return -1;
        }
        n = PAGE_SIZE - (srcva - va0);
        if (n > max)
            n = max;

        char *p = (char *)(pa0 + (srcva - va0));
        while (n > 0)
        {
            if (*p == '\0')
            {
                *dst = '\0';
                got_null = 1;
                break;
            }
            else
            {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
        }

        srcva = va0 + PAGE_SIZE;
    }
    if (got_null)
    {
        return 0;
    }
    else
    {
        printk("ungot null\n");
        return -1;
    }
}
