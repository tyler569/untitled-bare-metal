// This file is autogenerated by './gen.rb'. Do not edit.

#include "kern/methods.h"
#include "sys/syscall.h"

__attribute__ ((weak)) message_info_t
cnode_copy (cte_t *, word_t, uint8_t, cte_t *, word_t, uint8_t, cap_rights_t)
{
  err_printf ("unimplemented kernel method cnode_copy\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
cnode_delete (cte_t *, word_t, uint8_t)
{
  err_printf ("unimplemented kernel method cnode_delete\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
cnode_mint (cte_t *, word_t, uint8_t, cte_t *, word_t, uint8_t, cap_rights_t,
            word_t)
{
  err_printf ("unimplemented kernel method cnode_mint\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
cnode_revoke (cte_t *, word_t, uint8_t)
{
  err_printf ("unimplemented kernel method cnode_revoke\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
cnode_debug_print (cte_t *)
{
  err_printf ("unimplemented kernel method cnode_debug_print\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_configure (cte_t *, word_t, cte_t *, word_t, cte_t *, word_t, word_t,
               cte_t *)
{
  err_printf ("unimplemented kernel method tcb_configure\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_copy_registers (cte_t *, cte_t *, bool, bool, bool, bool, word_t)
{
  err_printf ("unimplemented kernel method tcb_copy_registers\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_read_registers (cte_t *, bool, word_t, word_t, user_context_t *)
{
  err_printf ("unimplemented kernel method tcb_read_registers\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_resume (cte_t *)
{
  err_printf ("unimplemented kernel method tcb_resume\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_bind_notification (cte_t *, cte_t *)
{
  err_printf ("unimplemented kernel method tcb_bind_notification\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_set_affinity (cte_t *, word_t)
{
  err_printf ("unimplemented kernel method tcb_set_affinity\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_set_ipc_buffer (cte_t *, word_t, cte_t *)
{
  err_printf ("unimplemented kernel method tcb_set_ipc_buffer\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_set_space (cte_t *, word_t, cte_t *, word_t, cte_t *, word_t)
{
  err_printf ("unimplemented kernel method tcb_set_space\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_set_tls_base (cte_t *, word_t)
{
  err_printf ("unimplemented kernel method tcb_set_tls_base\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_suspend (cte_t *)
{
  err_printf ("unimplemented kernel method tcb_suspend\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
tcb_write_registers (cte_t *, bool, word_t, word_t, user_context_t *)
{
  err_printf ("unimplemented kernel method tcb_write_registers\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
untyped_retype (cte_t *, word_t, word_t, cte_t *, word_t, uint8_t, word_t,
                word_t)
{
  err_printf ("unimplemented kernel method untyped_retype\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_in8 (cte_t *, word_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_in8\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_in16 (cte_t *, word_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_in16\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_in32 (cte_t *, word_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_in32\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_out8 (cte_t *, word_t, word_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_out8\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_out16 (cte_t *, word_t, word_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_out16\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_out32 (cte_t *, word_t, word_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_out32\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_io_port_control_issue (cte_t *, word_t, word_t, cte_t *, word_t,
                              uint8_t)
{
  err_printf ("unimplemented kernel method x86_64_io_port_control_issue\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_pdpt_map (cte_t *, cte_t *, word_t, x86_vm_attributes_t)
{
  err_printf ("unimplemented kernel method x86_64_pdpt_map\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_pdpt_unmap (cte_t *)
{
  err_printf ("unimplemented kernel method x86_64_pdpt_unmap\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_pd_map (cte_t *, cte_t *, word_t, x86_vm_attributes_t)
{
  err_printf ("unimplemented kernel method x86_64_pd_map\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_pd_unmap (cte_t *)
{
  err_printf ("unimplemented kernel method x86_64_pd_unmap\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_pt_map (cte_t *, cte_t *, word_t, x86_vm_attributes_t)
{
  err_printf ("unimplemented kernel method x86_64_pt_map\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_pt_unmap (cte_t *)
{
  err_printf ("unimplemented kernel method x86_64_pt_unmap\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_page_map (cte_t *, cte_t *, word_t, x86_vm_attributes_t)
{
  err_printf ("unimplemented kernel method x86_64_page_map\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_page_unmap (cte_t *)
{
  err_printf ("unimplemented kernel method x86_64_page_unmap\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_huge_page_map (cte_t *, cte_t *, word_t, x86_vm_attributes_t)
{
  err_printf ("unimplemented kernel method x86_64_huge_page_map\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
x86_64_huge_page_unmap (cte_t *)
{
  err_printf ("unimplemented kernel method x86_64_huge_page_unmap\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
irq_control_get (cte_t *, word_t, cte_t *, word_t, uint8_t)
{
  err_printf ("unimplemented kernel method irq_control_get\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
irq_handler_ack (cte_t *)
{
  err_printf ("unimplemented kernel method irq_handler_ack\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
irq_handler_clear (cte_t *)
{
  err_printf ("unimplemented kernel method irq_handler_clear\n");
  return illegal_operation;
}
__attribute__ ((weak)) message_info_t
irq_handler_set_notification (cte_t *, cte_t *)
{
  err_printf ("unimplemented kernel method irq_handler_set_notification\n");
  return illegal_operation;
}
