#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/kernel.h"
#include "kern/mem.h"
#include "kern/obj/tcb.h"
#include "limine.h"
#include "stddef.h"
#include "sys/bootinfo.h"
#include "tar.h"

constexpr size_t cnode_slots = BIT (INIT_CNODE_SIZE_BITS);

struct tcb init_tcb;
cte_t init_cnode[cnode_slots];

static struct limine_framebuffer_request fbinfo = {
  .id = LIMINE_FRAMEBUFFER_REQUEST,
};

// --- constants for "magic" addrs & flags ---
enum
{
  VA_IPC_BUFFER = 0x7fff'ffe0'0000,
  VA_BOOTINFO = 0x7fff'ffe0'1000,
  VA_INITRD = 0x1'0000'0000,

  VA_STACK = 0x7fff'fff0'0000,
  STACK_PAGES = 4,
  VA_STACK_TOP = VA_STACK + STACK_PAGES * PAGE_SIZE,

  USER_RW = PTE_PRESENT | PTE_USER | PTE_WRITE,
  USER_RO = PTE_PRESENT | PTE_USER,
};

// --- helpers ---
static inline uintptr_t
alloc_and_map_page (uintptr_t pml4, uintptr_t va, uint64_t flags)
{
  uintptr_t pa = alloc_page ();
  add_vm_mapping (pml4, va, pa, flags);
  return pa;
}

static inline void
map_virt_from_virt_phys (uintptr_t pml4, uintptr_t dst_va, uintptr_t src_va,
                         size_t size, uint64_t flags)
{
  for (size_t off = 0; off < size; off += PAGE_SIZE)
    add_vm_mapping (pml4, dst_va + off, physical_of (src_va + off), flags);
}

void
create_init_tcb (void *initrd, size_t initrd_size)
{
  struct elf_ehdr *init_elf = find_tar_entry (initrd, "userland");
  assert (init_elf && is_elf (init_elf));

  create_tcb_from_elf_in_this_vm (&init_tcb, init_elf);
  strcpy (init_tcb.name, "bootstrap");

  uintptr_t init_pml4 = get_vm_root ();

  uintptr_t ipc_buf = alloc_and_map_page (init_pml4, VA_IPC_BUFFER, USER_RW);
  uintptr_t bootinfo = alloc_and_map_page (init_pml4, VA_BOOTINFO, USER_RW);
  // todo: could/should we share this some other way?
  set_frame_r15 (&init_tcb.saved_state, VA_IPC_BUFFER);
  set_frame_arg (&init_tcb.saved_state, 0, VA_BOOTINFO);

  // stack: N pages contiguous VA, any PA
  for (size_t i = 0; i < STACK_PAGES; i++)
    alloc_and_map_page (init_pml4, VA_STACK + PAGE_SIZE * i, USER_RW);
  set_frame_sp (&init_tcb.saved_state, VA_STACK_TOP);

  // bootinfo struct (kernel direct map)
  struct boot_info *bi = (struct boot_info *)direct_map_of (bootinfo);

  // map initrd read-only into user VA
  map_virt_from_virt_phys (init_pml4, VA_INITRD, (uintptr_t)initrd,
                           initrd_size, USER_RO);
  bi->initrd = (void *)VA_INITRD;
  bi->initrd_size = initrd_size;

  // ipc buffer kernel VA
  init_tcb.ipc_buffer = (struct ipc_buffer *)direct_map_of (ipc_buf);

  // caps
  cap_t init_tcb_cap = cap_tcb_new (&init_tcb);
  cap_t init_cnode_cap = cap_cnode_new (init_cnode, INIT_CNODE_SIZE_BITS);
  cap_t init_vspace_cap = cap_x86_64_pml4_new (direct_map_of (get_vm_root ()));
  cap_t init_io_port_cap = cap_x86_64_io_port_control_new ();
  cap_t init_irq_control_cap = cap_irq_control_new ();

  // fill well-known slots
  struct
  {
    size_t slot;
    cap_t cap;
  } seed_caps[] = {
    { init_cap_init_tcb, init_tcb_cap },
    { init_cap_root_cnode, init_cnode_cap },
    { init_cap_init_vspace, init_vspace_cap },
    { init_cap_io_port_control, init_io_port_cap },
    { init_cap_irq_control, init_irq_control_cap },
  };

  for (size_t i = 0; i < sizeof (seed_caps) / sizeof (seed_caps[0]); i++)
    init_cnode[seed_caps[i].slot].cap = seed_caps[i].cap;

  init_tcb.cspace_root.cap = init_cnode_cap;
  init_tcb.vspace_root.cap = init_vspace_cap;

  // untyped ranges
  size_t n_untyped = cnode_slots - init_cap_first_untyped;
  create_init_untyped_caps (init_cnode + init_cap_first_untyped, &n_untyped,
                            bi->untypeds);

  size_t n_free_slots = cnode_slots - init_cap_first_untyped - n_untyped;
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
    .end = cnode_slots,
  };

  bi->framebuffer_info = *volatile_read (fbinfo.response)->framebuffers[0];

  // init_tcb.debug = true;

  make_tcb_runnable (&init_tcb);
  schedule ();
}
