#ifndef LOADER_H
#define LOADER_H

#include "../elf.h"

void loader_load_elf(elf_header_t *elf, uint32_t size);

#endif
