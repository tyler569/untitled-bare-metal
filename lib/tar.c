#include "ctype.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "tar.h"

static long
tar_number (char *p)
{
  char *end;
  long result = strtol (p, &end, 8);
  if (end == p || *end != '\0')
    {
      printf ("Invalid octal number: %s\n", p);
      return -1;
    }
  return result;
}

void *
find_tar_entry (struct tar_header *tar, const char *name)
{
  while (tar->name[0])
    {
      if (strcmp (tar->name, name) == 0)
        return (char *)tar + 512;

      long len = tar_number (tar->size);

      // COPYPASTE from above print_all_files
      uintptr_t next_tar = (uintptr_t)tar;
      next_tar += len + 0x200;

      next_tar += 0x1ff;
      next_tar &= ~0x1ff;

      tar = (struct tar_header *)next_tar;
    }

  return nullptr;
}
