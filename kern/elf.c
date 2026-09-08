#include "elf.h"
#include "kern/arch.h"
#include "kern/elf.h"
#include "kern/mem.h"
#include "string.h"
#include "sys/cdefs.h"

PURE static const struct elf_phdr *
phdr (const struct elf_ehdr *e, size_t i)
{
  return (struct elf_phdr *)((uintptr_t)e + e->phoff + i * e->phentsize);
}

static void
map_elf_phdr (uintptr_t root, const struct elf_phdr *p)
{
  for (uintptr_t pg = ALIGN_DOWN (p->vaddr, PAGE_SIZE);
       pg < p->vaddr + p->memsz; pg += PAGE_SIZE)
    {
      constexpr int flags = PTE_PRESENT | PTE_USER | PTE_WRITE;

      add_vm_mapping (root, pg, alloc_page (), flags);
    }
}

void
load_elf (const struct elf_ehdr *e)
{
  const uintptr_t root = get_vm_root ();

  for (size_t i = 0; i < e->phnum; i++)
    {
      const struct elf_phdr *p = phdr (e, i);

      if (p->type != PT_LOAD)
        continue;

      map_elf_phdr (root, p);

      memcpy ((char *)p->vaddr, (char *)e + p->offset, p->filesz);
      memset ((char *)p->vaddr + p->filesz, 0, p->memsz - p->filesz);
    }
}
