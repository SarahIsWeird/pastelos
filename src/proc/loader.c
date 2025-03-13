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

void loader_load_elf(elf_header_t *elf, uint32_t size) {
    vga_printf("elf %p\n", elf);

    validate_elf(elf);
    elf_program_header_t *ph = (elf_program_header_t *) ((void *) elf + elf->ph_off);

    proc_t *proc = proc_new((void*) elf->entry);
    vmm_ctx_t *vctx = proc_get_vmm_ctx(proc);

    for (uint32_t i = 0; i < elf->ph_num; i++, ph++) {
        if (ph->type != PT_LOAD) {
            vga_printf("skipping %d\n", ph->type);
            continue;
        }

        if (ph->file_size > ph->mem_size) {
            panic("File size was bigger than mem size!\n");
        }

        if (ph->file_size > size) {
            panic("File size was bigger than the provided ELF binary!\n");
        }

        vga_printf("paddr %p, vaddr %p\n", ph->p_addr, ph->v_addr);

        for (uint32_t i = ph->file_size; i < ph->mem_size; i += 4096) {
            void *mem = virt_alloc_at(vctx, ph->v_addr + (void *) i);

            void *v_mem = virt_temp_map(mem);
            memset(v_mem, 0, 4096);
            virt_remove_temp_map(v_mem);
        }

        for (uint32_t i = 0; i < ph->file_size; i += 4096) {
            void *phys = virt_alloc_at(vctx, (void *) ph->v_addr + i);
            void *tmp = virt_temp_map(phys);
            memcpy(tmp, (void *) elf + ph->offset + i, 4096);
            virt_remove_temp_map(tmp);
        }
    }
}
