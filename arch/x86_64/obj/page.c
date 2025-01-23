#include "../x86_64.h"
#include "kern/cap.h"
#include "kern/syscall.h"

#define PML4E_SHIFT 39
#define PDPTE_SHIFT 30
#define PDE_SHIFT 21
#define PTE_SHIFT 12

#define PML4E_MASK 0x1FF
#define PDPTE_MASK 0x1FF
#define PDE_MASK 0x1FF
#define PTE_MASK 0x1FF

static message_info_t
already_mapped ()
{
  return return_ipc (delete_first, 0);
}

static message_info_t
table_missing (int level)
{
  set_mr (0, level);
  return return_ipc (failed_lookup, 1);
}

message_info_t
x86_64_pdpt_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  (void)attr;

  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t pdpt_phy = (uintptr_t)cap_ptr (cte);

  pte_t *pml4e = get_pml4e (root_phy, vaddr);

  if ((*pml4e & PTE_PRESENT) != 0)
    return already_mapped ();

  *pml4e = pdpt_phy | PTE_PRESENT | PTE_WRITE | PTE_USER;

  return return_ipc (no_error, 0);
}

message_info_t
x86_64_pd_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  (void)attr;

  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t pd_phy = (uintptr_t)cap_ptr (cte);

  pte_t *pml4e = get_pml4e (root_phy, vaddr);
  if ((*pml4e & PTE_PRESENT) == 0)
    return table_missing (4);

  pte_t *pdpte = get_pdpte (root_phy, vaddr);
  if ((*pdpte & PTE_PRESENT) != 0)
    return already_mapped ();

  *pdpte = pd_phy | PTE_PRESENT | PTE_WRITE | PTE_USER;

  return return_ipc (no_error, 0);
}

message_info_t
x86_64_pt_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  (void)attr;

  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t pt_phy = (uintptr_t)cap_ptr (cte);

  pte_t *pml4e = get_pml4e (root_phy, vaddr);
  if ((*pml4e & PTE_PRESENT) == 0)
    return table_missing (4);

  pte_t *pdpte = get_pdpte (root_phy, vaddr);
  if ((*pdpte & PTE_PRESENT) == 0)
    return table_missing (3);

  pte_t *pde = get_pde (root_phy, vaddr);
  if ((*pde & PTE_PRESENT) != 0)
    return already_mapped ();

  *pde = pt_phy | PTE_PRESENT | PTE_WRITE | PTE_USER;

  return return_ipc (no_error, 0);
}

message_info_t
x86_64_page_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t page_phy = (uintptr_t)cap_ptr (cte);

  pte_t *pml4e = get_pml4e (root_phy, vaddr);
  if ((*pml4e & PTE_PRESENT) == 0)
    return table_missing (3);

  pte_t *pdpte = get_pdpte (root_phy, vaddr);
  if ((*pdpte & PTE_PRESENT) == 0)
    return table_missing (2);

  pte_t *pde = get_pde (root_phy, vaddr);
  if ((*pde & PTE_PRESENT) == 0)
    return table_missing (1);

  pte_t *pte = get_pte (root_phy, vaddr);
  if ((*pte & PTE_PRESENT) != 0)
    return already_mapped ();

  *pte = page_phy | PTE_PRESENT | PTE_USER | attr;

  return return_ipc (no_error, 0);
}
