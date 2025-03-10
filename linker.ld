ENTRY(_start)

SECTIONS {
    . = 1M;

    mb_start = .;

    .multiboot : {
        *(.multiboot)
        *(.entry)
    }

    . += 0xc0000000;

    kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - 0xc0000000) {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xc0000000) {
        *(.rodata)
    }

    . = ALIGN(4K);

    rw_start = .;
    .data ALIGN(4K) : AT(ADDR(.data) - 0xc0000000) {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xc0000000) {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    kernel_end = .;
}
