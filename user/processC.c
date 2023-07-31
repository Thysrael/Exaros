#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

int hello(int a, char **b)
{
    printf("hello, %d\n", a);
    return 0;
}

int hi(int a, char **b)
{
    printf("hi, %d\n", a);
    return 0;
}

struct FuncStru
{
    const char *name;
    int (*func)(int, char **);
} main_table[] = {
    {"hello", hello},
    {"hi", hi},
    {NULL, NULL},
};

int main(int argc, char **argv)
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);
    printf("execute func begin\n");
    struct FuncStru *index = main_table;
    while (index->name != NULL)
    {
        index->func(3, NULL);
        index++;
    }
    return 0;
}