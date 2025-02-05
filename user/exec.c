#include "elf.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#include "./lib.h"

static inline struct elf_phdr *get_phdr (struct elf_ehdr *, size_t);
cptr_t allocate (cptr_t untyped, word_t type, size_t n);
cptr_t allocate_pages (cptr_t untyped, size_t);
cptr_t create_page_tables (cptr_t untyped, uintptr_t min_address,
                           uintptr_t max_address);

buffer_t create_buffer (cptr_t untyped, size_t pages);
int map_buffer (cptr_t untyped, cptr_t vspace, buffer_t buffer,
                uintptr_t addr);

buffer_t map_phdr (cptr_t untyped, cptr_t vspace, struct elf_ehdr *ehdr,
                   struct elf_phdr *phdr);

#define assert(x)                                                             \
  if (!(x))                                                                   \
    {                                                                         \
      printf ("assertion failed: %s\n", #x);                                  \
      unreachable ();                                                         \
    }

uintptr_t
map_elf_to_new_vspace (struct elf_ehdr *ehdr, cptr_t untyped, cptr_t vspace,
                       cptr_t our_vspace)
{
  uintptr_t highest_addr = 0;

  for (int i = 0; i < ehdr->phnum; i++)
    {
      struct elf_phdr *phdr = get_phdr (ehdr, i);
      if (phdr->type == PT_LOAD)
        {
          buffer_t buffer = map_phdr (untyped, our_vspace, ehdr, phdr);
          // unmap buffer
          map_buffer (untyped, vspace, buffer, phdr->vaddr);

          if (phdr->vaddr + phdr->memsz > highest_addr)
            highest_addr = phdr->vaddr + phdr->memsz;
        }
    }

  return highest_addr;
}

int
create_process (void *elf_data, size_t elf_size, cptr_t untyped,
                cptr_t our_vspace, cptr_t *tcb, cptr_t *cspace)
{
  (void)elf_size;
  (void)untyped;
  (void)tcb;
  (void)cspace;

  struct elf_ehdr *ehdr = elf_data;
  if (!is_elf (ehdr))
    return -1;

  cptr_t vspace = allocate (untyped, cap_x86_64_pml4, 1);

  uintptr_t highest_addr
      = map_elf_to_new_vspace (ehdr, untyped, vspace, our_vspace);

  highest_addr += 0x1000;
  highest_addr = (highest_addr + 0xfff) & ~0xfff;
  uintptr_t ipc_addr = highest_addr;

  buffer_t ipc_buffer = create_buffer (untyped, 1);
  map_buffer (untyped, vspace, ipc_buffer, highest_addr);

  constexpr size_t stack_pages = 4;

  highest_addr += 0x2000;
  uintptr_t stack_addr = highest_addr + stack_pages * 0x1000;

  buffer_t stack_buffer = create_buffer (untyped, stack_pages);
  map_buffer (untyped, vspace, stack_buffer, highest_addr);

  *tcb = allocate (untyped, cap_tcb, 1);
  tcb_configure (*tcb, 0, init_cap_root_cnode, 0, vspace, 0, ipc_addr,
                 ipc_buffer.cptr_base);

  frame_t regs = {};
  regs.rip = ehdr->entry;
  regs.rsp = stack_addr;
  regs.rdi = ipc_addr;
  regs.cs = 0x23;
  regs.ss = 0x1b;

  tcb_write_registers (*tcb, false, 0, sizeof (regs), &regs);

  return 0;
}

static inline struct elf_phdr *
get_phdr (struct elf_ehdr *ehdr, size_t i)
{
  return (struct elf_phdr *)((char *)ehdr + ehdr->phoff + i * ehdr->phentsize);
}

buffer_t
create_buffer (cptr_t untyped, size_t pages)
{
  return (buffer_t){ allocate (untyped, cap_x86_64_page, pages), pages };
}

int
map_page (cptr_t untyped, cptr_t vspace, cptr_t page, uintptr_t addr)
{
  while (1)
    {
      int err = x86_64_page_map (page, vspace, addr, 0x7);

      if (err != failed_lookup) // including "no_error"
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
map_buffer (cptr_t untyped, cptr_t vspace, buffer_t buffer, uintptr_t addr)
{
  int err;
  for (size_t i = 0; i < buffer.pages; i++)
    {
      err = map_page (untyped, vspace, buffer.cptr_base + i,
                      addr + i * 0x1000);
      if (err != 0)
        return err;
    }

  return no_error;
}

uintptr_t mappable_addr = 0x900000;

uintptr_t
map_buffer_to_mappable_space (cptr_t untyped, cptr_t vspace, buffer_t buffer)
{
  uintptr_t addr = mappable_addr;
  int err = map_buffer (untyped, vspace, buffer, addr);
  mappable_addr += buffer.pages * 0x1000;
  if (err != 0)
    return 0;
  return addr;
}

void *
data_for_phdr (struct elf_ehdr *ehdr, struct elf_phdr *phdr)
{
  return (void *)ehdr + phdr->offset;
}

buffer_t
map_phdr (cptr_t untyped, cptr_t vspace, struct elf_ehdr *ehdr,
          struct elf_phdr *phdr)
{
  size_t offset = phdr->vaddr & 0xfff;

  buffer_t buffer
      = create_buffer (untyped, (phdr->memsz + offset + 0xfff) / 0x1000);
  uintptr_t mapped_addr
      = map_buffer_to_mappable_space (untyped, vspace, buffer);

  memcpy ((void *)(mapped_addr + offset), data_for_phdr (ehdr, phdr),
          phdr->filesz);
  return buffer;
}
