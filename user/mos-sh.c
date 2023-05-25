#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "stddef.h"

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2)
{
    *p1 = 0;
    *p2 = 0;
    if (s == 0)
    {
        return 0;
    }

    while (strchr(WHITESPACE, *s))
    {
        *s++ = 0;
    }
    if (*s == 0)
    {
        return 0;
    }

    if (strchr(SYMBOLS, *s))
    {
        int t = *s;
        *p1 = s;
        *s++ = 0;
        *p2 = s;
        return t;
    }

    *p1 = s;
    while (*s && !strchr(WHITESPACE SYMBOLS, *s))
    {
        s++;
    }
    *p2 = s;
    return 'w';
}

int gettoken(char *s, char **p1)
{
    static int c, nc;
    static char *np1, *np2;

    if (s)
    {
        nc = _gettoken(s, &np1, &np2);
        return 0;
    }
    c = nc;
    *p1 = np1;
    nc = _gettoken(np2, &np1, &np2);
    return c;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe)
{
    int argc = 0;
    while (1)
    {
        char *t;
        int fd, r;
        int c = gettoken(0, &t);
        switch (c)
        {
        case 0:
            return argc;
        case 'w':
            if (argc >= MAXARGS)
            {
                printf("too many arguments\n");
                exit(0);
            }
            argv[argc++] = t;
            break;
        case '<':
            if (gettoken(0, &t) != 'w')
            {
                printf("syntax error: < not followed by word\n");
                exit(0);
            }
            // Open 't' for reading, dup2 it onto fd 0, and then close the original fd.
            /* Exercise 6.5: Your code here. (1/3) */
            fd = open(t, O_RDONLY);
            dup2(fd, 0);
            close(fd);
            break;
        case '>':
            if (gettoken(0, &t) != 'w')
            {
                printf("syntax error: > not followed by word\n");
                exit(0);
            }
            // Open 't' for writing, dup2 it onto fd 1, and then close the original fd.
            /* Exercise 6.5: Your code here. (2/3) */
            fd = open(t, O_WRONLY);
            dup2(fd, 1);
            close(fd);
            break;
        case '|':;
            /*
             * First, allocate a pipe.
             * Then fork, set '*rightpipe' to the returned child envid or zero.
             * The child runs the right side of the pipe:
             * - dup2 the read end of the pipe onto 0
             * - close the read end of the pipe
             * - close the write end of the pipe
             * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
             *   command line.
             * The parent runs the left side of the pipe:
             * - dup2 the write end of the pipe onto 1
             * - close the write end of the pipe
             * - close the read end of the pipe
             * - and 'return argc', to execute the left of the pipeline.
             */
            int p[2];
            /* Exercise 6.5: Your code here. (3/3) */
            pipe(p);
            if (fork())
            {
                dup2(p[1], 1);
                close(p[0]);
                close(p[1]);
                return argc;
            }
            else
            {
                dup2(p[0], 0);
                close(p[0]);
                close(p[1]);
                return parsecmd(argv, rightpipe);
            }
            break;
        }
    }

    return argc;
}

void runcmd(char *s)
{
    gettoken(s, 0);

    char *argv[MAXARGS];
    int rightpipe = 0;
    int argc = parsecmd(argv, &rightpipe);
    if (argc == 0)
    {
        return;
    }
    argv[argc] = 0;

    int child = spawn(argv[0], argv);
    close_all();
    if (child >= 0)
    {
        wait(child);
    }
    else
    {
        printf("spawn %s: %d\n", argv[0], child);
    }
    if (rightpipe)
    {
        wait(rightpipe);
    }
    exit(0);
}

void readline(char *buf, u_int n)
{
    int r;
    for (int i = 0; i < n; i++)
    {
        if ((r = read(0, buf + i, 1)) != 1)
        {
            if (r < 0)
            {
                printf("read error: %d\n", r);
            }
            exit(0);
        }
        if (buf[i] == '\b' || buf[i] == 0x7f)
        {
            if (i > 0)
            {
                i -= 2;
            }
            else
            {
                i = -1;
            }
            if (buf[i] != '\b')
            {
                printf("\b");
            }
        }
        if (buf[i] == '\r' || buf[i] == '\n')
        {
            buf[i] = 0;
            return;
        }
    }
    printf("line too long\n");
    while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n')
    {
        ;
    }
    buf[0] = 0;
}

char buf[1024];

void usage(void)
{
    printf("usage: sh [-dix] [command-file]\n");
    exit(0);
}

int main(int argc, char **argv)
{
    int r;
    int interactive = iscons(0);
    int echocmds = 0;
    printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                         ::\n");
    printf("::                     MOS Shell 2023                      ::\n");
    printf("::                                                         ::\n");
    printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    ARGBEGIN
    {
    case 'i':
        interactive = 1;
        break;
    case 'x':
        echocmds = 1;
        break;
    default:
        usage();
    }
    ARGEND

    if (argc > 1)
    {
        usage();
    }
    if (argc == 1)
    {
        close(0);
        if ((r = open(argv[1], O_RDONLY)) < 0)
        {
            user_panic("open %s: %d", argv[1], r);
        }
        user_assert(r == 0);
    }
    for (;;)
    {
        if (interactive)
        {
            printf("\n$ ");
        }
        readline(buf, sizeof buf);

        if (buf[0] == '#')
        {
            continue;
        }
        if (echocmds)
        {
            printf("# %s\n", buf);
        }
        if ((r = fork()) < 0)
        {
            user_panic("fork: %d", r);
        }
        if (r == 0)
        {
            runcmd(buf);
            exit(0);
        }
        else
        {
            wait(r);
        }
    }
    return 0;
}
