#include "assert.h"
#include "kern/arch.h"
#include "kern/kernel.h"
#include "stdio.h"
#include "stdlib.h"

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

static int
int_cmp (const void *a, const void *b)
{
  const int *ia = a, *ib = b;
  return *ia - *ib;
}

static void
run_sort_test ()
{
  int data[] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3 };
  constexpr size_t len = sizeof (data) / sizeof (data[0]);

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

#define CPUID_COUNT(level, count, eax, ebx, ecx, edx)                         \
  asm volatile ("cpuid"                                                       \
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)                  \
                : "a"(level), "c"(count))

#define CPUID(level, eax, ebx, ecx, edx)                                      \
  CPUID_COUNT (level, 0, eax, ebx, ecx, edx)

static void
get_xsave_size ()
{
  uint32_t eax, ebx, ecx, edx;

  // Check if CPU supports XSAVE
  CPUID (1, eax, ebx, ecx, edx);
  if (!(ecx & (1 << 26)))
    {
      printf ("    XSAVE not supported.\n");
      return;
    }

  CPUID (0xD, eax, ebx, ecx, edx);
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
