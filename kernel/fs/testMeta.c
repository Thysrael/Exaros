#include <types.h>
#include <driver.h>
#include <dirmeta.h>
#include <file.h>
#include <string.h>

char testContentToWrite[10] = "abcdefghi";
char testContentToRead[10] = {0};

char autoTestContent[] = "\\time-test\n  \
./dhry2reg 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench DHRY2 test(lps): \"$0}'\n \
\\busybox sh \\busybox_testcode.sh\n \
\\busybox sh \\iozone_testcode.sh\n \
\\busybox sh \\lua_testcode.sh\n \
\\libc-bench\n \
\\busybox sh \\libctest_testcode.sh\n \
\\busybox sh \\unixbench_testcode.sh\n ";

void buildScript()
{
    printk("building auto script.\n");
    DirMeta *script = metaCreate(AT_FDCWD, "/auto.sh", T_FILE, O_CREATE | O_RDWR);
    metaWrite(script, 0, (u64)autoTestContent, 0, sizeof(autoTestContent) + 1);
}

void testMeta()
{
    printk("[testMeta] testing fat.......\n");
    DirMeta *testfile = metaCreate(AT_FDCWD, "/testfile", T_FILE, O_CREATE | O_RDWR);
    if (testfile == NULL)
    {
        panic("[testMeta] create file error\n");
    }
    printk("[testMeta] create file finish\n");

    int ret = metaWrite(testfile, 0, (u64)testContentToWrite, 0, 9);
    if (ret != 9)
    {
        panic("[testMeta] write file error\n");
    }

    printk("[testMeta] write file finish\n");
    testfile = metaName(AT_FDCWD, "/testfile", true);
    if (testfile == NULL)
    {
        panic("[testMeta] open file error\n");
    }
    metaRead(testfile, 0, (u64)testContentToRead, 0, 9);
    if (strncmp(testContentToWrite, testContentToRead, 114514) == 0)
    {
        printk("[testMeta]  testMeta passed\n");
    }
    else
    {
        panic("[testMeta]  testMeta failed\n");
    }
}