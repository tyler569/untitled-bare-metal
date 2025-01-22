#include "elf.h"
#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#include "./lib.h"

static inline struct elf_phdr *get_phdr (struct elf_ehdr *, size_t);
cptr_t allocate_pages (cptr_t untyped, size_t);

int
create_process (void *elf_data, size_t elf_size, cptr_t untyped, cptr_t *tcb, cptr_t *cspace)
{
  (void)elf_size;
  (void)untyped;
  (void)tcb;
  (void)cspace;

  struct elf_ehdr *ehdr = (struct elf_ehdr *)elf_data;
  // if (!is_valid_elf (ehdr))
  //   return -1;

  size_t needed_pages = 0;
  uintptr_t lowest_addr = 0;
  uintptr_t highest_addr = 0;

  for (int i = 0; i < ehdr->phnum; i++)
    {
      struct elf_phdr *phdr = get_phdr (ehdr, i);
      if (phdr->type == PT_LOAD)
        {
          uintptr_t start = phdr->vaddr;
          uintptr_t end = start + phdr->memsz;

          uintptr_t fstart = phdr->offset;
          uintptr_t fend = fstart + phdr->filesz;

          printf ("want to load %010lx-%010lx to %016lx-%016lx\n", fstart,
                  fend, start, end);

          if (lowest_addr == 0 || start < lowest_addr)
            lowest_addr = start;
          if (end > highest_addr)
            highest_addr = end;
          needed_pages += (end - start + 0xfff) / 0x1000;
        }
    }

  size_t needed_page_tables = (highest_addr + 0x1ff) / 0x200000;

  printf ("lowest_addr: %#lx\n", lowest_addr);
  printf ("highest_addr: %#lx\n", highest_addr);
  printf ("needed_pages: %zu\n", needed_pages);
  printf ("needed_page_tables: %zu\n", needed_page_tables);

  return 0;
}

static inline struct elf_phdr *
get_phdr (struct elf_ehdr *ehdr, size_t i)
{
  return (struct elf_phdr *)((char *)ehdr + ehdr->phoff + i * ehdr->phentsize);
}

cptr_t
allocate_pages (cptr_t untyped, size_t n)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, cap_x86_64_page, 12, init_cap_root_cnode, init_cap_root_cnode, 64, cptr, n);
  return cptr;
}

cptr_t
allocate_pml4 (cptr_t untyped)
{
  cptr_t cptr = cptr_alloc_range (1);
  untyped_retype (untyped, cap_x86_64_pml4, 0, init_cap_root_cnode, init_cap_root_cnode, 64, cptr, 1);
  return cptr;
}

cptr_t
allocate_pdpt (cptr_t untyped)
{
  cptr_t cptr = cptr_alloc_range (1);
  untyped_retype (untyped, cap_x86_64_pdpt, 0, init_cap_root_cnode, init_cap_root_cnode, 64, cptr, 1);
  return cptr;
}

cptr_t
allocate_pd (cptr_t untyped)
{
  cptr_t cptr = cptr_alloc_range (1);
  untyped_retype (untyped, cap_x86_64_pd, 0, init_cap_root_cnode, init_cap_root_cnode, 64, cptr, 1);
  return cptr;
}

cptr_t
allocate_pt (cptr_t untyped)
{
  cptr_t cptr = cptr_alloc_range (1);
  untyped_retype (untyped, cap_x86_64_pt, 0, init_cap_root_cnode, init_cap_root_cnode, 64, cptr, 1);
  return cptr;
}
