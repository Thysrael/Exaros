#include <driver.h>
#include <riscv.h>

#define BUF_SIZE 256
#define IsDigit(x) (((x) >= '0') && ((x) <= '9'))

static void printChar(char *buf, char c, int length, int ladjust);
static void printString(char *buf, char *s, int length, int ladjust);
static void printNum(char *buf, unsigned long u, int base, int negFlag,
                     int length, int ladjust, char padc, int upcase);
static inline void putBuffer(const char *s);
static void print(const char *fmt, va_list ap);
static void panicPrintf(const char *fmt, ...);

/**
 * @brief kernel 的格式化输出函数
 *
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void printk(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
}

/**
 * @brief PANIC 的内部函数，可以打印 HARTID，行数等信息
 *
 * @param file 文件名
 * @param line 文件行数
 * @param func 函数名
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void _panic_(const char *file, int line, const char *func, const char *fmt, ...)
{
    panicPrintf("hartId %d panic at %s: %d in %s(): ", getTp(), file, line, func);
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
    putchar('\n');
    while (true)
        ;
}

/**
 * @brief ASSERT 的内部函数，可以打印多种信息
 *
 * @param file 文件名
 * @param line 所在行数
 * @param func 所在函数
 * @param statement 断言条件
 */
void _assert_(const char *s, const char *file, int line, const char *func, u64 statement)
{
    if (!statement)
    {
        _panic_(file, line, func, "%s assert failed\n", s);
    }
}

static inline void putBuffer(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
        {
            putchar('\r');
        }
        putchar(*s++);
    }
}

static void printChar(char *buf, char c, int length, int ladjust)
{
    int i;

    if (length < 1)
        length = 1;
    if (ladjust)
    {
        *buf = c;
        for (i = 1; i < length; i++)
            buf[i] = ' ';
    }
    else
    {
        for (i = 0; i < length - 1; i++)
            buf[i] = ' ';
        buf[length - 1] = c;
    }
    buf[length] = '\0';
}

static void printString(char *buf, char *s, int length, int ladjust)
{
    int i;
    int len = 0;
    char *s1 = s;
    while (*s1++)
        len++;
    if (length < len)
        length = len;

    if (ladjust)
    {
        for (i = 0; i < len; i++)
            buf[i] = s[i];
        for (i = len; i < length; i++)
            buf[i] = ' ';
    }
    else
    {
        for (i = 0; i < length - len; i++)
            buf[i] = ' ';
        for (i = length - len; i < length; i++)
            buf[i] = s[i - length + len];
    }
    buf[length] = '\0';
}

static void printNum(char *buf, unsigned long u, int base, int negFlag,
                     int length, int ladjust, char padc, int upcase)
{
    int actualLength = 0;
    char *p = buf;
    int i;

    do
    {
        int tmp = u % base;
        if (tmp <= 9)
        {
            *p++ = '0' + tmp;
        }
        else if (upcase)
        {
            *p++ = 'A' + tmp - 10;
        }
        else
        {
            *p++ = 'a' + tmp - 10;
        }
        u /= base;
    } while (u != 0);

    if (negFlag)
    {
        *p++ = '-';
    }

    /* figure out actual length and adjust the maximum length */
    actualLength = p - buf;
    if (length < actualLength)
        length = actualLength;

    /* add padding */
    if (ladjust)
    {
        padc = ' ';
    }
    if (negFlag && !ladjust && (padc == '0'))
    {
        for (i = actualLength - 1; i < length - 1; i++)
            buf[i] = padc;
        buf[length - 1] = '-';
    }
    else
    {
        for (i = actualLength; i < length; i++)
            buf[i] = padc;
    }

    /* prepare to reverse the string */
    {
        int begin = 0;
        int end;
        if (ladjust)
        {
            end = actualLength - 1;
        }
        else
        {
            end = length - 1;
        }

        while (end > begin)
        {
            char tmp = buf[begin];
            buf[begin] = buf[end];
            buf[end] = tmp;
            begin++;
            end--;
        }
    }

    /* adjust the string pointer */
    buf[length] = '\0';
}

static void print(const char *fmt, va_list ap)
{
    char buf[BUF_SIZE];

    int i, c;
    char *s;
    long int num;

    int longFlag, negFlag, width, prec, ladjust;
    char padc;

    for (i = 0; fmt[i]; i++)
    {
        // put the char to the console
        if (fmt[i] != '%')
        {
            if (fmt[i] == '\n')
            {
                putchar('\r');
            }
            putchar(fmt[i]);
            continue;
        }

        // transfer the formatter
        c = fmt[++i];
        if (c == '-')
        {
            c = fmt[++i];
            ladjust = 1;
        }
        else
        {
            ladjust = 0;
        }

        if (c == '0')
        {
            c = fmt[++i];
            padc = '0';
        }
        else
        {
            padc = ' ';
        }

        width = 0;
        while (IsDigit(c))
        {
            width = (width << 3) + (width << 1) + c - '0';
            c = fmt[++i];
        }

        if (c == '.')
        {
            c = fmt[++i];
            prec = 0;
            while (IsDigit(c))
            {
                prec = prec * 10 + c - '0';
                c = fmt[++i];
            }
        }
        else
        {
            prec = 8;
        }

        if (c == 'l')
        {
            longFlag = 1;
            c = fmt[++i];
        }
        else
        {
            longFlag = 0;
        }

        negFlag = 0;
        switch (c)
        {
        case 'c':
            c = (char)va_arg(ap, u32);
            printChar(buf, c, width, ladjust);
            putchar(c);
            break;

        case 'D':
        case 'd':
            if (longFlag)
            {
                num = va_arg(ap, i64);
            }
            else
            {
                num = va_arg(ap, i32);
            }

            if (num < 0)
            {
                num = -num;
                negFlag = 1;
            }

            printNum(buf, num, 10, negFlag, width, ladjust, padc, 0);
            putBuffer(buf);
            break;

        case 'O':
        case 'o':
            if (longFlag)
            {
                num = va_arg(ap, i64);
            }
            else
            {
                num = va_arg(ap, i32);
            }

            printNum(buf, num, 8, 0, width, ladjust, padc, 0);
            putBuffer(buf);
            break;

        case 'U':
        case 'u':
            if (longFlag)
            {
                num = va_arg(ap, i64);
            }
            else
            {
                num = va_arg(ap, i32);
            }

            printNum(buf, num, 10, 0, width, ladjust, padc, 0);
            putBuffer(buf);
            break;

        case 'x':
            if (longFlag)
            {
                num = va_arg(ap, i64);
            }
            else
            {
                num = va_arg(ap, i32);
            }

            printNum(buf, num, 16, 0, width, ladjust, padc, 0);
            putBuffer(buf);
            break;

        case 'X':
            if (longFlag)
            {
                num = va_arg(ap, i64);
            }
            else
            {
                num = va_arg(ap, i32);
            }

            printNum(buf, num, 16, 0, width, ladjust, padc, 1);
            putBuffer(buf);
            break;

        case 's':
            if ((s = va_arg(ap, char *)) == 0)
            {
                s = "(null)";
            }

            printString(buf, s, width, ladjust);
            putBuffer(buf);
            break;

        case '\0':
            i--;
            break;

        default:
            putchar(c);
            break;
        }
    }
}

static void panicPrintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
}