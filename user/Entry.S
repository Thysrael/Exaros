    .section .text
    .globl _userStart
_userStart:
    addi a7, x0, 4
    addi a0, x0, 42
    ecall 
    call libMain
_userLoop:
    call _userLoop

    .section .buffer
    .align 12
    .globl printfBuffer
printfBuffer:
    .space 4 * 128

    .section .buffer
    .globl bufferLen
bufferLen:
    .word 0;