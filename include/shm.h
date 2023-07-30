/**
 * @file shm.h
 * @brief share memory 相关
 * @date 2023-07-30
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _SHM_H_
#define _SHM_H_
#include <types.h>

typedef struct ShareMemory
{
    bool used; // ShareMemory 是否被使用
    u64 start; // ShareMemory 起始地址
    u64 size;  // ShareMemory 大小
} ShareMemory;

#define SHM_COUNT 128

int shmAlloc(int key, u64 size, int shmflg);
u64 shmAt(int shmid, u64 shmaddr, int shmflg);

#endif