#include "kernel.h"
#include "stdatomic.h"
#include "string.h"
#include "sys/cdefs.h"
#include "sys/mem.h"
#include "arm64.h"

typedef uint64_t pte_t;

#define PTE_PRESENT (1 << 0)    // Valid bit
#define PTE_WRITE (1 << 10)     // Writeable (AP[2:1] = 00 for writeable)
#define PTE_USER (1 << 6)       // User access (AP[1] = 0 for user)
#define PTE_PWT (0)             // Write-through (not directly applicable)
#define PTE_PCD (0)             // Cache-disable (handled differently)
#define PTE_ACCESSED (1 << 10)  // Access flag
#define PTE_DIRTY (1 << 51)     // Dirty bit
#define PTE_PSE (0)             // Page size (handled differently in ARM64)
#define PTE_GLOBAL (0)          // Global (handled differently in ARM64)
#define PTE_PAT (0)             // Page attribute table (handled differently)
#define PTE_NX (1ULL << 54)     // Execute-never bit


#define PTE_ADDR(pte) ((pte) & 0x000FFFFFFFFFF000)
#define PTE(pte) (pte_t *)(direct_map_of (PTE_ADDR (pte)))

#define L4_SHIFT 39
#define L4_MASK 0x1FF

#define L3_SHIFT 30
#define L3_MASK 0x1FF

#define L2_SHIFT 21
#define L2_MASK 0x1FF

#define L1_SHIFT 12
#define L1_MASK 0x1FF

#define PAGE_SIZE 4096

extern uintptr_t KERNEL_END;
static _Atomic uintptr_t vm_alloc_base = (uintptr_t)&KERNEL_END;

uintptr_t get_vm_root()
{
    uintptr_t root;
    asm volatile ("mrs %0, ttbr0_el1" : "=r" (root));
    return root;
}

void set_vm_root(uintptr_t root)
{
    asm volatile ("msr ttbr0_el1, %0" : : "r" (root));
    asm volatile ("isb"); // Instruction Synchronization Barrier
}

uintptr_t
alloc_table ()
{
  uintptr_t page = alloc_page ();
  void *table = (void *)direct_map_of (page);
  memset (table, 0, PAGE_SIZE);
  return page;
}

void add_vm_mapping(uintptr_t root, uintptr_t virt, uintptr_t phys, int flags)
{
    pte_t *l4 = PTE(root);
    pte_t *l3;
    pte_t *l2;
    pte_t *l1;
    int table_flags = PTE_PRESENT | PTE_WRITE | (flags & PTE_USER);

    pte_t l4e = l4[(virt >> L4_SHIFT) & L4_MASK];
    if (!(l4e & PTE_PRESENT))
    {
        uintptr_t l3_page = alloc_table();
        l4[(virt >> L4_SHIFT) & L4_MASK] = l3_page | table_flags;
        l3 = PTE(l3_page);
    }
    else
    {
        l3 = PTE(l4e);
    }

    pte_t l3e = l3[(virt >> L3_SHIFT) & L3_MASK];
    if (!(l3e & PTE_PRESENT))
    {
        uintptr_t l2_page = alloc_table();
        l3[(virt >> L3_SHIFT) & L3_MASK] = l2_page | table_flags;
        l2 = PTE(l2_page);
    }
    else
    {
        l2 = PTE(l3e);
    }

    pte_t l2e = l2[(virt >> L2_SHIFT) & L2_MASK];
    if (!(l2e & PTE_PRESENT))
    {
        uintptr_t l1_page = alloc_table();
        l2[(virt >> L2_SHIFT) & L2_MASK] = l1_page | table_flags;
        l1 = PTE(l1_page);
    }
    else
    {
        l1 = PTE(l2e);
    }

    l1 = &l1[(virt >> L1_SHIFT) & L1_MASK];
    *l1 = phys | flags;
}

void
add_vm_mapping (uintptr_t root, uintptr_t virt, uintptr_t phys, int flags)
{
  pte_t *pml4 = PTE (root);
  pte_t *pdp;
  pte_t *pd;
  pte_t *pt;
  pte_t *pte;
  int table_flags = PTE_PRESENT | PTE_WRITE | (flags & PTE_USER);

  pte_t pml4e = pml4[virt >> PML4_SHIFT & PML4_MASK];
  if (!(pml4e & PTE_PRESENT))
    {
      uintptr_t pdp_page = alloc_table ();
      pml4[virt >> PML4_SHIFT & PML4_MASK] = pdp_page | table_flags;
      pdp = PTE (pdp_page);
    }
  else
    pdp = PTE (pml4e);

  pte_t pdpe = pdp[(virt >> PDP_SHIFT) & PDP_MASK];
  if (!(pdpe & PTE_PRESENT))
    {
      uintptr_t pd_page = alloc_table ();
      pdp[(virt >> PDP_SHIFT) & PDP_MASK] = pd_page | table_flags;
      pd = PTE (pd_page);
    }
  else
    pd = PTE (pdpe);

  pte_t pde = pd[(virt >> PD_SHIFT) & PD_MASK];
  if (!(pde & PTE_PRESENT))
    {
      uintptr_t pt_page = alloc_table ();
      pd[(virt >> PD_SHIFT) & PD_MASK] = (uintptr_t)pt_page | table_flags;
      pt = PTE (pt_page);
    }
  else
    pt = PTE (pde);

  pte = &pt[(virt >> PT_SHIFT) & PT_MASK];
  *pte = phys | flags;
}

uintptr_t
resolve_vm_mapping (uintptr_t root, uintptr_t virt)
{
  pte_t *pml4 = PTE (root);
  pte_t *pdp;
  pte_t *pd;
  pte_t *pt;
  pte_t *pte;

  pte_t pml4e = pml4[virt >> PML4_SHIFT & PML4_MASK];
  if (!(pml4e & PTE_PRESENT))
    return 0;

  pdp = PTE (pml4e);

  pte_t pdpe = pdp[(virt >> PDP_SHIFT) & PDP_MASK];
  if (!(pdpe & PTE_PRESENT))
    return 0;

  pd = PTE (pdpe);

  pte_t pde = pd[(virt >> PD_SHIFT) & PD_MASK];
  if (!(pde & PTE_PRESENT))
    return 0;

  pt = PTE (pde);

  pte = &pt[(virt >> PT_SHIFT) & PT_MASK];
  if (!(*pte & PTE_PRESENT))
    return 0;

  return PTE_ADDR (*pte) | (virt & 0xFFF);
}

uintptr_t
alloc_kernel_vm (size_t len)
{
  len = ALIGN_UP (len, PAGE_SIZE);
  return atomic_fetch_add (&vm_alloc_base, len);
}

uintptr_t
new_page_table ()
{
  uintptr_t root = alloc_table ();
  pte_t *pml4 = PTE (root);

  pte_t *current_root = PTE (get_vm_root ());

  // Copy all higher-half (kernel) mappings
  for (int i = 256; i < 512; i++)
    pml4[i] = current_root[i];

  return root;
}

void
destroy_page_table (uintptr_t)
{
  // todo
}
