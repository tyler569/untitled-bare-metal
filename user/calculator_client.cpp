#include "calculator_server.h"
#include "tern/ipc.hpp"

extern "C" int
tern_main (cptr_t endpoint_cap)
{
  printf ("C++ calculator client: starting\n");
  const tern::endpoint calculator{ endpoint_cap };
  // Pascal's triangle computes binomial coefficients using only addition.
  word_t row[21] = { 1 };
  for (size_t n = 1; n <= 20; ++n)
    {
      for (size_t k = n; k > 0; --k)
        {
          auto response = calculator.call (
              tern::message{ CALCULATOR_ADD, row[k - 1], row[k] });
          if (!response || response->size () != 1)
            {
              printf ("C++ calculator: invalid addition reply (label %lu)\n",
                      response.reply_label ());
              return 1;
            }
          row[k] = response->word (0);
        }
    }
  printf ("C++ Pascal row 20:");
  for (auto coefficient : row)
    printf (" %lu", coefficient);
  printf ("\nC++ lattice paths across a 10x10 grid: %lu\n", row[10]);
  if (row[10] != 184756)
    return 1;

  // A later call must not overwrite the saved reply's payload.
  auto saved = calculator.call (tern::message{ CALCULATOR_RET42 });
  auto next = calculator.call (tern::message{ CALCULATOR_ADD, 10, 20 });
  if (!saved || !next || saved->size () != 1 || next->size () != 1
      || saved->word (0) != 42 || next->word (0) != 30)
    return 1;
  auto rejected = calculator.call (tern::message{ 999 });
  if (rejected || rejected.error () != tern::ipc_error::unexpected_label
      || rejected.reply_label () != CALCULATOR_ERROR_UNKNOWN)
    return 1;
  printf ("C++ calculator client: PASS (210 additions, reply lifetime, "
          "protocol error)\n");
  return 0;
}
