#include <types.h>
#include <driver.h>
#include <dirmeta.h>
#include <file.h>
#include <string.h>

char testContentToWrite[10] = "abcdefghi";
char testContentToRead[10] = {0};

char autoTestContent[] = "time-test\n  \
";

char unixContent[] = "\
./dhry2reg 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench DHRY2 test(lps): \"$0}' \n\
./whetstone-double 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+.[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+.[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench WHETSTONE test(MFLOPS): \"$0}' \n\
./syscall 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SYSCALL test(lps): \"$0}' \n\
./context1 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox tail -n1 | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench CONTEXT test(lps): \"$0}' \n\
./pipe 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench PIPE test(lps): \"$0}' \n\
./spawn 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SPAWN test(lps): \"$0}' \n\
UB_BINDIR=./ ./execl 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench EXECL test(lps): \"$0}' \n\
./fstime -w -t 20 -b 256 -m 500 | ./busybox grep -o \"WRITE COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_WRITE_SMALL test(KBps): \"$0}'\n\
./fstime -r -t 20 -b 256 -m 500 | ./busybox grep -o \"READ COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_READ_SMALL test(KBps): \"$0}'\n\
./fstime -c -t 20 -b 256 -m 500 | ./busybox grep -o \"COPY COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_COPY_SMALL test(KBps): \"$0}'\n\
./fstime -w -t 20 -b 1024 -m 2000 | ./busybox grep -o \"WRITE COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_WRITE_MIDDLE test(KBps): \"$0}'\n\
./fstime -r -t 20 -b 1024 -m 2000 | ./busybox grep -o \"READ COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_READ_MIDDLE test(KBps): \"$0}'\n\
./fstime -c -t 20 -b 1024 -m 2000 | ./busybox grep -o \"COPY COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_COPY_MIDDLE test(KBps): \"$0}'\n\
./fstime -w -t 20 -b 4096 -m 8000 | ./busybox grep -o \"WRITE COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_WRITE_BIG test(KBps): \"$0}'\n\
./fstime -r -t 20 -b 4096 -m 8000 | ./busybox grep -o \"READ COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_READ_BIG test(KBps): \"$0}'\n\
./fstime -c -t 20 -b 4096 -m 8000 | ./busybox grep -o \"COPY COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_COPY_BIG test(KBps): \"$0}'\n\
./looper 20 ./multi.sh 1 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SHELL1 test(lpm): \"$0}' \n\
./looper 20 ./multi.sh 8 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SHELL8 test(lpm): \"$0}' \n\
./looper 20 ./multi.sh 16 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SHELL16 test(lpm): \"$0}' \n\
./arithoh 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench ARITHOH test(lps): \"$0}\'\n\
./short 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench SHORT test(lps): \"$0}\'\n\
./int 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench INT test(lps): \"$0}\'\n\
./long 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench LONG test(lps): \"$0}\'\n\
./float 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench FLOAT test(lps): \"$0}\'\n\
./double 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench DOUBLE test(lps): \"$0}\'\n\
./hanoi 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench HANOI test(lps): \"$0}\'\n\
./syscall 10 exec | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench EXEC test(lps): \"$0}\'\n";

char unixContentPass[] = "\
./dhry2reg 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench DHRY2 test(lps): \"$0}' \n\
./whetstone-double 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+.[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+.[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench WHETSTONE test(MFLOPS): \"$0}' \n\
./syscall 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SYSCALL test(lps): \"$0}' \n\
./context1 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox tail -n1 | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench CONTEXT test(lps): \"$0}' \n\
./pipe 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench PIPE test(lps): \"$0}' \n\
./spawn 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench SPAWN test(lps): \"$0}' \n\
UB_BINDIR=./ ./execl 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench EXECL test(lps): \"$0}' \n\
./arithoh 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench ARITHOH test(lps): \"$0}\'\n\
./short 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench SHORT test(lps): \"$0}\'\n\
./int 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench INT test(lps): \"$0}\'\n\
./long 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench LONG test(lps): \"$0}\'\n\
./float 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench FLOAT test(lps): \"$0}\'\n\
./double 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench DOUBLE test(lps): \"$0}\'\n\
./hanoi 10 | ./busybox grep -o \"COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk \'{print \"Unixbench HANOI test(lps): \"$0}\'\n\
./fstime -w -t 20 -b 256 -m 500 | ./busybox grep -o \"WRITE COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_WRITE_SMALL test(KBps): \"$0}'\n\
./fstime -r -t 20 -b 256 -m 500 | ./busybox grep -o \"READ COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_READ_SMALL test(KBps): \"$0}'\n\
./fstime -c -t 20 -b 256 -m 500 | ./busybox grep -o \"COPY COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_COPY_SMALL test(KBps): \"$0}'\n\
./fstime -w -t 20 -b 1024 -m 2000 | ./busybox grep -o \"WRITE COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_WRITE_MIDDLE test(KBps): \"$0}'\n\
./fstime -r -t 20 -b 1024 -m 2000 | ./busybox grep -o \"READ COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_READ_MIDDLE test(KBps): \"$0}'\n\
./fstime -c -t 20 -b 1024 -m 2000 | ./busybox grep -o \"COPY COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_COPY_MIDDLE test(KBps): \"$0}'\n\
./fstime -w -t 20 -b 4096 -m 8000 | ./busybox grep -o \"WRITE COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_WRITE_BIG test(KBps): \"$0}'\n\
./fstime -r -t 20 -b 4096 -m 8000 | ./busybox grep -o \"READ COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_READ_BIG test(KBps): \"$0}'\n\
./fstime -c -t 20 -b 4096 -m 8000 | ./busybox grep -o \"COPY COUNT|[[:digit:]]\\+|\" | ./busybox grep -o \"[[:digit:]]\\+\" | ./busybox awk '{print \"Unixbench FS_COPY_BIG test(KBps): \"$0}'\n\
busybox sh busybox_testcode.sh\n \
busybox sh lua_testcode.sh\n \
busybox sh libctest_testcode.sh\n\
libc-bench \n\
busybox sh cyclictest_testcode.sh \n\
busybox sh iozone_testcode.sh \n\
";

char libc[] = "./libc-bench \n";

void buildScript()
{
    printk("building auto script.\n");
    DirMeta *script = metaCreate(AT_FDCWD, "/auto.sh", T_FILE, O_CREATE | O_RDWR);
    // metaWrite(script, false, (u64)autoTestContent, 0, sizeof(autoTestContent));
    // metaWrite(script, false, (u64)unixContent, sizeof(autoTestContent), sizeof(unixContent));

    metaWrite(script, false, (u64)autoTestContent, 0, sizeof(autoTestContent));
    metaWrite(script, false, (u64)unixContentPass, sizeof(autoTestContent), sizeof(unixContentPass));

    // metaWrite(script, false, (u64)unixContentPass, 0, sizeof(unixContentPass));

    // metaWrite(script, false, (u64)libc, 0, sizeof(libc));
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