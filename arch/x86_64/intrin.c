#include "kernel.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/lock.h>

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
wrmsr (uint32_t msr_id, uint64_t value)
{
  uint32_t low = value & 0xffffffff;
  uint32_t high = value >> 32;
  asm volatile ("wrmsr" : : "c"(msr_id), "a"(low), "d"(high));
}

uint64_t
rdmsr (uint32_t msr_id)
{
  uint32_t low, high;
  asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr_id));
  return ((uint64_t)high << 32) | low;
}

uintptr_t
read_cr2 ()
{
  uintptr_t ret;
  asm volatile ("mov %%cr2, %0" : "=r"(ret));
  return ret;
}

void
write_e9 (char c)
{
  outb (0xe9, c);
}

ssize_t
write_debug (FILE *, const void *str, size_t len)
{
  static spin_lock_t lock;

  spin_lock (&lock);

  for (size_t i = 0; i < len; i++)
    write_e9 (((char *)str)[i]);

  spin_unlock (&lock);

  return (ssize_t)len;
}

void
cpu_relax ()
{
  asm volatile ("pause");
}
