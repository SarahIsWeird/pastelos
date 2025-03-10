#include "phys.h"

#include "../io/vga.h"
#include "../misc.h"
#include "virt.h"

extern void kernel_start;
extern void kernel_end;

extern void phys_acquire_lock();
extern void phys_release_lock();

static uint32_t mmap[PAGE_ENTRIES];
static uint32_t usable_pages = 0;
static uint32_t free_pages = 0;

static volatile int lock = 0;

static void pretty_print_mmap_entry(const mmap_addr_range_t *entry) {
    uint32_t base_high = entry->base_addr >> 32;
    uint32_t base_low = entry->base_addr & 0xffffffff;
    uint32_t length_high = entry->length >> 32;
    uint32_t length_low = entry->length & 0xffffffff;

    vga_printf("Base %p%p, length %p%p, type %d, entry size %d\n", base_high, base_low, length_high, length_low, entry->type, entry->size);
}

static void set_page_avail(uint32_t addr, int avail) {
    uint32_t page_index = addr / PAGE_SIZE;
    uint32_t mmap_index = page_index / 32;
    uint32_t mask = 1 << (page_index % 32);

    if (avail) {
        if (!(mmap[mmap_index] & mask)) free_pages++;
        mmap[mmap_index] |= mask;
    } else {
        if (mmap[mmap_index] & mask) free_pages--;
        mmap[mmap_index] &= ~mask;
    }
}

static void set_range_avail(uint32_t base, uint32_t limit, int avail) {
    while (base < limit) {
        set_page_avail(base, avail);
        base += PAGE_SIZE;
    }
}

static int is_page_avail(uint32_t addr) {
    uint32_t page_index = addr / PAGE_SIZE;
    uint32_t mmap_index = page_index / 32;
    uint32_t mask = 1 << (page_index % 32);

    return mmap[mmap_index] & mask;
}

// Base and limit have to be page-aligned.
// Limit is exclusive.
static int is_range_avail(uint32_t base, uint32_t limit) {
    for (; base < limit; base += PAGE_SIZE) {
        if (is_page_avail(base)) continue;

        return 0;
    }

    return 1;
}

static uint64_t align_to_page(uint64_t addr, int round_down) {
    // Align to page boundary
    if (round_down) {
        // If a usage page isn't aligned, we round down.
        addr -= addr % 4096;
        return addr;
    }

    // If an unusable page isn't aligned, we have to round up!
    // We could just do the above + 1, but eh, this is more correct :)
    int diff = addr % 4096;
    if (diff != 0) {
        addr += 4096 - diff;
    }

    return addr;
}

static uint32_t round_up_page(uint32_t addr) {
    return align_to_page(addr, 0);
}

void phys_print_avail() {
    for (uint32_t addr = PAGE_SIZE; addr != 0; addr += PAGE_SIZE) {
        if (!is_page_avail(addr)) continue;

        uint32_t start = addr;
        while (is_page_avail(addr)) addr += PAGE_SIZE;
        vga_printf("%p - %p\n", start, addr);
    }
}

void phys_init(const mb_info_t *mb_info) {
    memset(mmap, 0, PAGE_ENTRIES);

    if (!mb_info->flags.mmap) panic("The bootloader didn't provide a memory map!\n");

    uint32_t mmap_size = mb_info->mmap.length;
    mmap_addr_range_t *entry = (void *) mb_info->mmap.addr;
    mmap_addr_range_t *end = (mmap_addr_range_t *) ((void *) entry + mmap_size);
    while (entry < end) {
        pretty_print_mmap_entry(entry);

        uint32_t type = entry->type;
        uint64_t base = align_to_page(entry->base_addr, type != AR_AVAILABLE);
        uint64_t limit = align_to_page(base + entry->length, type == AR_AVAILABLE);

        if (limit > 0xffffffffll) {
            vga_printf("Skipping entry pointing to memory over 4GiB\n");
        } else {
            set_range_avail(base, limit, type == AR_AVAILABLE);
        }

        void *next_entry = (void *) entry + entry->size + sizeof(uint32_t);
        entry = (mmap_addr_range_t *) next_entry;
    }

    usable_pages = free_pages;

    // Make sure page 0 is unavailable, just to make sure NULL doesn't cause weird things
    set_page_avail(0, 0);
    set_range_avail(align_to_page((uint32_t) &kernel_start - 0xc0000000, 1),
                    align_to_page((uint32_t) &kernel_end - 0xc0000000, 0),
                    0);

    if (mb_info->flags.mods && mb_info->mods.count > 0) {
        mod_t *modules = mb_info->mods.addr;
        set_page_avail((uint32_t) modules, 0);
        virt_map_in_curr(modules, modules, 3);

        for (int i = 0; i < mb_info->mods.count; i++) {
            mod_t *module = &modules[i];
            set_range_avail((uint32_t) module->start, (uint32_t) module->end, 0);
        }
    }

    uint32_t kb_usable = usable_pages * PAGE_SIZE / 1024;
    uint32_t kb_free = free_pages * PAGE_SIZE / 1024;
    vga_printf("Max pages: %d, usable: %d KiB, free: %d KiB\n", MAX_PAGES, kb_usable, kb_free);
    phys_print_avail();
}

uint32_t phys_get_usable_pages() {
    return usable_pages;
}

uint32_t phys_get_free_pages() {
    return free_pages;
}

void *phys_alloc() {
    return phys_alloc_range(PAGE_SIZE);
}

void *phys_alloc_range(uint32_t size) {
    if (size == 0) return NULL;
    uint32_t aligned_length = round_up_page(size);

    while (lock == 1) {}
    lock = 1;

    uint64_t addr = 0;
    uint32_t end = 0;
    int found = 0;
    while (addr < 0xffffffff) {
        if (!is_page_avail(addr)) {
            addr += PAGE_SIZE;
            continue;
        }

        end = addr + aligned_length;
        if (!is_range_avail(addr, end)) {
            addr += aligned_length;
            continue;
        }

        found = 1;
        break;
    }

    if (!found) return NULL;

    set_range_avail(addr, end, 0);

    lock = 0;

    // vga_printf("avail: %d KiB\n", usable_pages * PAGE_SIZE / 1024);
    return (void *) addr;
}

void phys_free(void *ptr) {
    phys_free_range(ptr, PAGE_SIZE);
}

void phys_free_range(void *ptr, uint32_t size) {
    set_range_avail((uint32_t) ptr, (uint32_t) ptr + size, 1);
}
