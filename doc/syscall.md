# 系统调用

## 整体框架

`include/syscall.h` 文件中定义了系统调用的编号，例如 `#define SYSCALL_PUTCHAR 4` 。

在 `kernel/yield/syscall.c` 文件中定义系统调用向量 `syscallVector` ，这是一个函数指针数组，序号为系统调用的编号，每个元素指向一个系统调用处理函数。

约定在系统调用的时候用寄存器 `a0-a6` 传递参数，用 `a7` 存放系统调用的编号，用 `a0` 传递返回值。

异常处理函数会根据 `a7` 调用 `syscallVector` 中对应的系统调用函数，系统调用函数可以用 `getHartTrapFrame()` 这个函数获取进入异常时保存寄存器的 `trapframe`，`trapframe->a0-a6` 就可以得到系统调用的参数。如果系统调用有返回值，直接修改 `trapframe->a0` 即可。

---

如果要添加一个系统调用，以下步骤可以参考：

1. 在 `include/syscall.h` 中定义宏 异常处理编号
2. 在 `kernel/yield/syscall.c` 中写一个用于处理这个异常的函数，格式如 `syscall*`，并把这个函数添加到 `syscallVector` 中
3. 在用户态调用 `msyscall` 进行测试

---

官方系统调用的编号参考：[Linux 5.10](https://github.com/torvalds/linux/blob/v5.10/include/uapi/asm-generic/unistd.h)