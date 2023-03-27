# Platform

## Platform

plateform 这个词是“硬件平台的意思”，可以看做是多种硬件构成的一种集合，在 RISCV 手册中介绍了其覆盖的范围：

> A RISC-V hardware platform can contain one or more RISC-V-compatible processing cores together with other non-RISC-V-compatible cores, fixed-function accelerators, various physical memory structures, I/O devices, and an interconnect structure to allow the components to communicate.

## Virt

### doc

在区域赛的时候，只要求进行 `qemu` 硬件模拟，选定的硬件平台是 `virt` ，这个平台只是一个与现实中实际的硬件平台不相关的虚拟硬件平台。但是我们可以在[这里](https://www.qemu.org/docs/master/system/riscv/virt.html)查阅它支持的设备和 `boot` 相关的操作。

以下是他支持的设备：

| 设备名称             | 数量     | 功能                                                         |
| -------------------- | -------- | ------------------------------------------------------------ |
| RV32GC/RV64GC 内核   | 最多 8个 | 处理器内核，支持 RV32GC 或 RV64GC 指令集，并且可以根据需要进行扩展 |
| CLINT                | 1个      | 可编程的计时和中断控制器，可提供与内核中断向量表的交互       |
| PLIC                 | 1个      | 硬件设备，用于在多个外部设备的中间分配中断并将它们传递到尽可能少的处理器核心 |
| CFI 并行 NOR 闪存    | 1个      | CFI NOR 是一种闪存卡接口标准，用于访问串行闪存芯片           |
| NS16550 兼容的 UART  | 1个      | 电路，用于在计算机和其他设备之间传输数据                     |
| Google Goldfish RTC  | 1个      | Android 模拟器的 RTC 设备                                    |
| SiFive 测试设备      | 1个      | 用于测试 SiFive IP 核的仿真设备                              |
| virtio-mmio 传输设备 | 最多 8个 | 一种虚拟化设备驱动程序接口，用于在主机和客户机之间高效地传输数据 |
| 通用的 PCIe 主机桥   | 1个      | 用于在 PCIe 总线上连接不同种类的设备的硬件设备               |
| fw_cfg 设备          | 1个      | 一种 QEMU 虚拟设备，可将信息存储在“固件配置”中，并从 QEMU 客户机访问该信息 |

在介绍 boot 的时候，它的描述如下：

> The `virt` machine can start using the standard -kernel functionality for loading a Linux kernel, a VxWorks kernel, an S-mode U-Boot bootloader with the default OpenSBI firmware image as the `-bios`. It also supports the recommended RISC-V bootflow: U-Boot SPL (M-mode) loads OpenSBI fw_dynamic firmware and U-Boot proper (S-mode), using the standard -bios functionality.

似乎是在说如果是 `-bios default` 时，`qemu` 会使用 `openSBI` 。

### code

除了在文档中获得信息外，我们还可以在源码中找到一些相关的信息，项目源码在 gitlab 上，地址[qemu](https://gitlab.com/qemu-project/qemu)。

在 `qemu/hw/riscv/` 下有关于不同平台的代码实现，我们在 `virt.c` 中可以看到 `memmap` 如下所示

```c
static const MemMapEntry virt_memmap[] = {
    [VIRT_DEBUG] =        {        0x0,         0x100 },
    [VIRT_MROM] =         {     0x1000,        0xf000 },
    [VIRT_TEST] =         {   0x100000,        0x1000 },
    [VIRT_RTC] =          {   0x101000,        0x1000 },
    [VIRT_CLINT] =        {  0x2000000,       0x10000 },
    [VIRT_ACLINT_SSWI] =  {  0x2F00000,        0x4000 },
    [VIRT_PCIE_PIO] =     {  0x3000000,       0x10000 },
    [VIRT_PLATFORM_BUS] = {  0x4000000,     0x2000000 },
    [VIRT_PLIC] =         {  0xc000000, VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2) },
    [VIRT_APLIC_M] =      {  0xc000000, APLIC_SIZE(VIRT_CPUS_MAX) },
    [VIRT_APLIC_S] =      {  0xd000000, APLIC_SIZE(VIRT_CPUS_MAX) },
    [VIRT_UART0] =        { 0x10000000,         0x100 },
    [VIRT_VIRTIO] =       { 0x10001000,        0x1000 },
    [VIRT_FW_CFG] =       { 0x10100000,          0x18 },
    [VIRT_FLASH] =        { 0x20000000,     0x4000000 },
    [VIRT_IMSIC_M] =      { 0x24000000, VIRT_IMSIC_MAX_SIZE },
    [VIRT_IMSIC_S] =      { 0x28000000, VIRT_IMSIC_MAX_SIZE },
    [VIRT_PCIE_ECAM] =    { 0x30000000,    0x10000000 },
    [VIRT_PCIE_MMIO] =    { 0x40000000,    0x40000000 },
    [VIRT_DRAM] =         { 0x80000000,           0x0 },
}; 
```





## Reference

- [QEMU 代码与 RISCV 'virt' 平台 ZSBL 分析](https://gitee.com/tinylab/riscv-linux/blob/master/articles/20220911-qemu-riscv-zsbl.md#https://gitee.com/link?target=https%3A%2F%2Fjuejin.cn%2Fpost%2F6891922292075397127)