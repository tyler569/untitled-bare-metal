#include "stdio.h"

#include "./lib.h"
#include "calculator_server.h"

struct ipc_buffer *__ipc_buffer;

int
main (cptr_t endpoint_cap)
{
  printf ("Hello, World from userland thread! Arg is %lu\n", endpoint_cap);

  bool done = false;
  message_info_t info, resp;
  word_t badge;

  info = recv (endpoint_cap, &badge);

  while (!done)
    {
      word_t label = get_message_label (info);
      int err = 0;

      switch (label)
        {
        case calculator_quit:
          done = true;
          break;
        case calculator_ret42:
          set_mr (0, 42);
          break;
        case calculator_double:
          set_mr (0, get_mr (0) * 2);
          break;
        case calculator_inc:
          set_mr (0, get_mr (0) + 1);
          break;
        case calculator_add:
          set_mr (0, get_mr (0) + get_mr (1));
          break;
        default:
          err = calculator_error_unknown;
          break;
        }

      if (err)
        resp = new_message_info (err, 0, 0, 1);
      else
        resp = new_message_info (label, 0, 0, 1);

      if (done)
        reply (endpoint_cap);
      else
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
