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
elf_map (struct elf_phdr *p)
{
  uintptr_t root = get_vm_root ();

  for (size_t i = 0; i < p->memsz; i += PAGE_SIZE)
    {
      int flags = PTE_PRESENT | PTE_USER | PTE_WRITE;

      printf ("Mapping %#lx\n", p->vaddr + i);

      add_vm_mapping (root, p->vaddr + i, alloc_page (), flags);
    }
}

void
elf_load (struct elf_ehdr *e)
{
  for (size_t i = 0; i < n_phdrs (e); i++)
    {
      struct elf_phdr *p = phdr (e, i);

      if (p->type != PT_LOAD)
        continue;

      elf_map (p);

      memcpy ((void *)p->vaddr, (void *)e + p->offset, p->filesz);
      memset ((void *)p->vaddr + p->filesz, 0, p->memsz - p->filesz);
    }
}

uintptr_t
elf_entry (struct elf_ehdr *e)
{
  return e->entry;
}
