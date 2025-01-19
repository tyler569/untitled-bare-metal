#include "../x86_64.h"
#include "kern/cap.h"
#include "kern/syscall.h"

message_info_t
x86_64_pdpt_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  return return_ipc (no_error, 0);
}
