ENTRY(_start)

BASE_ADDRESS    =   0x80200000;

SECTIONS 
{
    . = BASE_ADDRESS;
    PROVIDE(kernelStart = .);
    . = ALIGN(4K);

    PROVIDE(textStart = .);
    .text : 
    {
        *(.entry)
        *(.text .text.*)
        . = ALIGN(4K);

         _trampoline = .;
        *(.trapframe)
        . = ALIGN(4K);
        *(.trampoline)
        . = ALIGN(4K);
        ASSERT(. - _trampoline == 8192, "error: trampoline+trapframe not have two page");

        /* _signalTrampoline = .;
        *(.signalTrampoline)
        . = ALIGN(4K);
        ASSERT(. - _signalTrampoline == 4096, "error: signal trampoline not one page");  */
        
        PROVIDE(textEnd = .);
    }
    . = ALIGN(4K);

    .rodata : 
    {
        *(.rodata .rodata.*)
    }
    . = ALIGN(4K);

    .data : 
    {
        *(.data .data.*)
        . = ALIGN(4K);

        *(.stack)
    }
    . = ALIGN(4K);

    PROVIDE(bssStart = .);
    .bss : 
    {
        *(.bss .bss.* .sbss .sbss.*)
        . = ALIGN(4K);

        _pgdir = .;
        *(.pgdir)
        ASSERT(. - _pgdir == 4096, "error: pgdir not one page");
        . = ALIGN(4K);
        PROVIDE(bssEnd = .);
    }

    . = ALIGN(4K);
    PROVIDE(kernelEnd = .);
}