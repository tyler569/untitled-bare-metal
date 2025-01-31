#include "assert.h"
#include "chacha.h"
#include "kern/arch.h"
#include "kern/kernel.h"
#include "kern/mem.h"
#include "rng.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

void hexdump (const void *data, size_t len);

[[noreturn]] void
panic (const char *msg, ...)
{
  va_list args;
  va_start (args, msg);

  printf ("PANIC: ");
  vprintf (msg, args);
  va_end (args);

  printf ("\n");

  debug_trap ();
  halt_forever ();
}

int
int_cmp (const void *a, const void *b)
{
  return *(int *)a - *(int *)b;
}

void
run_sort_test ()
{
  int data[] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
  size_t len = sizeof (data) / sizeof (data[0]);

  printf ("    Before: ");
  for (size_t i = 0; i < len; i++)
    printf ("%d ", data[i]);
  printf ("\n");

  qsort (data, len, sizeof (data[0]), int_cmp);

  printf ("    After:  ");
  for (size_t i = 0; i < len; i++)
    printf ("%d ", data[i]);
  printf ("\n");
}

#define __cpuid_count(level, count, eax, ebx, ecx, edx)                       \
  asm volatile ("cpuid"                                                       \
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)                  \
                : "a"(level), "c"(count))

#define __cpuid(level, eax, ebx, ecx, edx)                                    \
  __cpuid_count (level, 0, eax, ebx, ecx, edx)

void
get_xsave_size ()
{
  uint32_t eax, ebx, ecx, edx;

  // Check if CPU supports XSAVE
  __cpuid (1, eax, ebx, ecx, edx);
  if (!(ecx & (1 << 26)))
    {
      printf ("    XSAVE not supported.\n");
      return;
    }

  __cpuid (0xD, eax, ebx, ecx, edx);
  printf ("    Required XSAVE area size: %u bytes\n", ebx);
}

void
run_smoke_tests ()
{
  printf ("Smoke tests:\n");

  printf ("  Interrupt\n");
  printf ("    ");
  asm volatile ("int $255");

  printf ("  Sort\n");
  run_sort_test ();

  printf ("  XSAVE\n");
  get_xsave_size ();
}
