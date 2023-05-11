/**
 * @file pipe.h
 * @brief
 * @date 2023-05-10
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "types.h"

typedef struct File File;

#define PIPESIZE 65536
#define MAX_PIPE 128

typedef struct Pipe
{
    char data[PIPESIZE];
    u64 nread;     // number of bytes read
    u64 nwrite;    // number of bytes written
    int readopen;  // read fd is still open
    int writeopen; // write fd is still open
} Pipe;

int pipeNew(File **f0, File **f1);
void pipeClose(Pipe *pi, int writable);
int pipeWrite(Pipe *pi, bool isUser, u64 addr, int n);
int pipeRead(Pipe *pi, bool isUser, u64 addr, int n);

#endif