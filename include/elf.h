#pragma once

#include "stddef.h"
#include "stdint.h"
#include "string.h"

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

constexpr size_t EI_NIDENT = 16;
#define ELFMAGIC "\177ELF"
constexpr uint8_t ELFCLASS64 = 2;
constexpr uint8_t ELFDATA2LSB = 1;
constexpr elf_word ELFVERSION_CURRENT = 1;
constexpr uint8_t ELFOSABI_SYSV = 0;
constexpr uint8_t ELFOSABI_GNU = 3;

constexpr elf_half ET_REL = 1;
constexpr elf_half ET_EXEC = 2;
constexpr elf_half ET_DYN = 3;

constexpr elf_half EM_X86_64 = 62;

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

constexpr elf_word PT_NULL = 0;
constexpr elf_word PT_LOAD = 1;
constexpr elf_word PT_DYNAMIC = 2;
constexpr elf_word PT_INTERP = 3;

constexpr elf_word PF_X = 1;
constexpr elf_word PF_W = 2;
constexpr elf_word PF_R = 4;

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

constexpr elf_word SHT_NULL = 0;
constexpr elf_word SHT_PROGBITS = 1;
constexpr elf_word SHT_SYMTAB = 2;
constexpr elf_word SHT_STRTAB = 3;
constexpr elf_word SHT_RELA = 4;
constexpr elf_word SHT_HASH = 5;
constexpr elf_word SHT_DYNAMIC = 6;
constexpr elf_word SHT_NOTE = 7;
constexpr elf_word SHT_NOBITS = 8;
constexpr elf_word SHT_REL = 9;
constexpr elf_word SHT_SHLIB = 10;
constexpr elf_word SHT_DYNSYM = 11;

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

static inline bool
is_elf (struct elf_ehdr *e)
{
  return memcmp (e->ident, ELFMAGIC, 4) == 0;
}
