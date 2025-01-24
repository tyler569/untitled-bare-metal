#include "kern/kernel.h"
#include "limine.h"
#include "stddef.h"
#include "stdio.h"

static struct limine_module_request moduleinfo = {
  .id = LIMINE_MODULE_REQUEST,
};

bool
get_initrd_info (void **initrd_start, size_t *initrd_size)
{
  struct limine_module_response *resp = volatile_read (moduleinfo.response);

  if (resp->module_count == 0)
    return false;

  for (int i = 0; i < resp->module_count; i++)
    printf ("Module %d: %s\n", i, resp->modules[i]->path);

  *initrd_start = (void *)resp->modules[0]->address;
  *initrd_size = resp->modules[0]->size;

  return true;
}
