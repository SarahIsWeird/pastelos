#include "loader.h"

#include "../io/vga.h"
#include "../misc.h"
#include "proc.h"
#include "../mem/virt.h"

// FIXME: don't panic here
static void validate_elf_ident(elf_ident_t *ident) {
    if (ident->mag != ELF_MAG) panic("Expected ELF magic %p, but got %p!\n", ELF_MAG, ident->mag);
    if (ident->data_class != ELF_CLASS_32) panic("Expected 32 bit ELF object!\n");
    if (ident->encoding != ELF_DATA_LSB) panic("Expected LSB encoding! (This is the Intel x86 standard)\n");
}

static void validate_elf(elf_header_t *elf) {
    validate_elf_ident(&elf->ident);

    if (elf->type != ET_EXEC) panic("Can only load executable files right now! (got type %d)\n", elf->type);
    if (elf->machine != EM_386) panic("Can only load ELF files for the i386 architecture! (got arch %d)\n", elf->machine);
    if (elf->version != elf->ident.version) panic("Expected ELF version and ident version to be equal!\n");
}

static proc_t *last = (void *) 0;

void loader_load_elf(elf_header_t *elf, uint32_t size) {
    for (uint32_t i = 0; i < size; i += 4096) {
        virt_map_in_curr((void *) elf - 0xc0000000 + i, (void *) elf + i, P_PRESENT | P_WRITABLE | P_USER_ACC);
    }

    validate_elf(elf);
    elf_program_header_t *ph = (elf_program_header_t *) ((void *) elf + elf->ph_off);

    proc_t *proc = proc_new((void*) elf->entry);
    vga_printf("%p ", proc);
    vga_printf("proc: %p %p\n", proc->vmm_ctx->page_dir, proc->vmm_ctx->page_dir_phys);

    vga_printf("SDFASEDDFASDFASDFDFASFDSA %p %p\n", &last, last);
    if (last) {
        vga_printf("WTF");
        vga_printf("last: %p %p\n", last->vmm_ctx->page_dir, last->vmm_ctx->page_dir_phys);
    } else {
        last = proc;
        vga_printf("now it's %p\n", last);
    }


    // create new context

    for (uint32_t i = 0; i < elf->ph_num; i++, ph++) {
        if (ph->type != PT_LOAD) {
            vga_printf("skipping %d\n", ph->type);
            continue;
        }

        if (ph->file_size > ph->mem_size) {
            panic("File size was bigger than mem size!\n");
        }

        vga_printf("paddr %p, vaddr %p\n", ph->p_addr, ph->v_addr);

        for (uint32_t i = ph->file_size; i < ph->mem_size; i += 4096) {
            void *mem = virt_alloc_at(proc->vmm_ctx, ph->v_addr + (void *) i, P_PRESENT | P_WRITABLE | P_USER_ACC);

            virt_map_in_curr(mem, mem, P_PRESENT | P_WRITABLE);
            memset(mem, 0, 4096);
            virt_map_in_curr(0, mem, 0);
        }

        for (uint32_t i = 0; i < ph->file_size; i += 4096) {
            virt_alloc_at(proc->vmm_ctx, (void *) ph->v_addr + i, P_PRESENT | P_USER_ACC | P_WRITABLE);
            virt_enable_ctx(proc->vmm_ctx);
            memcpy((void *) ph->v_addr + i, (void *) elf + ph->offset + i, 4096);
            virt_enable_ctx(virt_get_kernel_ctx());
        }

        // vga_printf("%p: ", *(uint32_t *) ((void *) elf + ph->offset));
        // virt_map_user(proc->vmm_ctx, (void *) elf - 0xc0000000 + ph->offset, (void *) ph->v_addr, ph->file_size, P_PRESENT | P_WRITABLE | P_USER_ACC);
        // vga_printf("\n");

        // don't copy anymore, but map to vaddr for the new context
        // void *dst = (void *) ph->v_addr;
        // if (ph->mem_size > ph->file_size) {
        //     memset((void *) ph->v_addr + ph->file_size, 0, ph->mem_size - ph->file_size);
        // }

        // memcpy(dst, (void*) elf + ph->offset, ph->file_size);
    }

}
