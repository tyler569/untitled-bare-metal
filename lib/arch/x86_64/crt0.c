#include <sys/cdefs.h>
#include <sys/ipc.h>

struct ipc_buffer *__ipc_buffer;

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm ("mov %%r15, %0" : "=m"(__ipc_buffer));
  asm ("jmp main");
}
