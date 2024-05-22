#include "arm64.h"
#include "stdint.h"
#include "sys/types.h"

struct stream;

volatile uint8_t *uart = (uint8_t *)0x9000000;

ssize_t
write_debug (struct stream *, const char *str, size_t len) {
  for (size_t i = 0; i < len && str[i]; i++)
    *uart = str[i];
  return len;
}

void
relax_busy_loop ()
{
  asm volatile ("yield");
}

void
halt_forever ()
{
  while (true)
	{
	  asm volatile ("msr daifset, #0xf");
	  asm volatile ("wfi");
	}
}

void
halt_forever_interrupts_enabled ()
{
  while (true)
	asm volatile ("wfi");
}
