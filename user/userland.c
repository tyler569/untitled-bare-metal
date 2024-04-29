#include "stdint.h"
#include "sys/cdefs.h"

static int
_syscall0 (int syscall_num)
{
  asm volatile ("syscall"
                : "=a"(syscall_num)
                : "0"(syscall_num)
                : "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11");
  return syscall_num;
}

static int
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  asm volatile ("syscall"
                : "=a"(syscall_num)
                : "0"(syscall_num), "D"(a1), "S"(a2)
                : "rcx", "rdx", "r8", "r9", "r10", "r11");
  return syscall_num;
}

char stack[4096] __attribute__ ((aligned (16)));

void
thread_entry (void)
{
  _syscall2 (1, (uintptr_t) "Hello, World from userland 2!", 29);
  _syscall0 (3);
  _syscall0 (0);
}

USED int
_start ()
{
  _syscall2 (1, (uintptr_t) "Hello, World from userland!", 27);
  _syscall2 (2, (uintptr_t) thread_entry, (uintptr_t) stack + sizeof (stack));
  _syscall0 (0);

  UNREACHABLE ();
}

