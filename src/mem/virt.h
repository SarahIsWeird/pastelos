#ifndef VIRT_H
#define VIRT_H

#include <stdint.h>

#include "../multiboot.h"

#define P_PRESENT       0x01
#define P_WRITABLE      0x02
#define P_USER_ACC      0x04
#define P_WRITE_THROUGH 0x08
#define P_CACHE_DISABLE 0x10
#define P_ACCESSED      0x20
#define PT_DIRTY        0x40

struct vmm_ctx_t {
    uint32_t *page_dir;
    uint32_t *page_dir_phys;
    // things like swap stuff go here
};

typedef struct vmm_ctx_t vmm_ctx_t;

void virt_init(mb_info_t *mb_info);
vmm_ctx_t *virt_get_kernel_ctx(); // TODO: remove this

vmm_ctx_t *virt_new_pd();
void virt_enable_ctx(vmm_ctx_t *ctx);
// void virt_map(vmm_ctx_t *ctx, void *vaddr, void *paddr, uint32_t flags);
void virt_map_user(vmm_ctx_t *ctx, void *phys, void *virt, uint32_t size, uint32_t flags);

void virt_map_in_curr(void *phys, void *virt, uint32_t flags);

void *virt_alloc(vmm_ctx_t *ctx);
void *virt_alloc_at(vmm_ctx_t *ctx, void *virt, uint32_t flags);
void *virt_alloc_range(vmm_ctx_t *ctx, uint32_t size);

void *virt_alloc_kernel();
void *virt_alloc_range_kernel(uint32_t size);

#endif
