# ExarOS

## 概述

![banner](./doc/img/banner.png)

艾克萨罗斯（ExarOS）是一款北京航空航天大学计算机学院强生、陈凝香和曹宏墚采用 C 语言开发的基于 RISCV64 的操作系统。参加操作系统内核设计赛道区域赛并全部通过测试样例。成绩如下：

| 条目         | 内容                |
| ------------ | ------------------- |
| **rank**     | 102.0000            |
| 提交次数     | 2                   |
| 最后提交时间 | 2023-06-03 12:22:29 |
| 排名         | 16                  |



## 环境与工具

| 条目       | 工具                    | 版本   |
| ---------- | ----------------------- | ------ |
| 交叉编译器 | riscv64-unknown-elf-gcc | 12.2.0 |
| 硬件模拟器 | qemu-system-riscv64     | 7.0.0  |
| SBI        | openSBI                 |        |
| 开发板     | HiFive Unmatched 开发板 |        |
| 调试器     | gdb-multiarch           | 12.1   |



## 编译与运行

宿主机需要为 Linux 系统，并具有区域赛要求的[开发环境和工具]([ttps://github.com/loboris/ktool/tree/master/kendryte-toolchain/bin](https://github.com/loboris/ktool/tree/master/kendryte-toolchain/bin))。

运行如下指令编译内核：

```shell
make 
```

然后运行如下命令生成磁盘镜像：

```shell
make fat
```

最后使用如下指令运行内核：

```shell
make run
```



## 内核功能

- [x] 内核启动
- [x] printf
- [x] 页表
- [x] 虚拟内存
- [x] 时钟中断
- [x] S 态外部中断
- [x] 异常处理
- [x] 系统调用框架
- [x] 进程调度管理
- [x] virtio 块设备驱动
- [x] FAT 文件系统
- [x] 文件系统优化
- [x] ThyShell 
- [x] 测试遍历程序



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
│   ├── lock (锁)
│   ├── memory (内存)
│   ├── trap (异常及加载)
│   ├── util (辅助工具)
│   └── yield (进程管理)
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



## 设计文档

### 软件管理

- [软件规范](./doc/convention.md)：包括注释规范、代码风格规范、git commit 规范。
- [docker环境](docker.md)：符合官方要求的工具链 docker，并增设了自动化脚本，方便本地调试。
- [开发资源](./doc/resource.md)：记录开发手册、参考代码、前人博客和视频等资源。

### 开发环境

- [硬件平台](./doc/)：`machine virt` 平台的特性。
- [构建脚本](./doc/make.md)：编译器参数、qemu 参数、自动化脚本、GUI 调试。
- [SBI](./doc/sbi.md)：SBI 相关知识。

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

### 用户程序

- [用户程序设计](./doc/user.md)：用户函数库、初始化事项等。
- [shell](./doc/shell.md)：ThyShell 设计。

### 测试

- [赛题分析](precomp.md)：测试形式和样例的分析。
- [测试设计](./doc/test.md)：包括官方样例测试和模块测试。
