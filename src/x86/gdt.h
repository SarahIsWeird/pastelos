#ifndef GDT_H
#define GDT_H

void gdt_load();
void gdt_set_kernel_stack(void *stack);
void gdt_print();

#endif
