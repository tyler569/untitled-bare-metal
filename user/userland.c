#include "stdint.h"
#include "stdio.h"
#include "sys/cdefs.h"

#ifdef __x86_64__
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
#elifdef __aarch64__
long syscall(long number, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    register long x0 asm("x0") = arg1;
    register long x1 asm("x1") = arg2;
    register long x2 asm("x2") = arg3;
    register long x3 asm("x3") = arg4;
    register long x4 asm("x4") = arg5;
    register long x5 asm("x5") = arg6;
    register long x8 asm("x8") = number;

    asm volatile (
        "svc #0"
        : "=r"(x0)
        : "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
        : "memory"
    );

    return x0;
}

static uintptr_t
_syscall0 (int syscall_num)
{
  return syscall(syscall_num, 0, 0, 0, 0, 0, 0);
}

static uintptr_t
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  return syscall(syscall_num, a1, a2, 0, 0, 0, 0);
}
#endif

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
