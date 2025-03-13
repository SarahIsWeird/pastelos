#ifndef PROC_H
#define PROC_H

#include "../x86/idt.h"
#include "../multiboot.h"
#include "../mem/virt.h"

typedef struct proc_t proc_t;

void proc_load(mb_info_t *mb_info);
int_ctx_t *proc_get_current();
int_ctx_t *proc_schedule(int_ctx_t *ctx);

proc_t *proc_new(void *entry);
vmm_ctx_t *proc_get_vmm_ctx(proc_t *proc);
void proc_exit_current();

#endif
