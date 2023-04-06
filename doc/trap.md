# trap

## trapframe 

一个线程对应一个 trapframe，用于在异常发生时保存线程的上下文。

线程初始化的时候会把 trapframe 放在线程 sscratch 寄存器中，这样在发生用户态异常时，异常处理程序可以拿到 trapframe 的地址。

## 异常处理流程

## trampoline

`trampoline` 是一段地址，大小为 2 * pgsize，每个线程的页表都将用于错误处理的页 TRAMPOLINE ，从虚拟地址 TRAMPOLINE_BASE 映射到物理地址的trampoline。

在用户进程发生异常的时候，会跳转到 trampoline 中的 userTrap
