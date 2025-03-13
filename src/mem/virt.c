#include "virt.h"

#include <stdint.h>

#include "phys.h"
#include "../misc.h"
#include "../io/vga.h"

#define P_ADDR_MASK     0xfffff000

#define PD_INDEX(addr) (((uint32_t) addr >> 22))
#define PT_INDEX(addr) (((uint32_t) addr >> 12) & 0x3ff)

#define PD_ADDR ((volatile uint32_t *) 0xfffff000)
#define CURR_PD_ADDR ((volatile uint32_t *) 0xfffffffc)
#define PT_ADDR ((volatile uint32_t *) 0xffc00000)

#define PT_MISSING (0x00000000)
#define PD_MISSING (0xffffffff)

#define VIRT_OFFSET 0xc0000000
#define USER_START 0x100000
#define KERNEL_START VIRT_OFFSET
#define USER_END KERNEL_START
#define KERNEL_END ((uint32_t) PT_ADDR)

extern uint32_t init_pd;

static vmm_ctx_t _kernel_ctx;
static vmm_ctx_t *kernel_ctx;

struct vmm_ctx_t {
    uint32_t *page_dir;
    uint32_t *page_dir_phys;
    // things like swap stuff go here
};

static inline void flush_tlb() {
    uint32_t tmp = 0;
    asm volatile ("mov %%cr3, %0;"
                  "mov %0, %%cr3;"
                  : "=r" (tmp)
                  : "r" (tmp)
                  : "memory");
}

static inline void use_pd(uint32_t *pd_phys) {
    asm volatile ("mov %0, %%cr3" :: "r" (pd_phys) : "memory");
}

static inline void invalidate_page(void *addr) {
    asm volatile ("invlpg (%0)" :: "r" (addr) : "memory");
}

static uint32_t get_phys_in_current(uint32_t virt) {
    int pd_index = PD_INDEX(virt);
    int pt_index = PT_INDEX(virt);

    uint32_t pd_entry = PD_ADDR[pd_index];
    if ((pd_entry & P_PRESENT) == 0) return PD_MISSING;

    volatile uint32_t *pt = PT_ADDR + 1024 * pd_index;
    uint32_t pt_entry = pt[pt_index];
    if ((pt_entry & P_PRESENT) == 0) return PT_MISSING;
    return pt_entry & P_ADDR_MASK;
}

static void map_in_current(uint32_t phys, uint32_t virt, uint32_t flags) {
    int pd_index = PD_INDEX(virt);
    int pt_index = PT_INDEX(virt);

    uint32_t pd_entry = PD_ADDR[pd_index];
    if ((pd_entry & P_PRESENT) == 0) {
        // FIXME: is having the page *directory* user accessable fine?
        PD_ADDR[pd_index] = (uint32_t) phys_alloc() | P_USER_ACC | P_WRITABLE | P_PRESENT;
        flush_tlb();
    }

    volatile uint32_t *pt = PT_ADDR + 1024 * pd_index;
    uint32_t pt_entry = pt[pt_index];
    if ((pt_entry & P_PRESENT) != 0 && (flags & P_PRESENT) != 0) {
        vga_printf("virt.c: WARNING: Mapping already present for address %p!\n", virt);
    }

    pt[pt_index] = (phys & P_ADDR_MASK) | (flags & ~P_ADDR_MASK);
    invalidate_page((void *) virt);
}

static uint32_t find_free_in_range(uint32_t from, uint32_t to) {
    for (uint32_t addr = from; addr < to; addr += PAGE_SIZE) {
        uint32_t entry = get_phys_in_current(addr);
        if ((entry == PD_MISSING) || (entry == PT_MISSING)) return addr;
    }

    return 0;
}

void virt_init(mb_info_t *mb_info) {
    // Reuse init_pd from when we first enabled paging, but packaged nicer.
    // We can't alloc memory just yet, so we have _kernel_ctx. This allows
    // us to use kernel_ctx as any other context, no & needed. Makes it a
    // bit easier on the eyes and the brain :)
    kernel_ctx = &_kernel_ctx;
    kernel_ctx->page_dir = &init_pd;
    kernel_ctx->page_dir_phys = (void *) &init_pd - VIRT_OFFSET;

    kernel_ctx->page_dir[0] = 0;
    kernel_ctx->page_dir[0x3ff] = (uint32_t) kernel_ctx->page_dir_phys | P_PRESENT | P_WRITABLE;

    // Prepopulate kernel PTs
    for (int i = 0x300; i < 0x3ff; i++) {
        uint32_t *pd_entry = &kernel_ctx->page_dir[i];
        if (*pd_entry & P_PRESENT) continue;

        *pd_entry = (uint32_t) phys_alloc() | P_PRESENT | P_WRITABLE;
    }

    // address 0 is mapped to this, which overlaps with PT_MISSING.
    virt_remove_temp_map((void *) KERNEL_START);
    virt_mmap_kernel((void *) 0xb8000, (void *) VIRT_OFFSET, PAGE_SIZE);

    if (mb_info->flags.mods && mb_info->mods.count > 0) {
        mod_t *modules = mb_info->mods.addr + VIRT_OFFSET / sizeof(mod_t);
        for (uint32_t i = 0; i < mb_info->mods.count; i++) {
            mod_t *module = &modules[i];
            virt_mmap_kernel(module->start,
                module->start + VIRT_OFFSET,
                module->end - module->start);
        }
    }

    flush_tlb();
}

vmm_ctx_t *virt_new_ctx() {
    vmm_ctx_t *ctx = virt_alloc_kernel();
    ctx->page_dir_phys = phys_alloc();
    ctx->page_dir = virt_temp_map(ctx->page_dir_phys);
    memset(ctx->page_dir, 0, PAGE_SIZE);

    ctx->page_dir[0x3ff] = (uint32_t) ctx->page_dir_phys | P_PRESENT | P_WRITABLE;
    for (uint32_t i = 0x300; i < 0x3ff; i++) {
        ctx->page_dir[i] = kernel_ctx->page_dir[i];
    }

    return ctx;
}

void virt_destroy_ctx(vmm_ctx_t *ctx, int is_current_ctx) {
    if (is_current_ctx) {
        // We need to make sure we don't leave the current PD
        // in some undefined state!
        virt_use(kernel_ctx);
    }

    virt_free_kernel(ctx->page_dir);
    virt_free_kernel(ctx);
}

void virt_unsafe_identity_map(void *addr) {
    map_in_current((uint32_t) addr, (uint32_t) addr, P_PRESENT | P_WRITABLE);
}

void *virt_temp_map(void *addr) {
    uint32_t virt = find_free_in_range(KERNEL_START, KERNEL_END);
    if (virt == 0) return NULL;
    map_in_current((uint32_t) addr, virt, P_PRESENT | P_WRITABLE);
    return (void *) virt;
}

void virt_remove_temp_map(void *virt) {
    int pd_index = PD_INDEX(virt);
    int pt_index = PT_INDEX(virt);

    uint32_t pd_entry = PD_ADDR[pd_index];
    if ((pd_entry & P_PRESENT) == 0) return;

    volatile uint32_t *pt = PT_ADDR + 1024 * pd_index;
    pt[pt_index] = 0;
}

void virt_use(vmm_ctx_t *ctx) {
    use_pd(ctx->page_dir_phys);
}

void virt_mmap_kernel(void *phys, void *virt, uint32_t size) {
    if (phys == NULL) {
        for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
            virt_remove_temp_map(virt + offset);
        }

        return;
    }

    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        map_in_current((uint32_t) phys + offset, (uint32_t) virt + offset, P_PRESENT | P_WRITABLE);
    }
}

void *virt_alloc(vmm_ctx_t *ctx) {
    uint32_t addr = find_free_in_range(USER_START, KERNEL_START);
    if (addr == 0) return 0;
    virt_alloc_at(ctx, (void *) addr);
    return (void *) addr;
}

void *virt_alloc_at(vmm_ctx_t *ctx, void *virt) {
    if ((uint32_t) virt < USER_START) panic("Cannot allocate user memory below 1MiB! (at %p)\n", virt);
    if ((uint32_t) virt >= USER_END) panic("Cannot allocate user memory in kernel region! (at %p)\n", virt);

    void *phys = phys_alloc();
    if (phys == NULL) return NULL;

    uint32_t *current_pd = (uint32_t *) (*CURR_PD_ADDR & P_ADDR_MASK);
    virt_use(ctx);
    map_in_current((uint32_t) phys, (uint32_t) virt, P_PRESENT | P_WRITABLE | P_USER_ACC);
    use_pd(current_pd);
    return phys;
}

void *virt_alloc_kernel() {
    uint32_t addr = find_free_in_range(KERNEL_START, KERNEL_END);
    if (addr == 0) return 0;
    virt_alloc_at_kernel((void *) addr);
    return (void *) addr;
}

void *virt_alloc_at_kernel(void *virt) {
    if ((uint32_t) virt < KERNEL_START) panic("Cannot allocate kernel memory in user region! (at %p)\n", virt);
    if ((uint32_t) virt >= KERNEL_END) panic("Cannot allocate kernel memory in PD map region! (at %p)\n", virt);

    void *phys = phys_alloc();
    if (phys == NULL) return NULL;

    map_in_current((uint32_t) phys, (uint32_t) virt, P_PRESENT | P_WRITABLE);
    return phys;
}

void virt_free(vmm_ctx_t *ctx, void *virt) {
    if ((uint32_t) virt < USER_START) panic("Cannot deallocate user memory below 1MiB! (at %p)\n", virt);
    if ((uint32_t) virt >= USER_END) panic("Cannot deallocate user memory in kernel region! (at %p)\n", virt);

    uint32_t *current_pd = (uint32_t *) (*CURR_PD_ADDR & P_ADDR_MASK);
    virt_use(ctx);

    uint32_t phys = get_phys_in_current((uint32_t) virt);
    if (phys == PT_MISSING || phys == PD_MISSING) {
        use_pd(current_pd);
        vga_printf("WARNING: Tried to free unallocated user memory! (at %p)\n", virt);
        return;
    }

    phys_free((void *) phys);
    map_in_current(0, (uint32_t) virt, 0);

    use_pd(current_pd);
}

void virt_free_kernel(void *virt) {
    if ((uint32_t) virt < KERNEL_START) panic("Cannot deallocate kernel memory in user region! (at %p)\n", virt);
    if ((uint32_t) virt >= KERNEL_END) panic("Cannot deallocate kernel memory in PD map region! (at %p)\n", virt);

    uint32_t phys = get_phys_in_current((uint32_t) virt);
    if (phys == PT_MISSING || phys == PD_MISSING) {
        vga_printf("WARNING: Tried to free unallocated kernel memory! (at %p)\n", virt);
        return;
    }

    phys_free((void *) phys);
    map_in_current(0, (uint32_t) virt, 0);
}
