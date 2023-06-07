# 多核

## 多核启动

在 OpenSBI 中，多个核心将同时开始启动，最快的核心将以热启动的方式对整个系统进行配置，并来到操作系统的入口 0x80200000 ，该核心称为主核。

而对于其他三个副核，他们将停留在冷启动的 `sbi_hsm_hart_wait` 函数中：

```c
	while (atomic_read(&hdata->state) != SBI_HSM_STATE_START_PENDING) {
		wfi();
	};
```

直到主核向它们发送 IPI 中断并改变核的状态，副核才会继续执行并来到操作系统的入口 0x80200000 。

为了让多核正常工作，我们需要为他们分配不同的内核栈，如下所示

```assembly
    .section .stack
    .globl kernelStack
    .align 12
kernelStack:
    # every hart has a stack
    .space KERNEL_STACK_SIZE * CORE_NUM
    .globl kernelStackTop
kernelStackTop:
```

