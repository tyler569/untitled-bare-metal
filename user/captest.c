#include "captest.h"
#include "lib.h"

struct ipc_buffer *__ipc_buffer;

int
main ()
{
  while (true)
    {
      word_t badge;
      message_info_t tag = recv (captest_endpoint, &badge);

      printf ("captest: Received message from badge %d\n", badge);

      reply (0);
    }
}

[[noreturn]] void
c_start ()
{
  main ();
  exit (0);
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm ("mov %%r15, %0" : "=m"(__ipc_buffer));
  asm volatile ("jmp c_start");
}
