#include <hart.h>
#include <riscv.h>
#include <process.h>
#include <mem_layout.h>

struct Hart harts[CORE_NUM];

inline struct Hart *myHart()
{
    int r = getTp();
    return &harts[r];
}