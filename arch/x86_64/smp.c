#include "kernel.h"
#include "limine.h"
#include "list.h"
#include "stdio.h"
#include "kern/arch.h"
#include "sys/slab.h"
#include "x86_64.h"

#define MAX_CPUS 32

static struct limine_smp_request smpinfo = {
  .id = LIMINE_SMP_REQUEST,
};

struct per_cpu cpus[MAX_CPUS];

void
ap_entry (struct limine_smp_info *info)
{
  per_cpu_t *cpu = &cpus[info->processor_id];
  cpu->self = cpu;

  init_ap_idt ();
  init_ap_gdt (cpu);

  init_int_stacks ();

  printf ("AP started\n");

  halt_forever ();
}

void
init_aps ()
{
  struct limine_smp_response *resp = volatile_read (smpinfo.response);

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
