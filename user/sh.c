/**
 * @file sh.c
 * @brief 用户终端
 * Copy from tsh(qs)
 * @date 2023-05-25
 *
 * @copyright Copyright (c) 2023
 */

#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "file.h"

#define MAX_TOKENS 128
#define MAX_ARGS 64
#define MAX_COMMANDS 16
#define BUFF_SIZE 256
#define PERM

// 注意 O_CREATE_GPP

// 'Type' is the command word type
typedef enum
{
    TYPE_NORMAL,
    TYPE_IN_REDIRECT,
    TYPE_OUT_REDIRECT,
    TYPE_OUT_APPEND,
    TYPE_PIPE,
    TYPE_END
} Type;

// 'STATE' is the FSM state
typedef enum
{
    STATE_NORMAL,
    STATE_END,
    STATE_INTERVAL,
    STATE_PIPE,
    STATE_OUT_REDIRECT,
    STATE_OUT_APPEND,
    STATE_IN_REDIRECT,
    STATE_SINGLE_STR,
    STATE_DOUBLE_STR
} State;

typedef struct
{
    int argc;
    char *argv[MAX_ARGS];
    char *file_in;
    char *file_out;
    char *file_append;
} Command;

typedef struct
{
    char *content;
    Type type;
} Token;

Token tokens[MAX_TOKENS];
int token_cur;

Command commands[MAX_COMMANDS];
int command_cur;

char line_buf[BUFF_SIZE];
char read_buf[BUFF_SIZE];

// print the error informatino and quit
void unix_error(char *msg);
// print the head of shell
void print_head();
// print the prompt
void print_prompt();
// init the tokens and the commands
void init();
// excute commands
void eval();
// parse the cmd line to the commands
void parse_line();
// transfer the cmd line to the tokens
void tokenize();
// transfer the tokens to the commands
void parse_commands();
// excute the command
void execute_command(Command command, int fd_in, int fd_out);
// print the information of quit and quit
void quit();
// excute the builtin command
int builtin_command(Command command);
// wrapper the fork()
pid_t Fork();
// wrapper the waitpid
void Wait(pid_t pid);

#define CTRL_C 0x03
#define CTRL_D 0x04
#define BACKSPACE 0x7f
#define DELETE 0x08
#define ENTER 0x0D
#define ANSI 0x1b
#define TAB 0x09

void readline(char *buf, size_t n)
{
    int pos = 0;
    int r;
    while (1)
    {
        r = read(stdin, read_buf, BUFF_SIZE);
        switch (r)
        {
        case 0:
            printf("Error\n");
            break;
        case 1:
            if (read_buf[0] == BACKSPACE)
            {
                if (pos)
                {
                    pos--;
                    printf("\033[1D");
                    printf("\033[1P");
                }
            }
            else if (read_buf[0] == CTRL_C)
            {
                pos = 0;
                buf[pos++] = CTRL_C;
                buf[pos] = 0;
                return;
            }
            else if (read_buf[0] == CTRL_D)
            {
                if (!pos)
                {
                    pos = 0;
                    buf[pos++] = CTRL_D;
                    buf[pos] = 0;
                    return;
                }
            }
            else if (read_buf[0] == ENTER)
            {
                buf[pos] = 0;
                return;
            }
            else if (read_buf[0] == TAB)
            {
            }
            else
            {
                printf("%c", read_buf[0]);
                buf[pos++] = read_buf[0];
            }
            break;
        case 2:
        case 3:
        case 4:
        default: break;
        }
    }
}

int main()
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    print_head();
    while (1)
    {
        print_prompt();
        readline(line_buf, BUFF_SIZE);
        printf("\n");
        if (line_buf[0] == CTRL_C)
        {
            continue;
        }
        if (line_buf[0] == CTRL_D)
        {
            quit();
        }
        eval();
    }
    while (1)
        ;
    return 0;
}

void unix_error(char *msg)
{
    printf("\033[31mError in command: %s\033[0m\n", msg);
    exit(0);
}

void quit()
{
    printf("\033[32mThyShell closes ...\033[0m\n");
    exit(0);
}

pid_t Fork()
{
    pid_t pid;
    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}

// void Wait(pid_t pid)
// {
//     int status;
//     waitpid(pid, &status, 0);
//     if (!WIFEXITED(status))
//         printf("child %d terminated abnormally\n", pid);
// }

void print_head()
{
    printf("\033[32m");
    printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                         ::\n");
    printf("::                      Thysrael Shell                     ::\n");
    printf("::                                                         ::\n");
    printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("\n                 Can you hear me, my sweetie?\n");
    printf("\033[0m\n");
}

void print_prompt()
{
    static char cwd_buf[32];
    char *path = getcwd(cwd_buf, sizeof(cwd_buf));
    // const char *home = getenv("HOME");
    // abbreviate home path
    // if (strstr(home, path) == 0)
    // {
    //     path[0] = '~';
    //     size_t len_home = strlen(home);
    //     size_t len_path = strlen(path);
    //     memmove(path + 1, path + len_home, len_path - len_home);
    //     path[len_path - len_home + 1] = '\0';
    // }
    // use "\033[" to change color
    // printf("ThyShell \033[0;32m%s\033[0m $ ", path);
    printf("\033[0;32m%s\033[0m > ", path);
    // free(path);
}

void init()
{
    memset(tokens, 0, sizeof(tokens));
    token_cur = 0;
    memset(commands, 0, sizeof(commands));
    command_cur = 0;
}

void eval()
{
    parse_line();

    int fd[2], prev_out_fd = -1;

    for (int i = 0; i < command_cur; i++)
    {
        pipe(fd);
        execute_command(commands[i], prev_out_fd, i == command_cur - 1 ? -1 : fd[1]);
        close(fd[1]);
        if (prev_out_fd > 0)
            close(prev_out_fd);

        prev_out_fd = fd[0];
    }

    if (prev_out_fd > 0)
        close(prev_out_fd);
}

void execute_command(Command command, int fd_in, int fd_out)
{
    if (!builtin_command(command))
    {
        pid_t pid = Fork();
        if (pid == 0)
        {
            if (command.file_in)
            {
                int in = open(command.file_in, O_RDONLY);
                dup2(in, stdin);
            }
            else if (fd_in > 0)
            {
                dup2(fd_in, stdin);
            }

            if (command.file_out)
            {
                // int out = open(command.file_out, O_RDWR | O_CREAT | O_TRUNC);
                int out = open(command.file_out, O_RDWR | O_CREATE | O_TRUNC);
                dup2(out, stdout);
            }
            else if (command.file_append)
            {
                // int out = open(command.file_out, O_RDWR | O_CREAT | O_TRUNC);
                int append = open(command.file_append, O_WRONLY | O_CREATE | O_APPEND);
                dup2(append, stdout);
            }
            else if (fd_out > 0)
            {
                dup2(fd_out, stdout);
            }
            char *newenviron[] = {NULL}; // 未实现 environ
            execve(command.argv[0], command.argv, newenviron);
            unix_error(command.argv[0]);
        }
        wait(0);
    }
}

int builtin_command(Command command)
{
    if (!strcmp(command.argv[0], "quit"))
        quit();
    else if (!strcmp(command.argv[0], "cd"))
    {
        if (chdir(command.argv[1]) != 0)
        {
            printf("\033[31m");
            printf("cannot cd %s", command.argv[1]);
            printf("\033[0m\n");
        }
        return 1;
    }
    else if (!strcmp(command.argv[0], "clear"))
    {
        printf("\033[2J");
        return 1;
    }
    return 0;
}

void parse_line()
{
    init();
    tokenize();
    parse_commands();
}

void tokenize()
{
    char *cur = line_buf;
    char *start;
    State state = STATE_NORMAL;
    while (*cur && (*cur == ' ')) cur++;
    start = cur;

    while (1)
    {
        if (state == STATE_NORMAL)
        {
            if (*cur == ' ' || *cur == '>' || *cur == '<' || *cur == '|' || *cur == '\0')
            {
                tokens[token_cur].content = start;
                tokens[token_cur++].type = TYPE_NORMAL;
                if (*cur == ' ')
                    state = STATE_INTERVAL;
                else if (*cur == '>')
                    state = STATE_OUT_REDIRECT;
                else if (*cur == '<')
                    state = STATE_IN_REDIRECT;
                else if (*cur == '|')
                    state = STATE_PIPE;
                else if (*cur == '\0')
                    state = STATE_END;

                *cur = '\0';
            }
        }
        else if (state == STATE_INTERVAL)
        {
            if (*cur == '>')
            {
                start = cur;
                state = STATE_OUT_REDIRECT;
            }
            else if (*cur == '<')
            {
                start = cur;
                state = STATE_IN_REDIRECT;
            }
            else if (*cur == '|')
            {
                start = cur;
                state = STATE_PIPE;
            }
            else if (*cur == ' ')
            {
                state = STATE_INTERVAL;
            }
            else if (*cur == '\'')
            {
                start = cur + 1;
                state = STATE_SINGLE_STR;
            }
            else if (*cur == '\"')
            {
                start = cur + 1;
                state = STATE_DOUBLE_STR;
            }
            else if (*cur == '\0')
            {
                state = STATE_END;
            }
            else
            {
                start = cur;
                state = STATE_NORMAL;
            }
        }
        else if (state == STATE_SINGLE_STR)
        {
            if (*cur == '\'')
            {
                *cur = '\0';
                tokens[token_cur].content = start;
                tokens[token_cur++].type = TYPE_NORMAL;
                state = STATE_INTERVAL;
            }
        }
        else if (state == STATE_DOUBLE_STR)
        {
            if (*cur == '\"')
            {
                *cur = '\0';
                tokens[token_cur].content = start;
                tokens[token_cur++].type = TYPE_NORMAL;
                state = STATE_INTERVAL;
            }
        }
        else if (state == STATE_PIPE)
        {
            tokens[token_cur].content = "|";
            tokens[token_cur++].type = TYPE_PIPE;
            if (*cur == ' ')
            {
                state = STATE_INTERVAL;
            }
            else
            {
                start = cur;
                state = STATE_NORMAL;
            }
        }
        else if (state == STATE_IN_REDIRECT)
        {
            tokens[token_cur].content = "<";
            tokens[token_cur++].type = TYPE_IN_REDIRECT;
            if (*cur == ' ')
            {
                state = STATE_INTERVAL;
            }
            else
            {
                start = cur;
                state = STATE_NORMAL;
            }
        }
        else if (state == STATE_OUT_REDIRECT)
        {
            if (*cur == '>')
            {
                state = STATE_OUT_APPEND;
            }
            else
            {
                tokens[token_cur].content = ">";
                tokens[token_cur++].type = TYPE_OUT_REDIRECT;
                if (*cur == ' ')
                {
                    state = STATE_INTERVAL;
                }
                else
                {
                    start = cur;
                    state = STATE_NORMAL;
                }
            }
        }
        else if (state == STATE_OUT_APPEND)
        {
            tokens[token_cur].content = ">>";
            tokens[token_cur++].type = TYPE_OUT_APPEND;
            if (*cur == ' ')
            {
                state = STATE_INTERVAL;
            }
            else
            {
                start = cur;
                state = STATE_NORMAL;
            }
        }
        else if (state == STATE_END)
        {
            tokens[token_cur].content = NULL;
            tokens[token_cur++].type = TYPE_END;
            break;
        }
        cur++;
    }
}

void parse_commands()
{
    token_cur = 0;
    command_cur = 0;
    while (tokens[token_cur].type != TYPE_END)
    {
        while (tokens[token_cur].type != TYPE_END && tokens[token_cur].type != TYPE_PIPE)
        {
            if (tokens[token_cur].type == TYPE_NORMAL)
                commands[command_cur].argv[commands[command_cur].argc++] = tokens[token_cur].content;
            else if (tokens[token_cur].type == TYPE_IN_REDIRECT)
                commands[command_cur].file_in = tokens[++token_cur].content;
            else if (tokens[token_cur].type == TYPE_OUT_REDIRECT)
                commands[command_cur].file_out = tokens[++token_cur].content;
            else if (tokens[token_cur].type == TYPE_OUT_APPEND)
                commands[command_cur].file_append = tokens[++token_cur].content;
            token_cur++;
        }
        // we need it to execvp
        commands[command_cur].argv[commands[command_cur].argc++] = NULL;
        command_cur++;
        if (tokens[token_cur].type == TYPE_PIPE)
            token_cur++;
    }
}