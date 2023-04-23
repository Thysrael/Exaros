#include <assembly/trapframe.h>
.global userHandler
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

    # 获取 trapframe 的地址，这个地址保存在 sscratch 中
    # 保存寄存器
    # 调用异常处理函数 userHandler，这个函数的地址保存在 trapframe 中
    # 恢复寄存器
    
    # trap.c sets stvec to point here, so
    # traps from user space start here,
    # in supervisor mode, but with a
    # user page table.
    #
    # sscratch points to where the process's p->trapframe is
    # mapped into user space, at TRAPFRAME.
    #
        
	# swap a0 and sscratch
    # so that a0 is TRAPFRAME
    csrrw a0, sscratch, a0

    ld t0, KERNEL_SATP(a0)
    csrw satp, t0
    sfence.vma

    # save the user registers in TRAPFRAME
    sd ra, REG_RA(a0)
    sd sp, REG_SP(a0)
    sd gp, REG_GP(a0)
    sd tp, REG_TP(a0)
    sd t0, REG_T0(a0)
    sd t1, REG_T1(a0)
    sd t2, REG_T2(a0)
    sd s0, REG_S0(a0)
    sd s1, REG_S1(a0)
    sd a1, REG_A1(a0)
    sd a2, REG_A2(a0)
    sd a3, REG_A3(a0)
    sd a4, REG_A4(a0)
    sd a5, REG_A5(a0)
    sd a6, REG_A6(a0)
    sd a7, REG_A7(a0)
    sd s2, REG_S2(a0)
    sd s3, REG_S3(a0)
    sd s4, REG_S4(a0)
    sd s5, REG_S5(a0)
    sd s6, REG_S6(a0)
    sd s7, REG_S7(a0)
    sd s8, REG_S8(a0)
    sd s9, REG_S9(a0)
    sd s10, REG_S10(a0)
    sd s11, REG_S11(a0)
    sd t3, REG_T3(a0)
    sd t4, REG_T4(a0)
    sd t5, REG_T5(a0)
    sd t6, REG_T6(a0)

	# save the user a0 in p->trapframe->a0
    csrr t0, sscratch
    sd t0, REG_A0(a0)
    # restore kernel stack pointer from p->trapframe->kernel_sp
    # todo
    # ld sp, KERNEL_SP(a0)
    # # make tp hold the current hartid, from p->trapframe->kernel_hartid
    ld tp, KERNEL_HART_ID(a0)
    # load the address of usertrap(), p->trapframe->kernel_trap

    csrr t1, sepc
    # restore kernel page table from p->trapframe->kernel_satp
    sd t1, EPC(a0)    
    # restore kernel page table from p->trapframe->kernel_satp
    ld t0, TRAP_HANDLER(a0)
    jr t0


.globl userReturn
userReturn:
    # userret(TRAPFRAME, pagetable)
    # switch from kernel to user.
    # usertrapret() calls here.
    # a0: TRAPFRAME, in user page table.
    # a1: user page table, for satp.
    # switch to the user page table.

    csrw satp, a1
    # sfence.vma zero, zero
    sfence.vma

    # put the saved user a0 in sscratch, so we
    # can swap it with our a0 (TRAPFRAME) in the last step.
    ld t0, REG_A0(a0)
    csrw sscratch, t0
    ld t1, EPC(a0)
    csrw sepc, t1
    # restore all but a0 from TRAPFRAME
    ld ra, REG_RA(a0)
    ld sp, REG_SP(a0)
    ld gp, REG_GP(a0)
    ld tp, REG_TP(a0)
    ld t0, REG_T0(a0)
    ld t1, REG_T1(a0)
    ld t2, REG_T2(a0)
    ld s0, REG_S0(a0)
    ld s1, REG_S1(a0)
    ld a1, REG_A1(a0)
    ld a2, REG_A2(a0)
    ld a3, REG_A3(a0)
    ld a4, REG_A4(a0)
    ld a5, REG_A5(a0)
    ld a6, REG_A6(a0)
    ld a7, REG_A7(a0)
    ld s2, REG_S2(a0)
    ld s3, REG_S3(a0)
    ld s4, REG_S4(a0)
    ld s5, REG_S5(a0)
    ld s6, REG_S6(a0)
    ld s7, REG_S7(a0)
    ld s8, REG_S8(a0)
    ld s9, REG_S9(a0)
    ld s10, REG_S10(a0)
    ld s11, REG_S11(a0)
    ld t3, REG_T3(a0)
    ld t4, REG_T4(a0)
    ld t5, REG_T5(a0)
    ld t6, REG_T6(a0)


#    sll t2, t2, 52
 #   srl t2, t2, 52
  #  addi t1, x0, 21
   # beq t1, t2, L2
# 可以确定的是 sepc 是正确的，a00000，但是读这个值会出错，但是在前面 trap.c 中的 usertrapret 中，读取 sepc 地址的值是正确的
#     addi t1, x0, 21
#     csrr t2, sepc
#     beq t1, t2, L2
#    addi t1,x0, 10
#    sll t1,t1,  20
#    csrr t2, sepc
#    beq t1, t2, L2
# L1: 
#     addi a0, x0, 42
#     addi a7, x0, 1
#     ecall

# L2: addi a0, x0, 42
#     addi a7, x0, 1
#     ecall


	# restore user a0, and save TRAPFRAME in sscratch
    csrrw a0, sscratch, a0
    # return to user mode and user pc.
    # usertrapret() set up sstatus and sepc.
    addi a0, x0, 42
    addi a7, x0, 1
    ecall
    sret
