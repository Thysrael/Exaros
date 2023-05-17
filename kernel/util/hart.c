#include <memory.h>
#include <hart.h>
#include <mem_layout.h>

struct Hart harts[CORE_NUM];

struct Hart *myHart()
{
    int r = getTp();
    return &(harts[r]);
}