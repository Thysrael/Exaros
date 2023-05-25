#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

void test_brk()
{
    TEST_START(__func__);
    intptr_t cur_pos, alloc_pos, alloc_pos_1;

    cur_pos = brk(0);
    printf("Before alloc,heap pos: %lx\n", cur_pos);
    brk(cur_pos + 64);
    alloc_pos = brk(0);
    printf("After alloc,heap pos: %lx\n", alloc_pos);
    brk(alloc_pos + 64);
    alloc_pos_1 = brk(0);
    printf("Alloc again,heap pos: %lx\n", alloc_pos_1);
    TEST_END(__func__);
}
int main(void)
{
    test_brk();
    return 0;
}
