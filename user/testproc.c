#include "stdio.h"

#include "./lib.h"

extern inline long write (FILE *, const void *str, unsigned long len);

struct ipc_buffer *__ipc_buffer;

int
main (cptr_t endpoint_cap)
{
  printf ("Hello, World from userland thread! Arg is %lu\n", endpoint_cap);

  bool done = false;
  message_info_t info, resp;
  word_t badge;

  printf ("signalling notification: %lu\n", endpoint_cap + 1);
  signal (endpoint_cap + 1);
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

  exit ();
  return 0;
}

void
c_start (void *ipc_buffer, cptr_t endpoint_cap)
{
  __ipc_buffer = ipc_buffer;

  main (endpoint_cap);
  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("jmp c_start");
}
