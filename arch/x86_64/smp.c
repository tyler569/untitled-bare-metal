#include "limine.h"
#include "x86_64.h"
#include <stddef.h>

static volatile struct limine_smp_request limine_smp_request = {
  .id = LIMINE_SMP_REQUEST,
};

void ap_entry (struct limine_smp_info *);

void
init_aps ()
{
  struct limine_smp_response *resp = limine_smp_request.response;

  for (size_t i = 0; i < resp->cpu_count; i++)
    {
      if (resp->cpus[i]->lapic_id == resp->bsp_lapic_id)
        continue;

      resp->cpus[i]->goto_address = ap_entry;
    }
}