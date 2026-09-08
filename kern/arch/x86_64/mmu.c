#include "kern/mem.h"
#include "stdatomic.h"
#include "string.h"
#include "sys/cdefs.h"
#include "x86_64.h"

#define PTE_ADDR(pte) ((pte) & 0x000FFFFFFFFFF000)
#define PTE(pte) (pte_t *)(direct_map_of (PTE_ADDR (pte)))

static constexpr unsigned PML4_SHIFT = 39;
static constexpr uintptr_t PML4_MASK = 0x1FF;
static constexpr unsigned PDP_SHIFT = 30;
static constexpr uintptr_t PDP_MASK = 0x1FF;
static constexpr unsigned PD_SHIFT = 21;
static constexpr uintptr_t PD_MASK = 0x1FF;
static constexpr unsigned PT_SHIFT = 12;
static constexpr uintptr_t PT_MASK = 0x1FF;

extern uintptr_t KERNEL_END;

uintptr_t
get_vm_root ()
{
  uintptr_t root;
  asm volatile ("mov %%cr3, %0" : "=r"(root));
  return root;
}

void
set_vm_root (uintptr_t root)
{
  asm volatile ("mov %0, %%cr3" : : "r"(root));
}

static uintptr_t
alloc_table ()
{
  const uintptr_t page = alloc_page ();
  void *table = (void *)direct_map_of (page);
  memset (table, 0, PAGE_SIZE);
  return page;
}

pte_t *
get_pml4e (uintptr_t root, uintptr_t addr)
{
  pte_t *pml4 = PTE (root);
  return &pml4[addr >> PML4_SHIFT & PML4_MASK];
}

pte_t *
get_pdpte (uintptr_t root, uintptr_t addr)
{
  pte_t *pdpt = PTE (*get_pml4e (root, addr));
  return &pdpt[(addr >> PDP_SHIFT) & PDP_MASK];
}

pte_t *
get_pde (uintptr_t root, uintptr_t addr)
{
  pte_t *pd = PTE (*get_pdpte (root, addr));
  return &pd[(addr >> PD_SHIFT) & PD_MASK];
}

pte_t *
get_pte (uintptr_t root, uintptr_t addr)
{
  pte_t *pt = PTE (*get_pde (root, addr));
  return &pt[(addr >> PT_SHIFT) & PT_MASK];
}

void
add_vm_mapping (uintptr_t root, uintptr_t addr, uintptr_t phys,
                uintptr_t flags)
{
  pte_t *pml4 = PTE (root);
  pte_t *pdp, *pd, *pt, *pte;
  uintptr_t table_flags = PTE_PRESENT | PTE_WRITE | (flags & PTE_USER);

  pte_t pml4e = pml4[addr >> PML4_SHIFT & PML4_MASK];
  if (!(pml4e & PTE_PRESENT))
    {
      uintptr_t pdp_page = alloc_table ();
      pml4[addr >> PML4_SHIFT & PML4_MASK] = pdp_page | table_flags;
      pdp = PTE (pdp_page);
    }
  else
    pdp = PTE (pml4e);

  pte_t pdpe = pdp[(addr >> PDP_SHIFT) & PDP_MASK];
  if (!(pdpe & PTE_PRESENT))
    {
      uintptr_t pd_page = alloc_table ();
      pdp[(addr >> PDP_SHIFT) & PDP_MASK] = pd_page | table_flags;
      pd = PTE (pd_page);
    }
  else
    pd = PTE (pdpe);

  pte_t pde = pd[(addr >> PD_SHIFT) & PD_MASK];
  if (!(pde & PTE_PRESENT))
    {
      uintptr_t pt_page = alloc_table ();
      pd[(addr >> PD_SHIFT) & PD_MASK] = pt_page | table_flags;
      pt = PTE (pt_page);
    }
  else
    pt = PTE (pde);

  pte = &pt[(addr >> PT_SHIFT) & PT_MASK];
  *pte = phys | flags;

  asm volatile ("invlpg (%0)" ::"r"(addr) : "memory");
}

uintptr_t
resolve_vm_mapping (uintptr_t root, uintptr_t virt)
{
  const pte_t *pml4 = PTE (root);

  const pte_t pml4e = pml4[virt >> PML4_SHIFT & PML4_MASK];
  if (!(pml4e & PTE_PRESENT))
    return 0;

  const pte_t *pdp = PTE (pml4e);

  const pte_t pdpe = pdp[(virt >> PDP_SHIFT) & PDP_MASK];
  if (!(pdpe & PTE_PRESENT))
    return 0;

  const pte_t *pd = PTE (pdpe);

  const pte_t pde = pd[(virt >> PD_SHIFT) & PD_MASK];
  if (!(pde & PTE_PRESENT))
    return 0;

  const pte_t *pt = PTE (pde);

  const pte_t *pte = &pt[(virt >> PT_SHIFT) & PT_MASK];
  if (!(*pte & PTE_PRESENT))
    return 0;

  return PTE_ADDR (*pte) | (virt & 0xFFF);
}

void
destroy_page_table (uintptr_t)
{
  // todo
}
