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
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1), "S"(a2)
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
clone (void (*fn) (void), void *stack)
{
  _syscall2 (2, (uintptr_t)fn, (uintptr_t)stack);
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
      if (rx != 0 && rx % 100 == 0)
        asm volatile ("int3");
    }
}

[[noreturn]] void
client (uintptr_t task)
{
  unsigned i = 0;
  for (;;)
    send (task, i++);
}

[[noreturn]] USED int
_start (uintptr_t arg)
{
  printf ("Hello, World from userland; arg: %lu!\n", arg);

  uintptr_t task;

  if (arg == 0)
    {
      task = create (1);
      yield ();
      client (task);
    }
  else
    server ();

  exit ();
}
