ENTRY(_start)

BASE_ADDRESS        =   0x80200000;

SECTIONS {
    . = BASE_ADDRESS;
    .text : {
        *(.boot)
        *(.text .text.*)
    }

    . = ALIGN(4096);
    rodataStart = .;
    .rodata : {
        *(.rodata .rodata.*)
        rodataEnd = .;
    }

    . = ALIGN(4096);
    dataStart = .;
    .data : {
        *(.data .data.*)
        . = ALIGN(4096);
        _stack = .;
        *(.stack)
        dataEnd = .;
    }
/*
    . = ALIGN(4096);
    PROVIDE(bssStart = .);
    .bss : {
        *(.bss .bss.* .sbss .sbss.*)
        . = ALIGN(4096);
        _pgdir = .;
        *(.pgdir)
        ASSERT(. - _pgdir == 4096, "error: pgdir not one page");
        . = ALIGN(4096);
        PROVIDE(bssEnd = .);
    }
*/


    . = ALIGN(4096);
    PROVIDE(kernelEnd = .);
}