#include "proc.h"

#include "../io/vga.h"
#include "../misc.h"
#include "../timer/timer.h"
#include "loader.h"

#define MAX_PROCS 3

static proc_t processes[MAX_PROCS];
static uint32_t proc_i = 0;
static volatile int sched_timer = -1;
static volatile int sched_i = -1;

void proc_load(mb_info_t *mb_info) {
    vga_printf("mb struct is at %p\n", mb_info);
    if (mb_info->flags.mods && mb_info->mods.count > 0) {
        mod_t *modules = mb_info->mods.addr + 0xc0000000 / sizeof(mod_t);

        vga_printf("module count %d\n", mb_info->mods.count);
        for (int i = 0; i < mb_info->mods.count; i++) {
            mod_t *module = &modules[i] + 0xc0000000;
            vga_printf("Loading module %s\n", module->string ? module->string + 0xc0000000 : "<no name>");

            loader_load_elf(module->start + 0xc0000000, module->end - module->start);
        }
    }
}

proc_t *proc_new(void *entry) {
    if (proc_i >= MAX_PROCS) panic("proc.c: max process count reached!\n");

    proc_t *proc = &processes[proc_i];
    memset(proc, 0, 4096);
    proc->id = proc_i;
    proc->stack = virt_alloc_kernel();
    proc->vmm_ctx = virt_new_pd();

    proc->state = (int_ctx_t *) (proc->stack + 4096 - sizeof(int_ctx_t));
    uint32_t user_stack = (uint32_t) virt_alloc(proc->vmm_ctx) + 4096;
    *proc->state = (int_ctx_t) {
        .edi = 0,
        .esi = 0,
        .ebp = user_stack,
        .esp = user_stack,
        .ebx = 0,
        .edx = 0,
        .ecx = 0,
        .eax = 0,
        .int_nr = 0,
        .err = 0,
        .eip = (uint32_t) entry,
        .cs = 0x1b,
        .eflags = 0x202,
        .esp2 = user_stack,
        .ss = 0x23,
    };

    proc_i++;
    return proc;
}

int_ctx_t *proc_schedule(int_ctx_t *ctx) {
    if (proc_i == 0) {
        // No processes were added yet
        vga_printf("No procs!\n");
        while (1);
        return ctx;
    } else if (sched_timer > -1 && !timer_oneshot_is_done(sched_timer)) {
        // Time isn't up for this process
        return ctx;
    } else if (sched_i > -1) {
        processes[sched_i].state = ctx;
    }

    sched_timer = timer_new_oneshot(10);

    // sched_i++;
    sched_i = 0;
    sched_i %= proc_i;

    proc_t *proc = &processes[sched_i];

    vga_printf("%p %p\n", proc, proc->vmm_ctx);
    virt_enable_ctx(proc->vmm_ctx);
    // vga_printf("trying to access eip \n", processes[sched_i].state->eip);
    // vga_printf("*eip: %p\n", *(uint32_t*)(processes[sched_i].state->eip));
    // virt_map_user(processes[sched_i].vmm_ctx, (void *) 0x197000, (void *) 0x200000, 4096, P_PRESENT | P_USER_ACC);

    return proc->state;
}
