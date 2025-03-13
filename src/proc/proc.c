#include "proc.h"

#include "../io/vga.h"
#include "../misc.h"
#include "../timer/timer.h"
#include "loader.h"
#include "../x86/gdt.h"

#define MAX_PROCS 3

struct proc_t {
    uint32_t id;
    int_ctx_t *state;
    void *stack;
    void *user_stack;
    vmm_ctx_t *vmm_ctx;
    proc_t *prev;
    proc_t *next;
};

typedef struct __attribute__((packed)) dangling_stack_t {
    uint32_t stack_data[1023];
    struct dangling_stack_t *next;
} dangling_stack_t;

static proc_t *first_proc = NULL;
static proc_t *curr_proc = NULL;

static uint32_t proc_i = 0;
static volatile int sched_timer = -1;
static volatile int is_modifying_procs = 0;
static dangling_stack_t *dangling_stacks = NULL;
static int is_first_schedule = 1;

void proc_load(mb_info_t *mb_info) {
    vga_printf("mb struct is at %p\n", mb_info);
    if (mb_info->flags.mods && mb_info->mods.count > 0) {
        mod_t *modules = mb_info->mods.addr + 0xc0000000 / sizeof(mod_t);

        vga_printf("module count %d\n", mb_info->mods.count);
        for (uint32_t i = 0; i < mb_info->mods.count; i++) {
            mod_t *module = &modules[i];
            vga_printf("Loading module %s\n", module->string ? module->string + 0xc0000000 : "<no name>");

            loader_load_elf(module->start + 0xc0000000, module->end - module->start);
        }
    }
}

int_ctx_t *proc_get_current() {
    if (curr_proc == NULL) panic("proc.c: proc_get_current called when no processes are active!");
    return curr_proc->state;
}

proc_t *proc_new(void *entry) {
    if (proc_i >= MAX_PROCS) panic("proc.c: max process count reached!\n");

    proc_t *proc = virt_alloc_kernel();
    memset(proc, 0, 4096);
    proc->id = proc_i;
    proc->stack = virt_alloc_kernel();
    memset(proc->stack, 0, 4096);
    proc->vmm_ctx = virt_new_ctx();

    proc->state = (int_ctx_t *) (proc->stack + 4096 - sizeof(int_ctx_t));
    proc->user_stack = virt_alloc(proc->vmm_ctx);
    *proc->state = (int_ctx_t) {
        .edi = 0,
        .esi = 0,
        .ebp = (uint32_t) proc->user_stack + 4096,
        .esp = (uint32_t) proc->user_stack + 4096,
        .ebx = 0,
        .edx = 0,
        .ecx = 0,
        .eax = 0,
        .int_nr = 0,
        .err = 0,
        .eip = (uint32_t) entry,
        .cs = 0x1b,
        .eflags = 0x202,
        .esp2 = (uint32_t) proc->user_stack + 4096,
        .ss = 0x23,
    };

    while (is_modifying_procs) {}
    is_modifying_procs = 1;

    if (first_proc == NULL) {
        first_proc = proc;
        curr_proc = proc;
    }

    curr_proc->next = proc;
    proc->next = first_proc;

    proc->prev = curr_proc;
    first_proc->prev = proc;

    proc_i++;

    is_modifying_procs = 0;
    return proc;
}

vmm_ctx_t *proc_get_vmm_ctx(proc_t *proc) {
    return proc->vmm_ctx;
}

int_ctx_t *proc_schedule(int_ctx_t *ctx) {
    if (is_modifying_procs) return ctx;

    if (curr_proc == NULL) {
        // No processes were added yet
        vga_printf("No procs!\n");
        while (1);
        return ctx;
    } else if (sched_timer > -1 && !timer_oneshot_is_done(sched_timer)) {
        // Time isn't up for this process
        return ctx;
    } else if (!is_first_schedule) {
        curr_proc->state = ctx;
    } else {
        is_first_schedule = 0;
    }

    sched_timer = timer_new_oneshot(10);

    curr_proc = curr_proc->next;

    virt_use(curr_proc->vmm_ctx);
    gdt_set_kernel_stack(curr_proc->state + 1);

    return curr_proc->state;
}

void proc_exit_current() {
    while (is_modifying_procs) {}
    is_modifying_procs = 1;

    proc_t *curr = curr_proc;
    proc_t *next = curr->next;
    proc_t *prev = curr->prev;

    if (next == curr) {
        curr_proc = NULL;
    } else {
        next->prev = prev;
        prev->next = next;
        curr_proc = next;
    }

    virt_free(curr->vmm_ctx, curr->user_stack);
    dangling_stack_t *dangling_stack = curr->stack;
    dangling_stack->next = dangling_stacks == NULL ? NULL : dangling_stacks;
    dangling_stacks = dangling_stack;

    virt_destroy_ctx(curr->vmm_ctx, 1);
    virt_free_kernel(curr);

    is_first_schedule = 1;

    is_modifying_procs = 0;
    
    asm volatile ("sti");

    while(1);
}
