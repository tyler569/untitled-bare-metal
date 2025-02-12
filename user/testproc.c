#include "stdio.h"

#include "./lib.h"

struct ipc_buffer *__ipc_buffer;

int
main (cptr_t endpoint_cap)
{
  printf ("Hello, World from userland thread! Arg is %lu\n", endpoint_cap);

  bool done = false;
  message_info_t info, resp;
  word_t badge;

  printf ("signalling notification: %lu\n", endpoint_cap + 2);
  signal (endpoint_cap + 2);
  printf ("signal done\n");

  info = recv (endpoint_cap, &badge);

  while (!done)
    {
      word_t label = get_message_label (info);

      switch (label)
        {
        case 0:
          done = true;
          break;
        case 1:
          set_mr (0, 42);
          break;
        case 2:
          set_mr (0, get_mr (0) * 2);
          break;
        case 3:
          set_mr (0, get_mr (0) + 1);
          break;
        case 4:
          set_mr (0, get_mr (0) + get_mr (1));
          break;
        default:
          set_mr (0, get_message_label (info));
          break;
        }

      resp = new_message_info (label, 0, 0, 1);
      info = reply_recv (endpoint_cap, resp, &badge);
    }

  exit (0);
  return 0;
}

void
c_start (cptr_t endpoint_cap)
{
  main (endpoint_cap);
  exit (0);
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm ("mov %%r15, %0" : "=m"(__ipc_buffer));
  asm volatile ("jmp c_start");
}
