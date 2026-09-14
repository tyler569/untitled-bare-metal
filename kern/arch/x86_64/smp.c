#include "kern/arch.h"
#include "kern/kernel.h"
#include "kern/obj/tcb.h"
#include "limine.h"
#include "stdio.h"
#include "x86_64.h"

constexpr size_t MAX_CPUS = 64;

static struct limine_mp_request smpinfo = {
  .id = LIMINE_MP_REQUEST_ID,
};

per_cpu_t cpus[MAX_CPUS];

void
ap_entry (struct limine_mp_info *info)
{
  per_cpu_t *cpu = &cpus[info->processor_id];
  cpu->self = cpu;
  cpu->num = info->lapic_id;

  init_ap_idt ();
  init_ap_gdt (cpu);
  init_ap_int_stacks ();
  init_pic ();
  init_lapic ();

  printf ("AP started\n");

  enable_interrupts ();

  schedule ();
  return_from_kernel_code ();

  halt_forever ();
}

void
init_aps ()
{
  struct limine_mp_response *resp = volatile_read (smpinfo.response);

  if (!resp || resp->cpu_count <= 1)
    return;

  for (size_t i = 0; i < resp->cpu_count; i++)
    {
      if (resp->cpus[i]->processor_id >= MAX_CPUS)
        continue;
      if (resp->cpus[i]->lapic_id == resp->bsp_lapic_id)
        continue;

      resp->cpus[i]->goto_address = ap_entry;
    }
}
