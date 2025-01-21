#include "elf.h"
#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#include "./lib.h"

static inline struct phdr *get_phdr (struct elf_ehdr *, size_t);
cptr_t allocate_pages (size_t);

int create_process (void *elf_data, size_t elf_size, cptr_t *tcb, cptr_t *cspace)
{
  struct elf_ehdr *ehdr = (struct elf_ehdr *)elf_data;
  if (!is_valid_elf (ehdr))
    return -1;

  for (int i = 0; i < ehdr->e_phnum; i++)
    {
      struct phdr *phdr = get_phdr(ehdr, i);
      if (phdr->p_type == PT_LOAD)
        {
        }
    }

}

static inline struct phdr *get_phdr (struct elf_ehdr *ehdr, size_t i)
{
  return (struct phdr *)((char *)ehdr + ehdr->e_phoff + i * ehdr->e_phentsize);
}

cptr_t allocate_pages (size_t n)
{
  untyped_retype (x86_64_page, 12, init_cap_root_cnode, 64, 100, n);
}
