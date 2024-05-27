#include "kernel.h"
#include "kern/mem.h"
#include "sys/mman.h"
#include <stdio.h>
#include <stdlib.h>

void
halt_forever ()
{
  printf ("Halting forever\n");
  exit (0);
}

uintptr_t
get_vm_root ()
{
  return 0;
}

void
add_vm_mapping (uintptr_t, uintptr_t, uintptr_t, int)
{
}

uintptr_t
resolve_vm_mapping (uintptr_t, uintptr_t virt)
{
  return virt;
}

uintptr_t
alloc_kernel_vm (size_t)
{
  return 0;
}

void
debug_trap ()
{
  __builtin_trap ();
}

void
get_physical_extents (struct physical_extent *extents, size_t *count)
{
  size_t mem_len = 128 * 1024 * 1024;
  void *map = mmap (nullptr, mem_len, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  extents[0] = (struct physical_extent){
    .start = (uintptr_t)map,
    .len = mem_len,
    .flags = PHYSICAL_EXTENT_FREE,
  };
  *count = 1;
}

ssize_t
write_debug (struct stream *, const void *str, size_t len)
{
  fwrite (str, 1, len, stdout);
  return (ssize_t)len;
}

void
relax_busy_loop ()
{
}

int
main ()
{
  init_page_mmap ();

  kernel_main ();
}