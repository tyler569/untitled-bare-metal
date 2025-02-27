#pragma once
#include "../elf.h"

void elf_load (struct elf_ehdr *e);
bool elf_get_matching_symbol (struct elf_ehdr *e, uintptr_t address,
                              char *name, size_t name_len, ptrdiff_t *offset);
