#pragma once

#include "stdint.h"
#include "sys/cdefs.h"

typedef uint16_t elf_half;
typedef uint32_t elf_word;
typedef uint64_t elf_xword;
typedef int32_t elf_sword;
typedef int64_t elf_sxword;

typedef uint64_t elf_addr;
typedef uint64_t elf_off;
typedef uint16_t elf_section;
typedef uint16_t elf_versym;
typedef uint64_t elf_relr;

#define EI_NIDENT 16
#define ELFMAGIC "\x{7f}ELF"
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFVERSION_CURRENT 1
#define ELFOSABI_SYSV 0
#define ELFOSABI_GNU 3

#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3

#define EM_X86_64 62

struct elf_ehdr
{
  unsigned char ident[EI_NIDENT];
  elf_half type;
  elf_half machine;
  elf_word version;
  elf_addr entry;
  elf_off phoff;
  elf_off shoff;
  elf_word flags;
  elf_half ehsize;
  elf_half phentsize;
  elf_half phnum;
  elf_half shentsize;
  elf_half shnum;
  elf_half shstrndx;
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3

#define PF_X 1
#define PF_W 2
#define PF_R 4

struct elf_phdr
{
  elf_word type;
  elf_word flags;
  elf_off offset;
  elf_addr vaddr;
  elf_addr paddr;
  elf_xword filesz;
  elf_xword memsz;
  elf_xword align;
};

struct elf_shdr
{
  elf_word name;
  elf_word type;
  elf_xword flags;
  elf_addr addr;
  elf_off offset;
  elf_xword size;
  elf_word link;
  elf_word info;
  elf_xword addralign;
  elf_xword entsize;
};

struct elf_sym
{
  elf_word name;
  unsigned char info;
  unsigned char other;
  elf_half shndx;
  elf_addr value;
  elf_xword size;
};

struct elf_rel
{
  elf_addr offset;
  elf_xword info;
};

struct elf_rela
{
  elf_addr offset;
  elf_xword info;
  elf_sxword addend;
};

struct elf_dyn
{
  elf_sxword tag;
  elf_xword val;
};

void elf_load (struct elf_ehdr *e);
uintptr_t elf_entry (struct elf_ehdr *e);
