#include "io/vga.h"
#include "mem/phys.h"
#include "mem/virt.h"
#include "misc.h"
#include "multiboot.h"
#include "proc/proc.h"
#include "timer/timer.h"
#include "x86/gdt.h"
#include "x86/idt.h"

extern void enable_interrupts(); // idt.asm

void main(mb_info_t *mb_info, uint32_t mb_checksum) {
    vga_set_color(0x0f);
    vga_clear();

    if (mb_checksum != 0x2badb002) {
        panic("The kernel wasn't loaded by a multiboot-compliant bootloader!\n"
              "Expected a checksum of %8x, but got %8x.\n"
              "Other bootloaders aren't supported yet.\n",
              0x2badb002,
              mb_checksum);
    }

    gdt_load();
    idt_load();
    phys_init(mb_info);
    virt_init(mb_info);
    timer_init(TIMER_PIT, 1);

    proc_load(mb_info);

    vga_printf("Hi :3\n");
    enable_interrupts();

    while (1) {}
}
