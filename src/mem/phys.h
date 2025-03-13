#ifndef PHYS_H
#define PHYS_H

#include "../multiboot.h"


#define PAGE_SIZE 4096
#define PAGES_PER_GiB (1024 * 1024 * 1024 / PAGE_SIZE)
#define MAX_PAGES (4 * PAGES_PER_GiB)
#define PAGE_ENTRIES (MAX_PAGES / 32)

void phys_init(const mb_info_t *mb_info);

uint32_t phys_get_usable_pages();
uint32_t phys_get_free_pages();

void *phys_alloc();
void *phys_alloc_range(uint32_t size);
void phys_free(void *ptr);
void phys_free_range(void *ptr, uint32_t size);

#endif
