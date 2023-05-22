.section .text.entry
.globl _start
_start:
    mv a0, sp
    call __start_main
loop_:
    call loop_