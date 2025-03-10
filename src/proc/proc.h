#ifndef PROC_H
#define PROC_H

#include "../x86/idt.h"
#include "../multiboot.h"
#include "../mem/virt.h"

typedef struct proc_t {
    uint32_t id;
    int_ctx_t *state;
    void *stack;
    vmm_ctx_t *vmm_ctx;
} proc_t;

void proc_load(mb_info_t *mb_info);
proc_t *proc_new(void *entry);
int_ctx_t *proc_schedule(int_ctx_t *ctx);

#endif
