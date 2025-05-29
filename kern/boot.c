#include "arch/x86_64/exports.h"
#include "kern/cap.h"
#include "kern/kernel.h"
#include "kern/mem.h"
#include "kern/obj/tcb.h"
#include "limine.h"
#include "stddef.h"
#include "sys/bootinfo.h"
#include "tar.h"

struct tcb init_tcb;
cte_t init_cnode[BIT (INIT_CNODE_SIZE_BITS)];

static struct limine_framebuffer_request fbinfo = {
  .id = LIMINE_FRAMEBUFFER_REQUEST,
};

void
create_init_tcb (void *initrd, size_t initrd_size)
{
  struct elf_ehdr *init_elf = find_tar_entry (initrd, "main");

  assert (init_elf);
  assert (is_elf (init_elf));

  create_tcb_from_elf_in_this_vm (&init_tcb, init_elf);

  strcpy (init_tcb.name, "bootstrap");

  uintptr_t ipc_buffer = 0x7fffffe00000;
  uintptr_t bootinfo = 0x7fffffe01000;

  uintptr_t ipc_buffer_page = alloc_page ();
  uintptr_t bootinfo_page = alloc_page ();

  uintptr_t init_pml4 = get_vm_root ();

  add_vm_mapping (init_pml4, ipc_buffer, ipc_buffer_page,
                  PTE_PRESENT | PTE_USER | PTE_WRITE);
  init_tcb.saved_state.r15 = ipc_buffer;
  add_vm_mapping (init_pml4, bootinfo, bootinfo_page,
                  PTE_PRESENT | PTE_USER | PTE_WRITE);
  set_frame_arg (&init_tcb.saved_state, 0, bootinfo);

  uintptr_t stack = 0x7ffffff00000;
  constexpr size_t stack_pages = 4;

  for (size_t i = 0; i < stack_pages; i++)
    {
      uintptr_t stack_page = alloc_page ();
      add_vm_mapping (init_pml4, stack + PAGE_SIZE * i, stack_page,
                      PTE_PRESENT | PTE_USER | PTE_WRITE);
    }

  init_tcb.saved_state.rsp = stack + PAGE_SIZE * stack_pages;

  struct boot_info *bi = (struct boot_info *)direct_map_of (bootinfo_page);

  uintptr_t init_elf_mapping = 0x100000000;

  for (size_t i = 0; i < initrd_size; i += PAGE_SIZE)
    {
      uintptr_t page_phy = physical_of ((uintptr_t)initrd + i);
      add_vm_mapping (init_pml4, init_elf_mapping + i, page_phy,
                      PTE_PRESENT | PTE_USER);
    }

  bi->initrd = (void *)init_elf_mapping;
  bi->initrd_size = initrd_size;

  init_tcb.ipc_buffer = (struct ipc_buffer *)direct_map_of (ipc_buffer_page);

  cap_t init_tcb_cap = cap_tcb_new (&init_tcb);
  cap_t init_cnode_cap = cap_cnode_new (init_cnode, INIT_CNODE_SIZE_BITS);
  cap_t init_vspace_cap = cap_x86_64_pml4_new (direct_map_of (get_vm_root ()));
  cap_t init_io_port_cap = cap_x86_64_io_port_control_new ();
  cap_t init_irq_control_cap = cap_irq_control_new ();

  init_cnode[init_cap_init_tcb].cap = init_tcb_cap;
  init_cnode[init_cap_root_cnode].cap = init_cnode_cap;
  init_cnode[init_cap_init_vspace].cap = init_vspace_cap;
  init_cnode[init_cap_io_port_control].cap = init_io_port_cap;
  init_cnode[init_cap_irq_control].cap = init_irq_control_cap;

  init_tcb.cspace_root.cap = init_cnode_cap;
  init_tcb.vspace_root.cap = init_vspace_cap;

  size_t n_untyped = BIT (INIT_CNODE_SIZE_BITS) - init_cap_first_untyped;

  create_init_untyped_caps (init_cnode + init_cap_first_untyped, &n_untyped,
                            bi->untypeds);

  size_t n_free_slots
      = BIT (INIT_CNODE_SIZE_BITS) - init_cap_first_untyped - n_untyped;

  create_init_untyped_device_caps (init_cnode + init_cap_first_untyped
                                       + n_untyped,
                                   &n_free_slots, bi->untypeds + n_untyped);
  n_untyped += n_free_slots;
  bi->n_untypeds += n_untyped;

  bi->untyped_range = (struct cap_range){
    .start = init_cap_first_untyped,
    .end = init_cap_first_untyped + n_untyped,
  };

  bi->empty_range = (struct cap_range){
    .start = init_cap_first_untyped + n_untyped,
    .end = BIT (INIT_CNODE_SIZE_BITS),
  };

  bi->framebuffer_info = *volatile_read (fbinfo.response)->framebuffers[0];

  init_tcb.cspace_root.cap = init_cnode_cap;

  // init_tcb.debug = true;

  make_tcb_runnable (&init_tcb);
  schedule ();
}
