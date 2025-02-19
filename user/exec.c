#include "elf.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "sys/bootinfo.h"
#include "sys/syscall.h"

#include "./lib.h"

static inline struct elf_phdr *
get_phdr (struct elf_ehdr *ehdr, size_t i)
{
  return (struct elf_phdr *)((char *)ehdr + ehdr->phoff + i * ehdr->phentsize);
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

constexpr size_t default_stack_pages = 4;

int
spawn_thread (struct thread_data *data)
{
  struct elf_ehdr *ehdr = data->elf_header;
  if (!is_elf (ehdr))
    return invalid_argument;

  cptr_t untyped = data->untyped;
  cptr_t our_vspace = data->scratch_vspace;
  size_t stack_pages = data->stack_pages ?: default_stack_pages;

  cptr_t tcb = allocate (untyped, cap_tcb, 1);
  cptr_t vspace = allocate (untyped, cap_x86_64_pml4, 1);
  buffer_t ipc_buffer = create_buffer (untyped, 1);
  buffer_t stack_buffer = create_buffer (untyped, stack_pages);
  cptr_t cspace_root = data->cspace_root ?: init_cap_root_cnode;

  uintptr_t highest_addr
      = map_elf_to_new_vspace (ehdr, untyped, vspace, our_vspace);

  highest_addr += 0x1000;
  highest_addr = (highest_addr + 0xfff) & ~0xfff;
  uintptr_t ipc_addr = highest_addr;

  map_buffer (untyped, vspace, ipc_buffer, highest_addr);

  highest_addr += 0x2000;
  uintptr_t stack_addr = highest_addr + stack_pages * 0x1000;

  map_buffer (untyped, vspace, stack_buffer, highest_addr);

  tcb_configure (tcb, 0, cspace_root, 0, vspace, 0, ipc_addr,
                 ipc_buffer.cptr_base);

  frame_t regs = {
    .rip = ehdr->entry,
    .rsp = stack_addr,
    .cs = 0x23,
    .ss = 0x1b,
    .r15 = ipc_addr,
    .rdi = data->arguments[0],
    .rsi = data->arguments[1],
    .rdx = data->arguments[2],
    .rcx = data->arguments[3],
    .r8 = data->arguments[4],
    .r9 = data->arguments[5],
  };

  tcb_write_registers (tcb, false, 0, sizeof (regs), &regs);
  if (data->name)
    tcb_set_name (tcb, data->name, strlen (data->name));

  data->tcb = tcb;
  data->vspace = vspace;
  data->ipc_buffer = ipc_buffer;
  data->stack_buffer = stack_buffer;

  return 0;
}
