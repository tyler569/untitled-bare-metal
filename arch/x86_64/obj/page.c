#include "../x86_64.h"
#include "kern/cap.h"
#include "kern/mem.h"
#include "kern/syscall.h"

#define PML4E_SHIFT 39
#define PDPTE_SHIFT 30
#define PDE_SHIFT 21
#define PTE_SHIFT 12

#define MASK(n) ((1UL << (n)) - 1)

/*
 * TODO: unmap cannot be done without ASID - we need to be able to get from
 * any page or page table cap back to _both_ the vspace it is a part of and
 * its mapped address within that vspace. We can pack both of these in the
 * badge field but only if we have a smaller representation of the vspace
 * and that is the ASID.
 */

static message_info_t
already_mapped ()
{
  return msg_delete_first ();
}

static message_info_t
table_missing (int level)
{
  return msg_failed_lookup (level);
}

message_info_t
x86_64_pdpt_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  (void)attr;

  vaddr &= ~MASK (PML4E_SHIFT);

  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t pdpt_phy = physical_of ((uintptr_t)cap_ptr (cte));

  pte_t *pml4e = get_pml4e (root_phy, vaddr);

  // ensure kernel mappings are present in the new vspace
  {
    uintptr_t current_vm_root = get_vm_root ();
    pte_t *current_pml4 = get_pml4e (current_vm_root, 0);
    for (int i = 256; i < 512; i++)
      pml4e[i] = current_pml4[i];
  }

  if ((*pml4e & PTE_PRESENT) != 0)
    return already_mapped ();

  *pml4e = pdpt_phy | PTE_PRESENT | PTE_WRITE | PTE_USER;

  return msg_ok (0);
}

message_info_t
x86_64_pd_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  (void)attr;

  vaddr &= ~MASK (PDPTE_SHIFT);

  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t pd_phy = physical_of ((uintptr_t)cap_ptr (cte));

  pte_t *pml4e = get_pml4e (root_phy, vaddr);
  if ((*pml4e & PTE_PRESENT) == 0)
    return table_missing (3);

  pte_t *pdpte = get_pdpte (root_phy, vaddr);
  if ((*pdpte & PTE_PRESENT) != 0)
    return already_mapped ();

  *pdpte = pd_phy | PTE_PRESENT | PTE_WRITE | PTE_USER;

  return msg_ok (0);
}

message_info_t
x86_64_pt_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  (void)attr;

  vaddr &= ~MASK (PDE_SHIFT);

  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t pt_phy = physical_of ((uintptr_t)cap_ptr (cte));

  pte_t *pml4e = get_pml4e (root_phy, vaddr);
  if ((*pml4e & PTE_PRESENT) == 0)
    return table_missing (3);

  pte_t *pdpte = get_pdpte (root_phy, vaddr);
  if ((*pdpte & PTE_PRESENT) == 0)
    return table_missing (2);

  pte_t *pde = get_pde (root_phy, vaddr);
  if ((*pde & PTE_PRESENT) != 0)
    return already_mapped ();

  *pde = pt_phy | PTE_PRESENT | PTE_WRITE | PTE_USER;

  return msg_ok (0);
}

message_info_t
x86_64_page_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t page_phy = physical_of ((uintptr_t)cap_ptr (cte));

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

  return msg_ok (0);
}

message_info_t
x86_64_huge_page_map (cte_t *cte, cte_t *vspace, word_t vaddr, word_t attr)
{
  uintptr_t root_phy = (uintptr_t)cap_ptr (vspace);
  uintptr_t page_phy = physical_of ((uintptr_t)cap_ptr (cte));

  pte_t *pml4e = get_pml4e (root_phy, vaddr);
  if ((*pml4e & PTE_PRESENT) == 0)
    return table_missing (3);

  pte_t *pdpte = get_pdpte (root_phy, vaddr);
  if ((*pdpte & PTE_PRESENT) == 0)
    return table_missing (2);

  pte_t *pde = get_pde (root_phy, vaddr);
  if ((*pde & PTE_PRESENT) != 0)
    return already_mapped ();

  *pde = page_phy | PTE_PRESENT | PTE_USER | attr;

  return msg_ok (0);
}
