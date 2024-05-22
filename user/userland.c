#include "stdint.h"
#include "stdio.h"
#include "sys/cdefs.h"

static uintptr_t
_syscall0 (int syscall_num)
{
  uintptr_t ret;
  asm volatile ("syscall" : "=a"(ret) : "0"(syscall_num) : "rcx", "r11");
  return ret;
}

static uintptr_t
_syscall1 (int syscall_num, uintptr_t a1)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1)
                : "rcx", "r11");
  return ret;
}

static uintptr_t
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1), "S"(a2)
                : "rcx", "r11");
  return ret;
}

static uintptr_t
_syscall4 (int syscall_num, uintptr_t a1, uintptr_t a2, uintptr_t a3,
           uintptr_t _a4)
{
  uintptr_t ret;
  UNUSED register uintptr_t a4 asm ("r10") = _a4;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1), "S"(a2), "d"(a3)
                : "rcx", "r11");
  return ret;
}

[[noreturn]] static inline void
exit ()
{
  _syscall0 (0);
  UNREACHABLE ();
}

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (1, (uintptr_t)str, len);
  return (long)len;
}

static inline void
yield ()
{
  _syscall0 (3);
}

static inline void
send (uintptr_t to, uintptr_t msg)
{
  _syscall2 (4, to, msg);
}

static inline uintptr_t
receive ()
{
  return _syscall0 (5);
}

static inline uintptr_t
create (uintptr_t arg)
{
  return _syscall2 (6, 1, arg);
}

static inline void
map (uintptr_t virt, uintptr_t phys, unsigned long len, unsigned long flags)
{
  _syscall4 (7, virt, phys, len, flags);
}

static inline void
write_port_b (unsigned short port, unsigned char val)
{
  _syscall2 (8, port, val);
}

static inline uintptr_t
read_port_b (unsigned short port)
{
  return _syscall1 (9, port);
}

static void
yields ()
{
  for (int i = 0; i < 10; i++)
    yield ();
}

[[noreturn]] void
server ()
{
  for (;;)
    {
      uintptr_t rx = receive ();
      printf ("Server received: %lu\n", rx);
    }
}

[[noreturn]] void
client (uintptr_t task)
{
  unsigned i = 0;
  for (;;)
    send (task, i++);
}

enum hpet_registers
{
  HPET_CAPABILITIES = 0x0,
  HPET_CONFIGURATION = 0x10,
  HPET_INTERRUPT_STATUS = 0x20,
  HPET_MAIN_COUNTER = 0xf0,
};

void
hpet_initialize (uint32_t *hpet)
{
  printf ("HPET capabilities: %x\n", volatile_read (hpet));
}

[[noreturn]] void
hpet_driver ()
{
  uintptr_t hpet_base = 0xfed00000;
  uintptr_t hpet_vaddr = 0x10000000;
  map (hpet_vaddr, hpet_base, 0x1000, 0x7);
  uint32_t *hpet = (uint32_t *)hpet_vaddr;

  hpet_initialize (hpet);

  exit ();
}

[[noreturn]] USED int
_start (uintptr_t arg)
{
  printf ("Hello, World from userland; arg: %lu!\n", arg);

  uintptr_t task;

  switch (arg)
    {
    case 0:
	  create (1);
      create (2);
      break;
    case 1:
      server ();
    case 2:
      hpet_driver ();
    default:
      printf ("Unknown arg: %lu\n", arg);
    }

  exit ();
}
