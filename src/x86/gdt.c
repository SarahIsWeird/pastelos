#include "gdt.h"

#include <stdint.h>

#include "../io/vga.h"

#define GDT_SIZE 6

#define ACC_PRESENT     0x80
#define ACC_DPL_0       0x00
#define ACC_DPL_3       0x60
#define ACC_TSS_32BIT   0x09
#define ACC_CODE_SEG    0x18
#define ACC_DATA_SEG    0x10
#define ACC_CONFORMING  0x04
#define ACC_READABLE    0x02
#define ACC_WRITABLE    0x02
#define ACC_ACCESSED    0x01

#define FLG_PAGE_GRAN   0x8
#define FLG_SIZE_32BIT  0x4
#define FLG_LONG_MODE   0x2

typedef struct __attribute__((packed)) gdt_entry {
    uint16_t limit;
    uint32_t base : 24;
    uint8_t access;
    uint8_t limit2 : 4;
    uint8_t flags : 4;
    uint8_t base2;
} gdt_entry;

static gdt_entry gdt[GDT_SIZE];
static uint32_t tss[32] = { 0 };

extern void set_gdtr(gdt_entry *gdt, uint16_t size);
extern void set_tr(uint32_t *tss, uint16_t selector);

void set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_entry *entry = &gdt[i];

    entry->base = base & 0xffffff;
    entry->base2 = (base >> 24) & 0xff;

    entry->limit = limit & 0xffff;
    entry->limit2 = (limit >> 16) & 0xf;

    entry->access = access;
    entry->flags = flags & 0xf;
}

void gdt_load() {
    set_entry(0, 0, 0, 0, 0);

    set_entry(1, 0, 0xfffff, ACC_PRESENT | ACC_DPL_0 | ACC_CODE_SEG | ACC_READABLE, FLG_PAGE_GRAN | FLG_SIZE_32BIT);
    set_entry(2, 0, 0xfffff, ACC_PRESENT | ACC_DPL_0 | ACC_DATA_SEG | ACC_WRITABLE, FLG_PAGE_GRAN | FLG_SIZE_32BIT);
    set_entry(3, 0, 0xfffff, ACC_PRESENT | ACC_DPL_3 | ACC_CODE_SEG | ACC_READABLE, FLG_PAGE_GRAN | FLG_SIZE_32BIT);
    set_entry(4, 0, 0xfffff, ACC_PRESENT | ACC_DPL_3 | ACC_DATA_SEG | ACC_WRITABLE, FLG_PAGE_GRAN | FLG_SIZE_32BIT);

    set_entry(5, (uint32_t) tss, sizeof(tss), ACC_PRESENT | ACC_DPL_0 | ACC_TSS_32BIT, 0);

    set_gdtr(gdt, sizeof(gdt_entry) * GDT_SIZE - 1);
    set_tr(tss, 0x28);
}

void gdt_set_kernel_stack(void *stack) {
    tss[1] = (uint32_t) stack;
}

void gdt_print() {
    for (int i = 0; i < GDT_SIZE; i++) {
        gdt_entry entry = gdt[i];
        vga_printf("%d: %2x%6x %x%4x %2x %x\n", i, entry.base2, entry.base, entry.limit2, entry.limit, entry.access, entry.flags);
    }

    vga_printf("gdt address: %p\n", gdt);
}
