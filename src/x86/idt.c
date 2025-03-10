#include "idt.h"

#include "../io/vga.h"
#include "../misc.h"
#include "../proc/proc.h"
#include "../timer/devices/pit.h"
#include "../timer/timer.h"
#include "../x86/gdt.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xa0
#define PIC2_DATA 0xa1

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01
// There's more ICW stuff, but we don't need any of them.

#define PIC_EOI 0x20

static const char *EXCEPTION_NAMES[32] = {
    "Division error",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device not available",
    "Double Fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPE",
    "Alignment check",
    "Machine check",
    "SIMD FPE",
    "Virtualization exception",
    "Control protection exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor injection exception",
    "VMM Communication exception",
    "Security exception",
    "Reserved",
};

static int_ctx_t *handle_irq(int_ctx_t *ctx) {
    int irq = ctx->int_nr - 0x20;
    if (irq > 7) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);

    if (timer_get_type() == TIMER_PIT && irq == 0) {
        timer_tick();
        ctx = proc_schedule(ctx);
        gdt_set_kernel_stack(ctx + 1);
    } else {
        vga_printf("IRQ %d\n", irq);
    }

    return ctx;
}

extern void setup_idt(); // idt.asm

void idt_load() {
    setup_idt();

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);  // init, ICW4 will be sent
    outb(PIC1_DATA, 0x20);                  // IRQs begin at INT 0x20
    outb(PIC1_DATA, 4);                     // PIC2 is on IRQ2 (0000 0100)
    outb(PIC1_DATA, ICW4_8086);             // 8086 mode instead of 8080 mode
    outb(PIC1_DATA, 0);                     // no IRQ masking

    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);  // same same
    outb(PIC2_DATA, 0x28);                  // IRQs begin at INT 0x28
    outb(PIC2_DATA, 2);                     // PIC1 is on IRQ1 (0000 0010)
    outb(PIC2_DATA, ICW4_8086);             // same same
    outb(PIC2_DATA, 0);                     // same same
}

int_ctx_t *handle_interrupt(int_ctx_t *ctx) {
    if (ctx->int_nr < 0x20) {
        // Could use panic here, but that's a *lot* of varargs.
        vga_set_color(0x0c);
        vga_printf("Exception 0x%2x: %s, error code %x\n", ctx->int_nr, EXCEPTION_NAMES[ctx->int_nr], ctx->err);
        vga_printf("eax %8x | ebx %8x | ecx %8x | edx %8x\n", ctx->eax, ctx->ebx, ctx->ecx, ctx->edx);
        vga_printf("esp %8x | ebp %8x | esi %8x | edi %8x\n", ctx->esp, ctx->ebp, ctx->esi, ctx->edi);
        vga_printf("eip %8x |  cs     %4x | flg %8x\n", ctx->eip, ctx->cs, ctx->eflags);

        if (ctx->int_nr == 0x0e) {
            uint32_t addr;
            asm ("mov %%cr2, %0" : "=r" (addr));
            vga_printf("cr2 %8x\n", addr);
        }

        while (1);
    }

    if (ctx->int_nr < 0x30) {
        return handle_irq(ctx);
    } else if (ctx->int_nr == 0x69) {
        switch (ctx->eax) {
        case 0:
            vga_set_color(ctx->ebx >> 8);
            vga_putc(ctx->ebx & 0xff);
            vga_set_color(0x0f);
        }
    } else {
        vga_printf("Interrupt 0x%2x\n", ctx->int_nr);
    }

    return ctx;
}
