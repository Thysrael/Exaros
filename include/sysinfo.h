#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include "mem_layout.h"
#include "string.h"

struct sysinfo
{
    long uptime;             /* Seconds since boot */
    unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    unsigned long totalram;  /* Total usable main memory size */
    unsigned long freeram;   /* Available memory size */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap;  /* Swap space still available */
    unsigned short procs;    /* Number of current processes */
    char _f[22];             /* Pads structure to 64 bytes */
};

int sysinfo(struct sysinfo *info)
{
    asm volatile("rdtime %0"
                 : "=r"(info->uptime));
    info->loads[0] = 0;
    info->loads[1] = 0;
    info->loads[2] = 0;
    info->totalram = PHYSICAL_MEMORY_SIZE;
    info->freeram = PHYSICAL_MEMORY_SIZE;
    info->sharedram = 0;
    info->bufferram = 0;
    info->totalswap = 0;
    info->freeswap = 0;
    info->procs = 1;
    return 0;
}

#endif