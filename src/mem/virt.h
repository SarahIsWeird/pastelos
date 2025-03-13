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

typedef struct vmm_ctx_t vmm_ctx_t;

void virt_init(mb_info_t *mb_info);
vmm_ctx_t *virt_new_ctx();
void virt_destroy_ctx(vmm_ctx_t *ctx, int is_current_ctx);

void virt_unsafe_identity_map(void *addr);
void *virt_temp_map(void *phys);
void virt_remove_temp_map(void *virt);

void virt_use(vmm_ctx_t *ctx);

void virt_mmap_kernel(void *phys, void *virt, uint32_t size);

void *virt_alloc(vmm_ctx_t *ctx);
void *virt_alloc_at(vmm_ctx_t *ctx, void *virt);
void *virt_alloc_kernel();
void *virt_alloc_at_kernel(void *virt);

void virt_free(vmm_ctx_t *ctx, void *virt);
void virt_free_kernel(void *virt);

#endif
