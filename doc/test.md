# Test

为了提高操作系统内核的软件质量，除了区域赛官方提供的样例外，我们还在软件开发过程中进行了模块测试。

## 模块测试

### 驱动测试

对于同一个块进行先写后读，检验内容是否一致。

```c
for (int i = 0; i < 1024; i++) {
    binary[i] = i & 7;
    sdWrite(binary, j, 2);
    for (int i = 0; i < 1024; i++) {
        binary[i] = 0;
    }
    sdRead(binary, j, 2);
    for (int i = 0; i < 1024; i++) {
        if (binary[i] != (i & 7)) {
            panic("gg: %d ", j);
            break;
        }
    }
}
```

### 文件系统测试

分为两个部分，一个是通过读取 FAT 文件系统超级块的参数，来检验合理性，如图所示

```c
// 打印 superblock 信息
QS_DEBUG("[FAT32 init] bytsPerSec: %d\n", fs->superBlock.BPB.bytsPerSec);
QS_DEBUG("[FAT32 init] secPerClus: %d\n", fs->superBlock.BPB.secPerClus);
QS_DEBUG("[FAT32 init] rsvdSecCnt: %d\n", fs->superBlock.BPB.rsvdSecCnt);
QS_DEBUG("[FAT32 init] numFATs: %d\n", fs->superBlock.BPB.numFATs);
QS_DEBUG("[FAT32 init] totSec: %d\n", fs->superBlock.BPB.totSec);
QS_DEBUG("[FAT32 init] FATsz: %d\n", fs->superBlock.BPB.FATsz);
QS_DEBUG("[FAT32 init] rootClus: %d\n", fs->superBlock.BPB.rootClus);
QS_DEBUG("[FAT32 init] firstDataSec: %d\n", fs->superBlock.firstDataSec);
QS_DEBUG("[FAT32 init] dataSecCnt: %d\n", fs->superBlock.dataSecCnt);
QS_DEBUG("[FAT32 init] dataClusCnt: %d\n", fs->superBlock.dataClusCnt);
QS_DEBUG("[FAT32 init] bytsPerClus: %d\n", fs->superBlock.bytsPerClus);
```

另一个是测试文件的读写功能

```c
char testContentToWrite[10] = "abcdefghi";
char testContentToRead[10] = {0};

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
```

### 进程调度测试

这部分是测试时钟中断和系统调用是否正常响应是完成的。

测试代码放在了两个文件  `processA.c` 和 `processB.c`中，代码如下所示

processA.c

```c
#include "unistd.h"
#include "stdio.h"

int main(int argc, char **argv)
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    for (int i = 1; i <= 100; ++i)
    {
        putchar('@');
    }
    putchar('\n');
    while (1)c
        ;
    return 0;
}
```

processB.c

```c
#include "stdio.h"

int main(int argc, char **argv)
{
    for (int i = 1; i <= 100; ++i)
    {
        putchar('*');
    }
    while (1)
        ;
    return 0;
}
```

### 内存测试

在内核中进行中进行测试，测试分页，删除页面等功能是否正常。

```c
void pageTest()
{
    char *s = (char *)(0x84000000 - 0x1000);
    char str1[] = "Pass Memory Test! (1/2)";
    memmove(s, str1, strlen(str1));
    memoryInit();
    printk("%s\n", s);
    Page *page;
    pageAlloc(&page);
    u64 perm = PTE_READ_BIT | PTE_WRITE_BIT;
    u64 va = 0x84000000;
    pageInsert(kernelPageDirectory, va, page, perm);
    u64 pa = page2PA(page);
    s = (char *)va;
    char str2[] = "Pass Memory Test! (2/2)";
    memmove(s, str2, strlen(str2));
    printk("%s\n", (char *)pa);
}
```

### fork 测试

测试 fork 功能是否正常：

```c
int main(int argc, char **argv) {
    printf("start fork test....\n");
    int a = 0;
    int id = 0;
    if ((id = fork()) == 0) {
        if ((id = fork()) == 0) {
            a += 3;
            if ((id = fork()) == 0) {
                a += 4;
                for (;;) {
                    printf("   this is child3 :a:%d\n", a);
                }
            }
            for (;;) {
                printf("  this is child2 :a:%d\n", a);
            }
        }
        a += 2;
        for (;;) {
            printf(" this is child :a:%d\n", a);
        }
    }
    a++;
    for (;;) {
        printf("this is father: a:%d\n", a);
    }
}
```



## 区域赛测试

### 测试要求

* 测试目标：系统调用
* 测试方式：官方提供的一组二进制文件（程序），交给 OS 运行并输出结果

由于官方程序使用自己实现的 `syscall` 命令 **直接** 写寄存器 `a0`-`a5`,`a7` 和调用 `ecall`，我们只需要维护系统调用号对应的内核函数，不需要包装一个函数库给测试程序使用。

### 编译测试用例

详见[官方测试用例仓库](https://github.com/oscomp/testsuits-for-oskernel/tree/pre-2023/riscv-syscalls-testing)。样例库中有 `make` 辅助编译测试需要的二进制文件。

### 本地测试磁盘

在本地测试时，我们需要构造一个和官方测试相同的磁盘，我们首先将 `testsuits-for-oskernel/riscv-syscalls-testing/user/build/riscv64/` 下的二进制文件都复制到 `Exaros/testfile/` 下，然后利用 `make fat` 命令来构建磁盘，其原理是在宿主机上挂载磁盘后将 `testfile` 移入磁盘，再卸载。

```makefile
# 制作 FAT 格式的文件系统镜像
fat: $(user_dir)
	if [ ! -f "$(fs_img)" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=$(fs_img) bs=8M count=5; fi
	mkfs.vfat -F 32 $(fs_img); 
	@sudo mount $(fs_img) $(mnt_path)
	@sudo cp -r user/target/* $(mnt_path)/
	@sudo cp -r testcase/** $(mnt_path)/
	@sudo umount $(mnt_path)
```

### 测试程序

我们的测试程序可以遍历所有的测试二进制文件，并在执行后自动关掉 qemu，程序如下

```c
#include "unistd.h"
#include "stdio.h"

char *syscallList[] = {
    "brk",
    "chdir",
    "clone",
    "close",
    "dup",
    "dup2",
    "execve",
    "exit",
    "fork",
    "fstat",
    "getcwd",
    "getdents",
    "getpid",
    "getppid",
    "gettimeofday",
    "mkdir_",
    "mmap",
    "mount",
    "munmap",
    "open",
    "openat",
    "pipe",
    "read",
    "sleep",
    "test_echo",
    "times",
    "umount",
    "uname",
    "unlink",
    "wait",
    "waitpid",
    "write",
    "yield",
    // "sh",
};

char *argv[] = {NULL};
char *argp[] = {NULL};

int main()
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);

    for (int i = 0; i < sizeof(syscallList) / sizeof(char *); i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            execve(syscallList[i], argv, argp);
        }
        else
        {
            wait(0);
        }
    }
    shutdown();
    return 0;
}
```

其原理是对于每个样例，都 fork 出一个子进程并 exec 样例。

关机功能可以用 SBI 结合系统调用实现。

```c
void syscallShutdown()
{
#ifdef FAT_DUMP
    extern FileSystem *rootFileSystem;
    dumpDirMetas(rootFileSystem, &(rootFileSystem->root));
#endif
    SBI_CALL_0(SBI_SHUTDOWN);
    return;
}
```

