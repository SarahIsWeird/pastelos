#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define NIDENT 16

#define EV_NONE 0
#define EV_CURRENT 1

#define ELF_MAG0 0x7f
#define ELF_MAG1 'E'
#define ELF_MAG2 'L'
#define ELF_MAG3 'F'
#define ELF_MAG ((ELF_MAG3 << 24) | (ELF_MAG2 << 16) | (ELF_MAG1 << 8) | ELF_MAG0)

#define ELF_CLASS_NONE 0
#define ELF_CLASS_32 1
#define ELF_CLASS_64 2

#define ELF_DATA_NONE 0
#define ELF_DATA_LSB 1
#define ELF_DATA_MSB 2

typedef struct __attribute__((packed)) elf_ident_t {
    union {
        struct __attribute__((packed)) {
            uint8_t mag0;
            uint8_t mag1;
            uint8_t mag2;
            uint8_t mag3;
        };
        uint32_t mag;
    };
    uint8_t data_class;
    uint8_t encoding;
    uint8_t version;
    uint8_t padding[NIDENT - 7];
} elf_ident_t;

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7
#define EM_MIPS 8
#define EM_MIPS_RS4_BE 10

typedef struct __attribute__((packed)) elf_header_t {
    elf_ident_t ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t ph_off;
    uint32_t sh_off;
    uint32_t flags;
    uint16_t eh_size;
    uint16_t ph_entry_size;
    uint16_t ph_num;
    uint16_t sh_entry_size;
    uint16_t sh_num;
    uint16_t sh_str_index;
} elf_header_t;

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

typedef struct __attribute__((packed)) elf_section_header_t {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addr_align;
    uint32_t entry_size;
} elf_section_header_t;

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

typedef struct __attribute__((packed)) elf_symbol_table_t {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    union {
        uint8_t info;
        struct {
            uint8_t binding : 4;
            uint8_t type : 4;
        };
    };
    uint8_t other;
    uint16_t sh_index;
} elf_symbol_table_t;

typedef struct __attribute__((packed)) elf_rel_t {
    uint32_t offset;
    union {
        uint32_t info;
        struct {
            uint32_t st_index : 24;
            uint8_t type : 8;
        };
    };
} elf_rel_t;

typedef struct __attribute__((packed)) elf_rela_t {
    elf_rel_t rel;
    int32_t addend;
} elf_rela_t;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

typedef struct __attribute__((packed)) elf_program_header_t {
    uint32_t type;
    uint32_t offset;
    uint32_t v_addr;
    uint32_t p_addr;
    uint32_t file_size;
    uint32_t mem_size;
    uint32_t flags;
    uint32_t align;
} elf_program_header_t;

#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2

#endif
