#include "kernel/arch.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/cdefs.h>

USED [[noreturn]] void
halt_forever ()
{
  while (true)
    {
      asm volatile ("cli");
      asm volatile ("hlt");
    }
}

void
outb (uint16_t port, uint8_t value)
{
  asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t
inb (uint16_t port)
{
  uint8_t ret;
  asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void
write_e9 (char c)
{
  outb (0xe9, c);
}

ssize_t
arch_debug_write (FILE *, const void *str, size_t len)
{
  for (size_t i = 0; i < len; i++)
    write_e9 (((char *)str)[i]);

  return (ssize_t)len;
}