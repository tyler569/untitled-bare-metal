#include "assert.h"
#include "chacha.h"
#include "kern/arch.h"
#include "kern/mem.h"
#include "kernel.h"
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

void
run_smoke_tests ()
{
  printf ("Smoke tests:\n");

  printf ("  Interrupt\n");
  printf ("    ");
  debug_trap ();

  printf ("  Sort\n");
  run_sort_test ();
}
