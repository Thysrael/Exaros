#include <types.h>
#include <driver.h>
#include <dirmeta.h>
#include <file.h>
#include <string.h>

char testContentToWrite[10] = "abcdefghi";
char testContentToRead[10] = {0};

void testMeta()
{
    printk("[testfat] testing fat.......\n");
    DirMeta *testfile = metaCreate(AT_FDCWD, "/testfile", T_FILE, O_CREATE_GLIBC | O_RDWR);
    if (testfile == NULL)
    {
        panic("[testfat] create file error\n");
    }
    printk("create file finish\n");

    int ret = metaWrite(testfile, 0, (u64)testContentToWrite, 0, 9);
    if (ret != 9)
    {
        panic("[testfat] write file error\n");
    }

    printk("write file finish\n");
    testfile = metaName(AT_FDCWD, "/testfile", true);
    if (testfile == NULL)
    {
        panic("[testfat] open file error\n");
    }
    metaRead(testfile, 0, (u64)testContentToRead, 0, 9);
    if (strncmp(testContentToWrite, testContentToRead, 114514) == 0)
    {
        printk("[testfat]  testfat passed\n");
    }
    else
    {
        panic("[testfat]  testfat failed\n");
    }
}