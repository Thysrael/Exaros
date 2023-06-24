#include <types.h>

void *memset(void *dst, int c, u32 n)
{
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++)
    {
        cdst[i] = c;
    }
    return dst;
}

int memcmp(const void *v1, const void *v2, u32 n)
{
    const uchar *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0)
    {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}

/**
 * @brief 从内核源地址拷贝一段内存到内核目的地址
 *
 * @param dst 目的地址
 * @param src 源地址
 * @param n 拷贝长度
 * @return void* 目的地址
 */
void *memmove(void *dst, const void *src, u32 n)
{
    const char *s;
    char *d;
    s = src;
    d = dst;

    while (n-- > 0)
        *d++ = *s++;

    return dst;
}

/**
 * @brief 从内核源地址拷贝一段内存到内核目的地址，和 memmove 一样
 *
 * @param dst 目的地址
 * @param src 源地址
 * @param n 拷贝长度
 * @return void* 目的地址
 */
void *memcpy(void *dst, const void *src, u32 n)
{
    return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, u32 n)
{
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uchar)*p - (uchar)*q;
}

char *strncpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char *safestrcpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}

int strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

char *strchr(const char *s, char c)
{
    for (; *s; s++)
        if (*s == c)
            return (char *)s;
    return 0;
}

// convert wide char string into uchar string
void snstr(char *dst, wchar const *src, int len)
{
    while (len-- && *src)
    {
        *dst++ = (uchar)(*src & 0xff);
        src++;
    }
    while (len-- > 0)
        *dst++ = 0;
}