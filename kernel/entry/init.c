/**
 * @file init.c
 * @brief 对于主核和副核分别进行启动，主核启动包括清空 BSS
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 */

#include <types.h>
#include <driver.h>
#include <debug.h>
#include <riscv.h>
#include <memory.h>
#include <string.h>
#include <process.h>
#include <trap.h>
#include <fs.h>
#include <fat.h>
#include <file.h>
#include <bio.h>
#include <virtio.h>

/**
 * @brief boot banner, `train` style character drawing.
 *
 */
// char *banner =
//     "\n\t\033[34mU\033[0m _____ \033[34mU\033[0m   __  __       _         ____        \033[34mU\033[0m  ___ \033[34mU\033[0m   ____     \n"
//     "\t\033[34m\\\033[0m| ___\033[32m\"\033[0m|\033[34m/\033[0m   \\ \\/\033[32m\"\033[0m/   \033[34mU\033[0m  /\033[32m\"\033[0m\\  \033[34mU\033[0m  \033[34mU\033[0m |  _\033[32m\"\033[0m\\ \033[34mU\033[0m      \033[34m\\\033[0m/\033[32m\"\033[0m_ \\\033[34m/\033[0m  / __\033[32m\"\033[0m| \033[34mU\033[0m  \n"
//     "\t |  _|\033[32m\"\033[0m     \033[34m/\033[0m\\  /\033[34m\\\033[0m    \033[34m\\\033[0m/ _ \\\033[34m/\033[0m    \033[34m\\\033[0m| |_) |\033[34m/\033[0m      | | | | \033[34m<\033[0m\\___ \\\033[34m/\033[0m   \n"
//     "\t | |___    \033[34mU\033[0m /  \\ \033[34mU\033[0m   / ___ \\     |  _ <    \033[31m.-,\033[33m_\033[0m| |_| |  \033[34mU\033[0m___) |   \n"
//     "\t |_____|    /_/\\_\\   /_/   \\_\\    |_| \\_\\    \033[31m\\_)\033[33m-\033[0m\\___/   |____/\033[33m>>\033[0m  \n"
//     "\t \033[33m<<   >>  \033[31m,-,\033[33m>> \\\\\033[31m_\033[33m   \\\\    >>    //   \\\\\033[31m_\033[33m        \\\\      \033[31m)(  (__) \n"
//     "\t(__) (__)  \\_)  (__) (__)  (__)  (__)  (__)      (__)    (__)\033[0m      \n\n";

/**
 * @brief 清空 BSS 段
 *
 */
static inline void clearBSS()
{
    // from kernel.lds
    extern u64 bssStart[];
    extern u64 bssEnd[];
    for (u64 *i = bssStart; i < bssEnd; i++)
    {
        *i = 0;
    }
}

/**
 * @brief 对于主副核采用不同的启动策略
 *
 * @param hartId 核 ID
 */
void main(u64 hartId)
{
    setTp(hartId);
    clearBSS();
    printk("Hello, Exaros!\n");

    memoryInit();

    processInit();

    // fs initialize

    binit();
    virtioDiskInit();
    fileinit();

    trapInit();
    plicinit();
    plicinithart();

    // PROCESS_CREATE_PRIORITY(processA, 1);
    // PROCESS_CREATE_PRIORITY(processB, 1);
    // PROCESS_CREATE_PRIORITY(processC, 1);
    PROCESS_CREATE_PRIORITY(test, 1);

    yield();
    while (1)
        ;
}