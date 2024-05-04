#include "elf.h"
#include "stdio.h"
#include "string.h"
#include "sys/arch.h"
#include "sys/cdefs.h"
#include "sys/mem.h"

PURE static size_t
n_phdrs (struct elf_ehdr *e)
{
  return e->phnum;
}

PURE static struct elf_phdr *
phdr (struct elf_ehdr *e, size_t i)
{
  return (struct elf_phdr *)((uintptr_t)e + e->phoff + i * e->phentsize);
}

static void
elf_map (uintptr_t root, struct elf_phdr *p)
{
  for (uintptr_t pg = ALIGN_DOWN (p->vaddr, PAGE_SIZE);
       pg < p->vaddr + p->memsz; pg += PAGE_SIZE)
    {
      int flags = PTE_PRESENT | PTE_USER | PTE_WRITE;

      add_vm_mapping (root, pg, alloc_page (), flags);
    }
}

void
elf_load (struct elf_ehdr *e)
{
  uintptr_t root = get_vm_root ();

  for (size_t i = 0; i < n_phdrs (e); i++)
    {
      struct elf_phdr *p = phdr (e, i);

      if (p->type != PT_LOAD)
        continue;

      elf_map (root, p);

      memcpy ((char *)p->vaddr, (char *)e + p->offset, p->filesz);
      memset ((char *)p->vaddr + p->filesz, 0, p->memsz - p->filesz);
    }
}

uintptr_t
elf_entry (struct elf_ehdr *e)
{
  return e->entry;
}

bool
elf_get_matching_symbol (struct elf_ehdr *e, uintptr_t address, char *name,
                         size_t name_len, ptrdiff_t *offset)
{
  struct elf_shdr *shstrtab
      = (struct elf_shdr *)((uintptr_t)e + e->shoff
                            + e->shentsize * e->shstrndx);
  char *shstrtab_p = (char *)e + shstrtab->offset;

  for (size_t i = 0; i < e->shnum; i++)
    {
      struct elf_shdr *s
          = (struct elf_shdr *)((uintptr_t)e + e->shoff + e->shentsize * i);
      if (s->type != SHT_SYMTAB)
        continue;

      struct elf_shdr *strtab = (struct elf_shdr *)((uintptr_t)e + e->shoff
                                                    + e->shentsize * s->link);
      char *strtab_p = (char *)e + strtab->offset;

      for (size_t j = 0; j < s->size / s->entsize; j++)
        {
          struct elf_sym *sym
              = (struct elf_sym *)((uintptr_t)e + s->offset + j * s->entsize);
          if (sym->value <= address && address < sym->value + sym->size)
            {
              strncpy (name, strtab_p + sym->name, name_len);
              return true;
            }
        }
    }

  return false;
}
