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
cptr_t create_page_tables (cptr_t untyped, uintptr_t min_address,
                           uintptr_t max_address);

#define assert(x)                                                             \
  if (!(x))                                                                   \
    {                                                                         \
      printf ("assertion failed: %s\n", #x);                                  \
      unreachable ();                                                         \
    }

int
create_process (void *elf_data, size_t elf_size, cptr_t untyped, cptr_t *tcb,
                cptr_t *cspace)
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

          if (lowest_addr == 0 || start < lowest_addr)
            lowest_addr = start;
          if (end > highest_addr)
            highest_addr = end;
          needed_pages += (end - start + 0xfff) / 0x1000;

          printf (
              "want to load %010lx-%010lx to %016lx-%016lx, needs %zu pages\n",
              fstart, fend, start, end, (end - start + 0xfff) / 0x1000);
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
allocate (cptr_t untyped, word_t type, size_t n)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, 0, init_cap_root_cnode, init_cap_root_cnode,
                  64, cptr, n);
  return cptr;
}

cptr_t
create_buffer (cptr_t untyped, size_t pages)
{
  return allocate (untyped, cap_x86_64_page, pages);
}

int
map_page (cptr_t untyped, cptr_t vspace, cptr_t page, uintptr_t addr)
{
  while (1)
    {
      int err = x86_64_page_map (page, vspace, addr, 0x7);

      if (err != failed_lookup)
        return err;

      switch (get_mr (0))
        {
        case 3:
          cptr_t pdpt = allocate (untyped, cap_x86_64_pdpt, 1);
          assert (x86_64_pdpt_map (pdpt, vspace, addr, 0x7) == no_error);
          break;
        case 2:
          cptr_t pd = allocate (untyped, cap_x86_64_pd, 1);
          assert (x86_64_pd_map (pd, vspace, addr, 0x7) == no_error);
          break;
        case 1:
          cptr_t pt = allocate (untyped, cap_x86_64_pt, 1);
          assert (x86_64_pt_map (pt, vspace, addr, 0x7) == no_error);
          break;
        default:
          assert (0);
        }
    }
}

int
map_buffer (cptr_t untyped, cptr_t vspace, cptr_t buffer, size_t pages,
            uintptr_t addr)
{
  int err;
  for (size_t i = 0; i < pages; i++)
    {
      err = map_page (untyped, vspace, buffer + i, addr + i * 0x1000);
      if (err != 0)
        return err;
    }

  return no_error;
}
