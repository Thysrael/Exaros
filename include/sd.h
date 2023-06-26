/**
 * @file sd.h SD 卡相关
 * @brief
 * @date 2023-06-24
 *
 * @copyright Copyright (c) 2023
 */

#include "types.h"

#define MAX_TIMES 50000

void sdInit();
int sdRead(u8 *buf, u64 startBlock, u64 blockNumber);
int sdWrite(u8 *buf, u64 startSector, u64 sectorNumber);
int sdTest();