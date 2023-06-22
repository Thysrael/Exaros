# Make

## RV64

现代操作系统一般都是使用 64 位架构进行开发。使用 64 位架构的主要优势在于可以使用更多的内存和更强大的处理器，从而提供更好的性能和吞吐量。具体来说，64 位架构支持更大的虚拟内存地址空间和物理内存容量，可以支持更大的程序和数据集。此外，64 位架构还支持硬件级别的内存保护和更强大的浮点运算能力，在某些应用场景下也非常重要。所以我们选择 64 位的交叉编译器去开发我们的操作系统。

```makefile
# use 64 bit arch cross compiler
CROSS_COMPILE	:= riscv64-unknown-elf-
```

`riscv64-unknown-elf-gcc` 编译器的默认 `-march` 选项是 `rv64imafd`。这个选项指示编译器生成针对 RV64I、M、A、F 和 D 标准的 RISC-V 64 位 ELF 可执行文件。

具体来说，`-march=rv64imafd` 选项分别表示：

- `rv64i` 表示基础标准，64 位寄存器的整型指令集。
- `m` 表示常见的整型乘除扩展指令，包括和、异或和求和指令。
- `a` 表示原子操作指令集，用于并发编程。
- `f` 表示 32 位浮点指令集。
- `d` 表示 64 位浮点指令集。

`rv64imafd` 实际上是 `rv64g` 指令集的一个子集，具体区别如下：

- `rv64g` 是 RISC-V 64 位指令集的一个扩展，它包括 `rv64i`（基本整数指令集）、`rv64m`（整数乘法/除法扩展）、`rv64a`（原子指令扩展）以及 `rv64f` 和 `rv64d`（浮点指令扩展）。而 `rv64imafd` 是基于这些扩展的一部分组成的子集，缺少了 `g` 扩展，即缺少了压缩指令集扩展（C）。
- `rv64g` 中增加了两个压缩指令集扩展，分别是 `C` 和 `Z` 扩展，其中 `C` 扩展是基于乘法的指令集扩展，可以生成加、减、比较和分支指令，而 `Z` 扩展是用于压缩的指令集扩展，可以将指令压缩到 16 位。这些指令集扩展可以提高处理器性能和代码密度，同时还能减少能耗和成本。
- `rv64imafd` 指令集的主要特点是支持整数运算、软件中断、浮点数运算等基本指令集，但不包括压缩指令扩展。因此，在空间上占用更大一些，但也可运行更多的软件。

总之，`rv64imafd` 和 `rv64g` 在 RISC-V 64 位指令集的基础上，使用不同的指令扩展进行了不同的优化和扩展。对于具体的应用场景，可以根据需要选择不同的指令集，以达到更好的性能和代码密度。

---



## GCC Flags

| 参数                     | 含义                                                                                       |
| ------------------------ | ------------------------------------------------------------------------------------------ |
| -Wall                    | 发现所有的警告信息                                                                         |
| -Werror                  | 将警告信息作为错误处理，即若有警告信息产生，则直接停止编译                                 |
| -O                       | 优化编译，可以提高程序的运行效率                                                           |
| -fno-omit-frame-pointer  | 确保生成的代码可以准确地将控制流信息通知给调试器                                           |
| -ggdb -g                 | 产生供GDB使用的调试信息，方便程序的调试和追踪                                              |
| -march=rv64g -mabi=lp64f | 设置目标平台的指令集架构和数据模型，即64位RISC-V指令集架构和小端序的LP64数据模型           |
| -MD                      | 生成依赖关系文件                                                                           |
| -mcmodel=medany          | 指定生成目标代码的内存模型，适用于任何内存大小，但需要通过中等通用的加载程序段（GP）来寻址 |
| -mcmodel=medany          | 指定了生成的目标代码的内存模型                                                             |
| -ffreestanding           | 剔除依赖于操作系统的部分，使生成的二进制程序可以独立运行                                   |
| -fno-common              | 不允许重复定义共享变量                                                                     |
| -nostdlib                | 不使用标准库                                                                               |
| -mno-relax               | 禁用relaxation，生成的汇编代码不应该包含relaxation指令                                     |
| -I.                      | 加入当前目录到头文件的搜索路径中                                                           |
| -fno-stack-protector     | 禁用堆栈保护功能，降低程序负担                                                             |



## LD Flags

| 参数                  | 含义                       |
| --------------------- | -------------------------- |
| -z max-page-size=4096 | 允许的最大页大小为4096字节 |



## Qemu Flags

这里列举了一些 Qemu 的常见参数

| 参数名     | 参数值                | 解释                                                         |
| ---------- | --------------------- | ------------------------------------------------------------ |
| machine    | virt                  | 这个标志指定了QEMU虚拟机的机器类型为虚拟化主机               |
| -bios      | default               | 这个标志表示使用默认的BIOS镜像，BIOS负责检查硬件设备并引导操作系统 |
| -m         | 128M                  | 这个标志表示设置虚拟机的内存大小为128MB                      |
| -nographic |                       | 这个标志表示在启动时切换到非图形化控制台模式，可以在命令行环境下运行虚拟机 |
| -d         | in_asm                | 在模拟器中跟踪和记录指令级别的执行过程并输出汇编指令         |
| -d         | int                   | 输出模拟器模拟执行时发生的中断、异常等事件的详细信息         |
| -s         |                       | QEMU 将启动一个 GDB 服务器并监听本地主机的 1234 端口（默认值），可以让 gdb 连接 QEMU 内的调试服务器，通过 gdb 命令来控制 QEMU 的执行、查询内存和寄存器等信息，方便调试应用程序 |
| -S         |                       | 在启动时暂停虚拟机，等待 gdb 连接，等待 gdb 客户端的连接后才会开始执行。此选项用于在 QEMU 启动时挂起，以进行远程调试 |
| -drive     |                       | 建了一个可以被设备使用的驱动器。驱动器是一个包含磁盘镜像文件的虚拟存储设备 |
|            | file=sdcard.img       | 指定了驱动器使用的磁盘镜像文件的名称 `sdcard.img`            |
|            | if=none               | 指定了驱动器的接口类型，值 none 意味着驱动器没有连接到任何接口，只能被指定了驱动器 id 的设备使用 |
|            | format=raw            | 指定了磁盘镜像文件的格式，值 raw 意味着文件包含了没有任何元数据或压缩的原始二进制数据。 |
|            | id=x0                 | 这指定了驱动器的 id，用于和 `virtio` 作用                    |
| -device    | virtio-blk-device     | virtio 块设备是一种使用 virtio 接口与主机系统通信的虚拟磁盘  |
|            | drive=x0              | 设备使用 id 为 x0 的驱动器作为其数据源                       |
|            | bus=virtio-mmio-bus.0 | 设备连接到 id 为 virtio-mmio-bus.0 的总线上                  |
| -initrd    | initrd.img            | 指定了初始 ramdisk 镜像文件为 initrd.img                     |

在计算机系统中，硬盘是一种存储设备，它可以读写数据并将其保存在磁盘上。然而，在早期的计算机系统中，硬盘并不是像现代计算机那样直接连接到主板上并通过总线与其他设备通信的。相反，它们是通过一种称为“硬盘控制器”的独立设备连接到计算机系统中。 

硬盘控制器是一种专门的设备，它负责控制硬盘的读写操作，并将数据传输到计算机系统的内存中。因此，硬盘控制器本身就是一种设备，而硬盘则是连接到该设备上的存储介质。这种设计模式一直延续到现代计算机系统中，因此，我们仍然将硬盘视为一种驱动器，而不是一种设备。 

当我们在虚拟化平台中模拟硬盘时，我们也需要模拟硬盘控制器和硬盘之间的通信。因此，在 QEMU 中，我们使用 -drive 参数来指定虚拟机中的硬盘驱动器，同时使用 -device 参数来指定虚拟机中的硬盘控制器设备。这种设计模式与实际硬件系统的设计相似，因此可以更好地模拟真实的计算机系统。



## debug

debug 的原理是利用 qemu 启动的服务器监听主机的 `1234` 端口，gdb 连接这个服务器，用 gdb 命令来控制 qemu 的执行，对其上的操作系统进行调试。

所以在进行这个过程的时候，需要开启两个终端，一个终端运行 `make debug` ，即如下命令

```makefile
$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -s -S
```

而另一个终端与其进行通信，运行 `make gdb`，即如下命令（此时加载 `elf` 应该是为了获得符号表等信息）

```shell
$(GDB) $(exaros_elf)
```

在 `gdb` 这个进程中，需要先输入以下命令进行连接

```shell
(gdb) target extended-remote localhost:1234
```

为了自动化，避免多次输入，可以在项目根目录下新建 `.gdbinit` 文件，避免每次的输入

```shell
target extended-remote localhost:1234
```

有一定概率这个 `init` 文件不会被加载，因为不太安全，此时可以在 `~/.config/gdb/gdbinit` 中加入如下指令即可

```shell
add-auto-load-safe-path <progject->/.gdbinit
```

`gdb` 的命令如下

| 命令               | 缩写   | 含义                                 |
| ------------------ | ------ | ------------------------------------ |
| `file`             |        | 加载一个可执行文件                   |
| `run`              | `r`    | 运行程序                             |
| `breakpoint`       | `b`    | 在指定位置设置断点                   |
| `delete`           | `d`    | 删除一个或多个断点                   |
| `continue`         | `c`    | 继续执行程序                         |
| `step`             | `s`    | 单步执行程序，并进入任何被调用的函数 |
| `next`             | `n`    | 单步执行程序，但不进入被调用的函数   |
| `finish`           | `fin`  | 执行程序，直到当前函数返回           |
| `up`               | `u`    | 进入当前调用堆栈的上层函数           |
| `down`             | `d`    | 进入当前调用堆栈的下层函数           |
| `print`            | `p`    | 显示一个变量的值                     |
| `set`              |        | 设置一个变量的值                     |
| `display`          | `disp` | 启动一个计算某个值的监视             |
| `undisplay`        |        | 取消一个监视                         |
| `list`             | `l`    | 显示源代码                           |
| `backtrace`        | `bt`   | 显示调用链和函数参数                 |
| `info breakpoints` | `i b`  | 显示断点列表                         |
| `info functions`   | `i f`  | 显示程序中的函数列表                 |
| `info locals`      | `i lo` | 显示当前函数中的局部变量             |
| `info variables`   | `i va` | 显示当前程序中所有可访问的全局变量   |
| `quit`             | `q`    | 退出gdb                              |





## vscode debug

本文参考了散步的[在 vscode 上完美调试 xv6](https://sanbuphy.github.io/p/%E4%BC%98%E9%9B%85%E7%9A%84%E8%B0%83%E8%AF%95%E5%9C%A8vscode%E4%B8%8A%E5%AE%8C%E7%BE%8E%E8%B0%83%E8%AF%95xv6/)。

当你希望调试的时候，你应当先 `make`，然后再按 `F5` 即可。

### launch.json

`.vscode/launch.json` 文件可以对调试进行设置，我们将如下内容写入到文件中

```json
//launch.json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "gdb",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/target/exaros.elf",
            "args": [],
            "stopAtEntry": true,
            "cwd": "${workspaceFolder}",
            "miDebuggerServerAddress": "localhost:1234",
            //这里实际上可以用各种能调试的 gdb，如果找不到你可以使用 which gdb-multiarch
            //但要注意的是，为了能在ubuntu20.04调出寄存器，强烈建议使用 riscv64 的 gdb 
            "miDebuggerPath": "/usr/local/bin/riscv64-unknown-elf-gdb",
            // "environment": [],
            // "externalConsole": false,
            "preLaunchTask": "exarosDebug",
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "pretty printing",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true,
                },
                //在这里加载了寄存器信息表
                {
                    "text": "set tdesc filename gdb.xml",
                }
            ],
            //用于gdb调试的工具，可以发现gdb出错的原因
            // "logging": {
            //     "engineLogging": true,
            //     "programOutput": true,
            // }
        }
    ]
}
```

此时需要注意，我们已经设置了远端调试的路径和端口，所以需要将 `.gdbinit` 中的内容删掉。

```json
"miDebuggerServerAddress": "localhost:1234"
```

下面这个条目指定了 gdb 的路径，最好使用 `riscv64-unknown-elf-gdb` ，不然可能无法调试出寄存器

```json
"miDebuggerPath": "/usr/local/bin/riscv64-unknown-elf-gdb",
```

下面这个条目指定了再调试前需要进行的任务，由前面一章可知，我们需要将 qemu 启动起来，这个部分也可以借用 vscode 自动化。

```json
"preLaunchTask": "exarosDebug"
```

### tasks.json

这是 vscode 中自动脚本的文件，我们在这里设置一个类似于 `make debug` 的脚本，如下所示

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "exarosDebug",
            "type": "shell",
            "isBackground": true,
            "command": "make debug",
            "problemMatcher": [
                {
                    "pattern": [
                        {
                            "regexp": ".",
                            "file": 1,
                            "location": 2,
                            "message": 3
                        }
                    ],
                    "background": {
                        "beginsPattern": ".*Now run 'gdb' in another window.",
                        "endsPattern": "."
                    }
                }
            ]
        }
    ]
}
```

初次运行的时候，可能会发生“The task 'exarosDebug' cannot be tracked..”的提示框，可以在勾选“remember……”之后选择 “Debug Anyway”即可。

### 显示寄存器

在 manjaro 上似乎无法直接显示寄存器，可以按照散步大佬的方法，在项目目录下新建 `gdb.xml` 并粘贴如下内容即可

```xml
<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target>
  <architecture>riscv:rv64</architecture>
  <feature name="org.gnu.gdb.riscv.cpu">
    <reg name="zero" bitsize="64" type="int" regnum="0"/>
    <reg name="ra" bitsize="64" type="code_ptr" regnum="1"/>
    <reg name="sp" bitsize="64" type="data_ptr" regnum="2"/>
    <reg name="gp" bitsize="64" type="data_ptr" regnum="3"/>
    <reg name="tp" bitsize="64" type="data_ptr" regnum="4"/>
    <reg name="t0" bitsize="64" type="int" regnum="5"/>
    <reg name="t1" bitsize="64" type="int" regnum="6"/>
    <reg name="t2" bitsize="64" type="int" regnum="7"/>
    <reg name="fp" bitsize="64" type="data_ptr" regnum="8"/>
    <reg name="s1" bitsize="64" type="int" regnum="9"/>
    <reg name="a0" bitsize="64" type="int" regnum="10"/>
    <reg name="a1" bitsize="64" type="int" regnum="11"/>
    <reg name="a2" bitsize="64" type="int" regnum="12"/>
    <reg name="a3" bitsize="64" type="int" regnum="13"/>
    <reg name="a4" bitsize="64" type="int" regnum="14"/>
    <reg name="a5" bitsize="64" type="int" regnum="15"/>
    <reg name="a6" bitsize="64" type="int" regnum="16"/>
    <reg name="a7" bitsize="64" type="int" regnum="17"/>
    <reg name="s2" bitsize="64" type="int" regnum="18"/>
    <reg name="s3" bitsize="64" type="int" regnum="19"/>
    <reg name="s4" bitsize="64" type="int" regnum="20"/>
    <reg name="s5" bitsize="64" type="int" regnum="21"/>
    <reg name="s6" bitsize="64" type="int" regnum="22"/>
    <reg name="s7" bitsize="64" type="int" regnum="23"/>
    <reg name="s8" bitsize="64" type="int" regnum="24"/>
    <reg name="s9" bitsize="64" type="int" regnum="25"/>
    <reg name="s10" bitsize="64" type="int" regnum="26"/>
    <reg name="s11" bitsize="64" type="int" regnum="27"/>
    <reg name="t3" bitsize="64" type="int" regnum="28"/>
    <reg name="t4" bitsize="64" type="int" regnum="29"/>
    <reg name="t5" bitsize="64" type="int" regnum="30"/>
    <reg name="t6" bitsize="64" type="int" regnum="31"/>
    <reg name="pc" bitsize="64" type="code_ptr" regnum="32"/>
  </feature>
  <feature name="org.gnu.gdb.riscv.fpu">
    <union id="riscv_double">
      <field name="float" type="ieee_single"/>
      <field name="double" type="ieee_double"/>
    </union>
    <reg name="ft0" bitsize="64" type="riscv_double" regnum="33"/>
    <reg name="ft1" bitsize="64" type="riscv_double" regnum="34"/>
    <reg name="ft2" bitsize="64" type="riscv_double" regnum="35"/>
    <reg name="ft3" bitsize="64" type="riscv_double" regnum="36"/>
    <reg name="ft4" bitsize="64" type="riscv_double" regnum="37"/>
    <reg name="ft5" bitsize="64" type="riscv_double" regnum="38"/>
    <reg name="ft6" bitsize="64" type="riscv_double" regnum="39"/>
    <reg name="ft7" bitsize="64" type="riscv_double" regnum="40"/>
    <reg name="fs0" bitsize="64" type="riscv_double" regnum="41"/>
    <reg name="fs1" bitsize="64" type="riscv_double" regnum="42"/>
    <reg name="fa0" bitsize="64" type="riscv_double" regnum="43"/>
    <reg name="fa1" bitsize="64" type="riscv_double" regnum="44"/>
    <reg name="fa2" bitsize="64" type="riscv_double" regnum="45"/>
    <reg name="fa3" bitsize="64" type="riscv_double" regnum="46"/>
    <reg name="fa4" bitsize="64" type="riscv_double" regnum="47"/>
    <reg name="fa5" bitsize="64" type="riscv_double" regnum="48"/>
    <reg name="fa6" bitsize="64" type="riscv_double" regnum="49"/>
    <reg name="fa7" bitsize="64" type="riscv_double" regnum="50"/>
    <reg name="fs2" bitsize="64" type="riscv_double" regnum="51"/>
    <reg name="fs3" bitsize="64" type="riscv_double" regnum="52"/>
    <reg name="fs4" bitsize="64" type="riscv_double" regnum="53"/>
    <reg name="fs5" bitsize="64" type="riscv_double" regnum="54"/>
    <reg name="fs6" bitsize="64" type="riscv_double" regnum="55"/>
    <reg name="fs7" bitsize="64" type="riscv_double" regnum="56"/>
    <reg name="fs8" bitsize="64" type="riscv_double" regnum="57"/>
    <reg name="fs9" bitsize="64" type="riscv_double" regnum="58"/>
    <reg name="fs10" bitsize="64" type="riscv_double" regnum="59"/>
    <reg name="fs11" bitsize="64" type="riscv_double" regnum="60"/>
    <reg name="ft8" bitsize="64" type="riscv_double" regnum="61"/>
    <reg name="ft9" bitsize="64" type="riscv_double" regnum="62"/>
    <reg name="ft10" bitsize="64" type="riscv_double" regnum="63"/>
    <reg name="ft11" bitsize="64" type="riscv_double" regnum="64"/>
  </feature>
  <feature name="org.gnu.gdb.riscv.virtual">
    <reg name="priv" bitsize="64" type="int" regnum="65"/>
  </feature>
  <feature name="org.gnu.gdb.riscv.csr">
    <reg name="sstatus" bitsize="64" type="int" regnum="322"/>
    <reg name="sie" bitsize="64" type="int" regnum="326"/>
    <reg name="stvec" bitsize="64" type="int" regnum="327"/>
    <reg name="scounteren" bitsize="64" type="int" regnum="328"/>
    <reg name="senvcfg" bitsize="64" type="int" regnum="332"/>
    <reg name="sscratch" bitsize="64" type="int" regnum="386"/>
    <reg name="sepc" bitsize="64" type="int" regnum="387"/>
    <reg name="scause" bitsize="64" type="int" regnum="388"/>
    <reg name="stval" bitsize="64" type="int" regnum="389"/>
    <reg name="sip" bitsize="64" type="int" regnum="390"/>
    <reg name="satp" bitsize="64" type="int" regnum="450"/>
    <reg name="vsstatus" bitsize="64" type="int" regnum="578"/>
    <reg name="vsie" bitsize="64" type="int" regnum="582"/>
    <reg name="vstvec" bitsize="64" type="int" regnum="583"/>
    <reg name="vsscratch" bitsize="64" type="int" regnum="642"/>
    <reg name="vsepc" bitsize="64" type="int" regnum="643"/>
    <reg name="vscause" bitsize="64" type="int" regnum="644"/>
    <reg name="vstval" bitsize="64" type="int" regnum="645"/>
    <reg name="vsip" bitsize="64" type="int" regnum="646"/>
    <reg name="vsatp" bitsize="64" type="int" regnum="706"/>
    <reg name="mstatus" bitsize="64" type="int" regnum="834"/>
    <reg name="misa" bitsize="64" type="int" regnum="835"/>
    <reg name="medeleg" bitsize="64" type="int" regnum="836"/>
    <reg name="mideleg" bitsize="64" type="int" regnum="837"/>
    <reg name="mie" bitsize="64" type="int" regnum="838"/>
    <reg name="mtvec" bitsize="64" type="int" regnum="839"/>
    <reg name="mcounteren" bitsize="64" type="int" regnum="840"/>
    <reg name="menvcfg" bitsize="64" type="int" regnum="844"/>
    <reg name="mcountinhibit" bitsize="64" type="int" regnum="866"/>
    <reg name="mhpmevent3" bitsize="64" type="int" regnum="869"/>
    <reg name="mhpmevent4" bitsize="64" type="int" regnum="870"/>
    <reg name="mhpmevent5" bitsize="64" type="int" regnum="871"/>
    <reg name="mhpmevent6" bitsize="64" type="int" regnum="872"/>
    <reg name="mhpmevent7" bitsize="64" type="int" regnum="873"/>
    <reg name="mhpmevent8" bitsize="64" type="int" regnum="874"/>
    <reg name="mhpmevent9" bitsize="64" type="int" regnum="875"/>
    <reg name="mhpmevent10" bitsize="64" type="int" regnum="876"/>
    <reg name="mhpmevent11" bitsize="64" type="int" regnum="877"/>
    <reg name="mhpmevent12" bitsize="64" type="int" regnum="878"/>
    <reg name="mhpmevent13" bitsize="64" type="int" regnum="879"/>
    <reg name="mhpmevent14" bitsize="64" type="int" regnum="880"/>
    <reg name="mhpmevent15" bitsize="64" type="int" regnum="881"/>
    <reg name="mhpmevent16" bitsize="64" type="int" regnum="882"/>
    <reg name="mhpmevent17" bitsize="64" type="int" regnum="883"/>
    <reg name="mhpmevent18" bitsize="64" type="int" regnum="884"/>
    <reg name="mhpmevent19" bitsize="64" type="int" regnum="885"/>
    <reg name="mhpmevent20" bitsize="64" type="int" regnum="886"/>
    <reg name="mhpmevent21" bitsize="64" type="int" regnum="887"/>
    <reg name="mhpmevent22" bitsize="64" type="int" regnum="888"/>
    <reg name="mhpmevent23" bitsize="64" type="int" regnum="889"/>
    <reg name="mhpmevent24" bitsize="64" type="int" regnum="890"/>
    <reg name="mhpmevent25" bitsize="64" type="int" regnum="891"/>
    <reg name="mhpmevent26" bitsize="64" type="int" regnum="892"/>
    <reg name="mhpmevent27" bitsize="64" type="int" regnum="893"/>
    <reg name="mhpmevent28" bitsize="64" type="int" regnum="894"/>
    <reg name="mhpmevent29" bitsize="64" type="int" regnum="895"/>
    <reg name="mhpmevent30" bitsize="64" type="int" regnum="896"/>
    <reg name="mhpmevent31" bitsize="64" type="int" regnum="897"/>
    <reg name="mscratch" bitsize="64" type="int" regnum="898"/>
    <reg name="mepc" bitsize="64" type="int" regnum="899"/>
    <reg name="mcause" bitsize="64" type="int" regnum="900"/>
    <reg name="mtval" bitsize="64" type="int" regnum="901"/>
    <reg name="mip" bitsize="64" type="int" regnum="902"/>
    <reg name="mtinst" bitsize="64" type="int" regnum="908"/>
    <reg name="mtval2" bitsize="64" type="int" regnum="909"/>
    <reg name="pmpcfg0" bitsize="64" type="int" regnum="994"/>
    <reg name="pmpcfg1" bitsize="64" type="int" regnum="995"/>
    <reg name="pmpcfg2" bitsize="64" type="int" regnum="996"/>
    <reg name="pmpcfg3" bitsize="64" type="int" regnum="997"/>
    <reg name="pmpaddr0" bitsize="64" type="int" regnum="1010"/>
    <reg name="pmpaddr1" bitsize="64" type="int" regnum="1011"/>
    <reg name="pmpaddr2" bitsize="64" type="int" regnum="1012"/>
    <reg name="pmpaddr3" bitsize="64" type="int" regnum="1013"/>
    <reg name="pmpaddr4" bitsize="64" type="int" regnum="1014"/>
    <reg name="pmpaddr5" bitsize="64" type="int" regnum="1015"/>
    <reg name="pmpaddr6" bitsize="64" type="int" regnum="1016"/>
    <reg name="pmpaddr7" bitsize="64" type="int" regnum="1017"/>
    <reg name="pmpaddr8" bitsize="64" type="int" regnum="1018"/>
    <reg name="pmpaddr9" bitsize="64" type="int" regnum="1019"/>
    <reg name="pmpaddr10" bitsize="64" type="int" regnum="1020"/>
    <reg name="pmpaddr11" bitsize="64" type="int" regnum="1021"/>
    <reg name="pmpaddr12" bitsize="64" type="int" regnum="1022"/>
    <reg name="pmpaddr13" bitsize="64" type="int" regnum="1023"/>
    <reg name="pmpaddr14" bitsize="64" type="int" regnum="1024"/>
    <reg name="pmpaddr15" bitsize="64" type="int" regnum="1025"/>
    <reg name="hstatus" bitsize="64" type="int" regnum="1602"/>
    <reg name="hedeleg" bitsize="64" type="int" regnum="1604"/>
    <reg name="hideleg" bitsize="64" type="int" regnum="1605"/>
    <reg name="hie" bitsize="64" type="int" regnum="1606"/>
    <reg name="htimedelta" bitsize="64" type="int" regnum="1607"/>
    <reg name="hcounteren" bitsize="64" type="int" regnum="1608"/>
    <reg name="hgeie" bitsize="64" type="int" regnum="1609"/>
    <reg name="henvcfg" bitsize="64" type="int" regnum="1612"/>
    <reg name="htval" bitsize="64" type="int" regnum="1669"/>
    <reg name="hip" bitsize="64" type="int" regnum="1670"/>
    <reg name="hvip" bitsize="64" type="int" regnum="1671"/>
    <reg name="htinst" bitsize="64" type="int" regnum="1676"/>
    <reg name="hgatp" bitsize="64" type="int" regnum="1730"/>
    <reg name="tselect" bitsize="64" type="int" regnum="2018"/>
    <reg name="tdata1" bitsize="64" type="int" regnum="2019"/>
    <reg name="tdata2" bitsize="64" type="int" regnum="2020"/>
    <reg name="tdata3" bitsize="64" type="int" regnum="2021"/>
    <reg name="tinfo" bitsize="64" type="int" regnum="2022"/>
    <reg name="mcycle" bitsize="64" type="int" regnum="2882"/>
    <reg name="minstret" bitsize="64" type="int" regnum="2884"/>
    <reg name="mhpmcounter3" bitsize="64" type="int" regnum="2885"/>
    <reg name="mhpmcounter4" bitsize="64" type="int" regnum="2886"/>
    <reg name="mhpmcounter5" bitsize="64" type="int" regnum="2887"/>
    <reg name="mhpmcounter6" bitsize="64" type="int" regnum="2888"/>
    <reg name="mhpmcounter7" bitsize="64" type="int" regnum="2889"/>
    <reg name="mhpmcounter8" bitsize="64" type="int" regnum="2890"/>
    <reg name="mhpmcounter9" bitsize="64" type="int" regnum="2891"/>
    <reg name="mhpmcounter10" bitsize="64" type="int" regnum="2892"/>
    <reg name="mhpmcounter11" bitsize="64" type="int" regnum="2893"/>
    <reg name="mhpmcounter12" bitsize="64" type="int" regnum="2894"/>
    <reg name="mhpmcounter13" bitsize="64" type="int" regnum="2895"/>
    <reg name="mhpmcounter14" bitsize="64" type="int" regnum="2896"/>
    <reg name="mhpmcounter15" bitsize="64" type="int" regnum="2897"/>
    <reg name="mhpmcounter16" bitsize="64" type="int" regnum="2898"/>
    <reg name="mhpmcounter17" bitsize="64" type="int" regnum="2899"/>
    <reg name="mhpmcounter18" bitsize="64" type="int" regnum="2900"/>
    <reg name="hgeip" bitsize="64" type="int" regnum="3668"/>
    <reg name="mvendorid" bitsize="64" type="int" regnum="3923"/>
    <reg name="marchid" bitsize="64" type="int" regnum="3924"/>
    <reg name="mimpid" bitsize="64" type="int" regnum="3925"/>
    <reg name="mhartid" bitsize="64" type="int" regnum="3926"/>
    <reg name="mconfigptr" bitsize="64" type="int" regnum="3927"/>
  </feature>
</target>
```

这个文件会通过 `launch.json` 的如下配置读入

```json
"text": "set tdesc filename gdb.xml",
```

