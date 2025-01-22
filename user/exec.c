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
cptr_t create_page_tables (cptr_t untyped, uintptr_t min_address, uintptr_t max_address);

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

          if (lowest_addr == 0 || start < lowest_addr)
            lowest_addr = start;
          if (end > highest_addr)
            highest_addr = end;
          needed_pages += (end - start + 0xfff) / 0x1000;

          printf ("want to load %010lx-%010lx to %016lx-%016lx, needs %zu pages\n", fstart,
                  fend, start, end, (end - start + 0xfff) / 0x1000);
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
  untyped_retype (untyped, type, 0, init_cap_root_cnode, init_cap_root_cnode, 64, cptr, n);
  return cptr;
}

#define allocate_pages(untyped, n) allocate(untyped, cap_x86_64_page_table, n)
#define allocate_pml4(untyped) allocate(untyped, cap_x86_64_pml4, 1)
#define allocate_pdpt(untyped) allocate(untyped, cap_x86_64_pdpt, 1)
#define allocate_pd(untyped) allocate(untyped, cap_x86_64_pd, 1)
#define allocate_pt(untyped) allocate(untyped, cap_x86_64_pt, 1)

constexpr uintptr_t page_size = 0x1000;
constexpr uintptr_t page_table_size = page_size * 512;
constexpr uintptr_t page_directory_size = page_table_size * 512;

cptr_t
create_page_tables (cptr_t untyped, uintptr_t min_address, uintptr_t max_address)
{
  min_address &= page_directory_size - 1;
  max_address = (max_address + page_directory_size - 1) & ~(page_directory_size - 1);

  // assert (max_address > min_address);
  // assert (max_address < page_directory_size);

  cptr_t pml4 = allocate_pml4 (untyped);

  cptr_t pdpt = allocate_pdpt (untyped);
  x86_64_pdpt_map (pml4, pdpt, 0, 0);

  cptr_t pd = allocate_pd (untyped);
  x86_64_pd_map (pml4, pd, 0, 0);

  for (uintptr_t i = min_address; i < max_address; i += 0x200000)
    {
      cptr_t pt = allocate_pt (untyped);
      x86_64_pd_map (pd, pt, i, 0);
    }

  return pml4;
}
