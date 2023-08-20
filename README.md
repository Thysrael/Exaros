# ExarOS

## 概述

![banner](./doc/img/banner.png)

艾克萨罗斯（ExarOS）是一款北京航空航天大学计算机学院强生、陈凝香和曹宏墚采用 C 语言开发的基于 RISCV64 的操作系统，提供部分 linux 系统调用。

在功能方面，目前全国赛第一阶段只有 `lmbench` 3 个测试点尚未调通，`iperf, netperf` 网络测试功能不支持，其他测试样例均完全通过。具体通过情况见下表：

| 测试     | busybox | libctest | lua  | libcbench | iozone | cyclictest | unixbench | lmbench | iperf | netperf |
| -------- | ------- | -------- | ---- | --------- | ------ | ---------- | --------- | ------- | ----- | ------- |
| 样例个数 | 54      | 220      | 9    | 27        | 8      | 4          | 27        | 24      | 6     | 5       |
| 通过个数 | 54      | 220      | 9    | 27        | 8      | 4          | 27        | 21      | 0     | 0       |

在性能方面，我们与 qemu 上运行的 linux 进行对比，取得了较为优异的表现，具体的数据可以参考[性能总结](./doc/perfomance.md)文档。

此外，我们还移植了 musl-gcc。

---



## 环境与工具

| 条目       | 工具                    | 版本   |
| ---------- | ----------------------- | ------ |
| 交叉编译器 | riscv64-unknown-elf-gcc | 12.2.0 |
| 硬件模拟器 | qemu-system-riscv64     | 7.0.0  |
| SBI        | openSBI                 |        |
| 开发板     | HiFive Unmatched 开发板 |        |
| 调试器     | gdb-multiarch           | 12.1   |

---



## 编译与运行

宿主机需要为 Linux 系统，并具有区域赛要求的[开发环境和工具]([ttps://github.com/loboris/ktool/tree/master/kendryte-toolchain/bin](https://github.com/loboris/ktool/tree/master/kendryte-toolchain/bin))。

运行如下指令编译内核：

```shell
make 
```

使用如下指令在 qemu 上运行内核，此时 qemu 模拟的开发板是 hifive_u：

```shell
make run
```

如果希望 qemu 模拟的开发板是 virt，需要先修改 `Exraos/include/arch.h` 为如下内容

```c
#define QEMU
#define VIRT
```

然后执行

```shell
make	  # compile
make virt # run
```

如果希望上板测试，需要修改 `Exraos/include/arch.h` 为如下内容

```c
// #define QEMU
// #define VIRT
```

然后编译

```makefile
make
```

我们采用的方式是利用 uboot 网络加载内核和初始 sd 卡，需要在宿主机上搭建 tftp 服务器，并在开发板的 uboot 上进行相关配置，具体的流程可以参考文档[board](./doc/board.md) 。

---



## 仓库架构

```shell
ExarOS
├── doc (项目文档)
│   └── img (项目图片)
├── include (头文件)
│   └── assembly 
├── kernel (内核实现)
│   ├── driver (驱动)
│   ├── entry (内核入口)
│   ├── fs (文件系统)
│   ├── memory (内存)
│   ├── net (网络)
│   ├── process (进程管理)
│   ├── trap (异常及加载)
│   └── util (辅助工具)
├── linkscript (链接脚本)
├── testcase (测试样例)
│   └── mnt 
├── user (用户程序)
│   ├── include(用户头文件)
│   │   └── arch
│   │       └── riscv
│   └── target 
└── utility (辅助工具)
```

---



## 设计文档

### 软件管理

- [软件规范](./doc/convention.md)：包括注释规范、代码风格规范、git commit 规范。
- [docker环境](./doc/docker.md)：符合官方要求的工具链 docker，并增设了自动化脚本，方便本地调试。
- [开发资源](./doc/resource.md)：记录开发手册、参考代码、前人博客和视频等资源。

### 开发环境

- [硬件平台](./doc/)：`machine virt` 平台的特性。
- [构建脚本](./doc/make.md)：编译器参数、qemu 参数、自动化脚本、GUI 调试。
- [SBI](./doc/sbi.md)：SBI 相关知识。
- [开发板](./doc/board.md)：包括搭建 tftp 服务器和 uboot 启动。

### 内核设计

- [内核启动](./doc/boot.md)：启动流程和 SBI 应用。
- [内存管理](./doc/memory.md)：虚拟内存布局和内存管理。
- [trap](./doc/trap.md)：包括相关概念，中断和异常处理。
- [进程管理](./doc/process.md)：进程管理、进程调度、睡眠锁。
- [系统调用](./doc/syscall.md)：系统调用框架。
- [virtio 驱动](./doc/virtio.md)：virtio 驱动实现。
- [FAT32](./doc/fat.md)：FAT32 文件系统基础理论介绍。
- [文件系统](./doc/fs.md)：分层文件系统各层次介绍。
- [多核启动](./doc/multicore.md)：多核启动相关。
- [信号](./doc/signal.md)：信号的相关概念和设计。
- [调度](./doc/sched.md)：新型调度方案，实现了 linux 相关的系统调用。
- [动态链接](./doc/dynamic.md)：动态链接的概念，需求，设计，实现。
- [线程](./doc/process.md)：线程相关。
- [套接字](./doc/socket.md)：套接字的概念、时序逻辑和接口。
- [网络协议栈](./doc/net.md)：网络协议栈的设计和实现思路，未能完成的教训。
- [网卡驱动](./doc/macb.md)：macb 网卡驱动的实现思路。
- [共享内存](./doc/shm.md)：共享内存的设计和实现。

### 内核优化

- [懒加载](./doc/lazyload.md)：只在用户进程需要的时候，将数据加载入内存，节省了加载的开支。
- [外存写回](./doc/extern.md)：外存优化，包括缓存的设计和写回策略的选择。
- [临时文件](./doc/tmpfile.md)：在 `/tmp/` 文件夹下的文件为临时文件，不在放到 SD 卡内，而是在内存中进行维护。
- [UART移植](./uart.md)：将 UART 移植到 S 级，避免了陷入 M 级的时间。

### 用户程序

- [用户程序设计](./doc/user.md)：用户函数库、初始化事项等。
- [shell](./doc/shell.md)：ThyShell 设计。

### 测试

- [赛题分析](precomp.md)：测试形式和样例的分析。
- [测试设计](./doc/test.md)：包括官方样例测试和模块测试。
- [bug](./bug.md)：记录开发过程中遇到的十分困难或者离奇的 bug。
- [TODO](./todo.md)：记录内核还需要完善和拓展的地方。
