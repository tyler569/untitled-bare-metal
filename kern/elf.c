#include "elf.h"
#include "kern/arch.h"
#include "kern/elf.h"
#include "kern/mem.h"
#include "stdio.h"
#include "string.h"
#include "sys/cdefs.h"

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

  for (size_t i = 0; i < e->phnum; i++)
    {
      struct elf_phdr *p = phdr (e, i);

      if (p->type != PT_LOAD)
        continue;

      elf_map (root, p);

      memcpy ((char *)p->vaddr, (char *)e + p->offset, p->filesz);
      memset ((char *)p->vaddr + p->filesz, 0, p->memsz - p->filesz);
    }
}
