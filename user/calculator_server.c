#include "stdio.h"

#include "./lib.h"
#include "calculator_server.h"

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
        case CALCULATOR_QUIT:
          done = true;
          break;
        case CALCULATOR_RET42:
          set_mr (0, 42);
          break;
        case CALCULATOR_DOUBLE:
          set_mr (0, get_mr (0) * 2);
          break;
        case CALCULATOR_INC:
          set_mr (0, get_mr (0) + 1);
          break;
        case CALCULATOR_ADD:
          set_mr (0, get_mr (0) + get_mr (1));
          break;
        default:
          err = CALCULATOR_ERROR_UNKNOWN;
          break;
        }

      if (err)
        resp = new_message_info (err, 0, 0, 0);
      else
        resp = new_message_info (label, 0, 0, 1);

      if (done)
        reply (resp);
      else
        info = reply_recv (endpoint_cap, resp, &badge);
    }

  exit (0);
}
