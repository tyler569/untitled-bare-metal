#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/mem.h"
#include "kern/obj/cnode.h"
#include "kern/obj/tcb.h"
#include "sys/bootinfo.h"

struct tcb init_tcb;
cte_t init_cnode[BIT (INIT_CNODE_SIZE_BITS)];

void
create_init_tcb (void *init_elf)
{
  create_tcb_from_elf_in_this_vm (&init_tcb, init_elf);

  uintptr_t ipc_buffer = 0x7fffffe00000;
  uintptr_t ipc_buffer_page = alloc_page ();

  add_vm_mapping (init_tcb.vm_root, ipc_buffer, ipc_buffer_page,
                  PTE_PRESENT | PTE_USER | PTE_WRITE);
  set_frame_arg (&init_tcb.saved_state, 0, ipc_buffer);

  init_tcb.ipc_buffer_user = ipc_buffer;
  init_tcb.ipc_buffer = (struct ipc_buffer *)direct_map_of (ipc_buffer_page);

  cap_t init_cnode_cap = cap_cnode_new (init_cnode, INIT_CNODE_SIZE_BITS);

  init_cnode[1].cap = cap_tcb_new (&init_tcb);
  init_cnode[2].cap = init_cnode_cap;
  init_cnode[3].cap = cap_vspace_new (get_vm_root ());

  init_tcb.cspace_root = init_cnode_cap;

  make_tcb_runnable (&init_tcb);
}
