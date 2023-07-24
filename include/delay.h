/**
 * @file delay.h
 * @brief 用于产生内核延迟，一般用于使配置生效
 * @date 2023-07-24
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _DELAY_H_
#define _DELAY_H_
#include "types.h"

void usdelay(u64 interval);

#endif