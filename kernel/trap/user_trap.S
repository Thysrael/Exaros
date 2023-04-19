
# 开一页的 trapframe
    .section .trapframe
    .globl trapframe
    .align 12
trapframe:
    .space 4096


	.section .trampoline
    .globl trampoline
    .align 12
trampoline:
    .global userTrap
    .align 2
userTrap:

    # todo
    # 获取 trapframe 的地址，这个地址保存在 sscratch 中
    # 保存寄存器
    # 调用异常处理函数 userHandler，这个函数的地址保存在 trapframe 中
    # 恢复寄存器
    
    sret


userReturn: