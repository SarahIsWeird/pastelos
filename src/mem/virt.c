#include "virt.h"

#include <stdint.h>

#include "phys.h"
#include "../misc.h"
#include "../io/vga.h"

#define P_ADDR_MASK     0xfffff000

#define PD_INDEX(addr) (((uint32_t) addr >> 22))
#define PT_INDEX(addr) (((uint32_t) addr >> 12) & 0x3ff)

static vmm_ctx_t _kernel_ctx;
static vmm_ctx_t *kernel_ctx;
static uint32_t kernel_pages;

extern void invalidate_page(void *addr);
extern void enable_paging();
extern void set_active_page_directory(uint32_t *pd);
extern void flush_pd();

extern void kernel_start;
extern void kernel_end;
extern uint32_t init_pd;
extern uint32_t init_pt;

#define PD_ADDR ((volatile uint32_t *) 0xfffff000)
#define PT_ADDR ((volatile uint32_t *) 0xffc00000)

#define PT_MISSING ((void *) 0x00000000)
#define PD_MISSING ((void *) 0xffffffff)

vmm_ctx_t *virt_get_kernel_ctx() {
    return kernel_ctx;
}

void enable_pd(uint32_t *pd) {
    asm volatile ("mov %0, %%cr3" :: "r" (pd) : "memory");
}

void clear_tlb() {
    asm volatile (  "mov %%cr3, %%eax;"
                    "mov %%eax, %%cr3"
                    ::: "memory");
}

/*
 * Retrieves the physical address of the specified virtual address.
 * `P_PRESENT` is ignored.
 * @returns `null` if no mapping exists for the address.
 */
static volatile void *get_physical_addr(void *virt) {
    uint32_t pd_index = PD_INDEX(virt);
    uint32_t pt_index = PT_INDEX(virt);

    uint32_t pd_entry = PD_ADDR[pd_index];
    if (!(pd_entry & P_PRESENT)) return PD_MISSING;

    void *pt = (void *) PT_ADDR + (PAGE_SIZE * pd_index);
    uint32_t pt_entry = ((uint32_t *) pt)[pt_index];
    if (!(pt_entry & P_PRESENT)) return PT_MISSING;

    return (void *) (pt_entry & P_ADDR_MASK);
}

static void copy_kernel_pd(uint32_t *pd) {
    //vga_printf("copy_kernel_pd: %p %p\n", pd, PD_ADDR);
    memcpy(pd, PD_ADDR, PAGE_SIZE - 4);
}

static void *find_free_range(uint32_t size, uint32_t mr_start, uint32_t mr_end) {
    for (uint32_t addr = mr_start; addr < mr_end; addr += PAGE_SIZE) {
        void *phys_addr = get_physical_addr((void *) addr);
        if ((phys_addr != PD_MISSING) && (phys_addr != PT_MISSING)) continue;

        uint32_t start = addr;
        for (; addr < mr_end; addr += PAGE_SIZE) {
            void *curr_phys = get_physical_addr((void *) addr);
            if ((curr_phys != PD_MISSING) && (curr_phys != PT_MISSING)) break;

            if (addr < ((uint32_t) start + size)) continue;
            return (void *) start;
        }
    }
    
    return NULL;
}

void virt_map_in_curr(void *phys, void *virt, uint32_t flags) {
    uint32_t pd_index = PD_INDEX(virt);
    uint32_t pt_index = PT_INDEX(virt);

    uint32_t pd_entry = PD_ADDR[pd_index];
    if (!(pd_entry & P_PRESENT)) {
        PD_ADDR[pd_index] = (uint32_t) phys_alloc() | (flags & 0xfff);
        clear_tlb();
    }

    uint32_t *pt = PT_ADDR + 0x400 * pd_index;
    pt[pt_index] = ((uint32_t) phys & P_ADDR_MASK) | (flags & 0xfff);
    asm volatile ("invlpg (%0)" :: "r" (virt) : "memory");
}

/*
 * Maps the specified physical address range at some point between `mr_start` (inclusive) and `mr_end` (exclusive).
 * `mr_start` must be page-aligned, `mr_end` doesn't have to be (i.e., 0xffffffff means search until end of memory).
 * `flags` specifies the page structure flags that should be set, must be `P_PRESENT` at minimum.
 */
static void *map_anywhere_between(vmm_ctx_t *ctx, void *phys, uint32_t size, uint32_t flags, uint32_t mr_start, uint32_t mr_end) {
    uint32_t *current_pd = get_physical_addr((void *) 0xfffffffc);
    if (current_pd == PT_MISSING || current_pd == PD_MISSING)
        panic("virt.c: map_anywhere_between: Couldn't get current page directory!\n");
    
    virt_enable_ctx(ctx);

    void *base = find_free_range(size, mr_start, mr_end);
    if (base == NULL)
        panic("virt.c: map_anywhere_between: couldn't find %d bytes of free space between %p and %p!\n", size, mr_start, mr_end);
    
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        virt_map_in_curr(phys + offset, base + offset, flags);
    }

    // copy_kernel_pd(current_pd);
    enable_pd(current_pd);
    memcpy(PD_ADDR, ctx->page_dir, 4092);
    
    return base;
}

static void *map_for_kernel(void *phys, uint32_t size, uint32_t flags) {
    void *base = find_free_range(size, 0xc0000000, 0xffffffff);
    if (base == NULL)
        panic("virt.c: map_anywhere_between: couldn't find %d bytes of free space in kernel space!\n", size);
    
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        virt_map_in_curr(phys + offset, base + offset, flags);
    }

    return base;
}

void virt_init(mb_info_t *mb_info) {
    // Reuse init_pd from when we first enabled paging, but packaged nicer.
    // We can't alloc memory just yet, so we have _kernel_ctx. This allows
    // us to use kernel_ctx as any other context, no & needed. Makes it a
    // bit easier on the eyes :)
    kernel_ctx = &_kernel_ctx;
    kernel_ctx->page_dir = &init_pd;
    kernel_ctx->page_dir_phys = (void *) &init_pd - 0xc0000000;

    uint32_t pd_index_offset = 0;
    if (PD_INDEX(&kernel_start) == PD_INDEX(&kernel_end)) pd_index_offset++;
    for (uint32_t pd_index = PD_INDEX(&kernel_end) + pd_index_offset; pd_index < PD_INDEX(PD_ADDR); pd_index++) {
        uint32_t entry = (uint32_t) phys_alloc() | P_PRESENT | P_WRITABLE;
        kernel_ctx->page_dir[pd_index] = entry;
    }

    kernel_ctx->page_dir[0] = 0;
    kernel_ctx->page_dir[1023] = (uint32_t) kernel_ctx->page_dir_phys | P_PRESENT | P_WRITABLE;

    enable_pd(kernel_ctx->page_dir_phys);
}

vmm_ctx_t *virt_new_pd() {
    vmm_ctx_t *ctx = virt_alloc_kernel();
    memset(ctx, 0, PAGE_SIZE);

    ctx->page_dir_phys = phys_alloc();
    ctx->page_dir = map_for_kernel(ctx->page_dir_phys, PAGE_SIZE, P_PRESENT | P_WRITABLE);
    memset(ctx->page_dir, 0, PAGE_SIZE);
    vga_printf("new pd: %p %p\n", ctx->page_dir, ctx->page_dir_phys);

    copy_kernel_pd(ctx->page_dir);
    ctx->page_dir[0x3ff] = (uint32_t) ctx->page_dir_phys | P_PRESENT | P_WRITABLE;
    return ctx;
}

void virt_enable_ctx(vmm_ctx_t *ctx) {
    vga_printf("%p %p %p\n", ctx, ctx->page_dir, ctx->page_dir_phys);
    copy_kernel_pd(ctx->page_dir);
    asm volatile ("mov %0, %%cr3" :: "r" (ctx->page_dir_phys) : "memory");
    // enable_pd(ctx->page_dir_phys);
}

void virt_map_user(vmm_ctx_t *ctx, void *phys, void *virt, uint32_t size, uint32_t flags) {
    uint32_t *current_pd = get_physical_addr((void*) 0xfffffffc);
    if (current_pd == PT_MISSING || current_pd == PD_MISSING)
        panic("virt.c: map_anywhere_between: Couldn't get current page directory!\n");
    
    virt_enable_ctx(ctx);

    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        virt_map_in_curr(phys + offset, virt + offset, flags);
    }

    enable_pd(current_pd);
}

void *virt_alloc(vmm_ctx_t *ctx) {
    return virt_alloc_range(ctx, PAGE_SIZE);
}

void *virt_alloc_kernel() {
    return virt_alloc_range_kernel(PAGE_SIZE);
}

void *virt_alloc_at(vmm_ctx_t *ctx, void *virt, uint32_t flags) {
    void *phys = phys_alloc();
    if (phys == NULL) return NULL;

    virt_map_user(ctx, phys, virt, PAGE_SIZE, flags);
    return phys;
}

void *virt_alloc_range(vmm_ctx_t *ctx, uint32_t size) {
    void *phys = phys_alloc_range(size);
    if (phys == NULL) return NULL;

    void *virt = map_anywhere_between(ctx, phys, size, P_PRESENT | P_WRITABLE | P_USER_ACC, 0x100000, 0xc0000000);
    if (virt == NULL) {
        // We panic for now, but before I forget to add this later,
        // I'm gonna just put it here already.
        phys_free_range(phys, size);
    }

    return virt;
}

void *virt_alloc_range_kernel(uint32_t size) {
    void *phys = phys_alloc_range(size);
    if (phys == NULL) return NULL;

    void *virt = map_for_kernel(phys, size, P_PRESENT | P_WRITABLE);
    if (virt == NULL) {
        // We panic for now, but before I forget to add this later,
        // I'm gonna just put it here already.
        phys_free_range(phys, size);
    }

    return virt;
}
