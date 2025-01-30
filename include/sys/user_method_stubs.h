// This file is autogenerated by './gen.rb'. Do not edit.

#pragma once

#include "sys/syscall.h"

static inline word_t get_mr (word_t index);
static inline void set_mr (word_t index, word_t value);
static inline cptr_t get_cap (word_t index);
static inline void set_cap (word_t index, cptr_t value);

static inline int
cnode_copy (cptr_t obj, word_t dest_index, uint8_t dest_depth, cptr_t src_root,
            word_t src_index, uint8_t src_depth, cap_rights_t rights)
{
  set_mr (0, (word_t)dest_index);
  set_mr (1, (word_t)dest_depth);
  set_mr (2, (word_t)src_index);
  set_mr (3, (word_t)src_depth);
  set_mr (4, (word_t)rights);
  set_cap (0, src_root);
  message_info_t _info = new_message_info (METHOD_cnode_copy, 0, 1, 5);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
cnode_delete (cptr_t obj, word_t index, uint8_t depth)
{
  set_mr (0, (word_t)index);
  set_mr (1, (word_t)depth);

  message_info_t _info = new_message_info (METHOD_cnode_delete, 0, 0, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
cnode_mint (cptr_t obj, word_t dest_index, uint8_t dest_depth, cptr_t src_root,
            word_t src_index, uint8_t src_depth, cap_rights_t rights,
            word_t badge)
{
  set_mr (0, (word_t)dest_index);
  set_mr (1, (word_t)dest_depth);
  set_mr (2, (word_t)src_index);
  set_mr (3, (word_t)src_depth);
  set_mr (4, (word_t)rights);
  set_mr (5, (word_t)badge);
  set_cap (0, src_root);
  message_info_t _info = new_message_info (METHOD_cnode_mint, 0, 1, 6);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
cnode_revoke (cptr_t obj, word_t index, uint8_t depth)
{
  set_mr (0, (word_t)index);
  set_mr (1, (word_t)depth);

  message_info_t _info = new_message_info (METHOD_cnode_revoke, 0, 0, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
cnode_debug_print (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_cnode_debug_print, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_configure (cptr_t obj, word_t fault_ep, cptr_t cspace_root,
               word_t cspace_root_data, cptr_t vspace_root,
               word_t vspace_root_data, word_t buffer, cptr_t buffer_frame)
{
  set_mr (0, (word_t)fault_ep);
  set_mr (1, (word_t)cspace_root_data);
  set_mr (2, (word_t)vspace_root_data);
  set_mr (3, (word_t)buffer);
  set_cap (0, cspace_root);
  set_cap (1, vspace_root);
  set_cap (2, buffer_frame);
  message_info_t _info = new_message_info (METHOD_tcb_configure, 0, 3, 4);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_copy_registers (cptr_t obj, cptr_t source, bool suspend_source,
                    bool resume_target, bool transfer_frame,
                    bool transfer_integer, word_t arch_flags)
{
  set_mr (0, (word_t)suspend_source);
  set_mr (1, (word_t)resume_target);
  set_mr (2, (word_t)transfer_frame);
  set_mr (3, (word_t)transfer_integer);
  set_mr (4, (word_t)arch_flags);
  set_cap (0, source);
  message_info_t _info = new_message_info (METHOD_tcb_copy_registers, 0, 1, 5);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_read_registers (cptr_t obj, bool suspend_source, word_t arch_flags,
                    word_t count, user_context_t *regs)
{
  set_mr (0, (word_t)suspend_source);
  set_mr (1, (word_t)arch_flags);
  set_mr (2, (word_t)count);
  set_mr (3, (word_t)regs);

  message_info_t _info = new_message_info (METHOD_tcb_read_registers, 0, 0, 4);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_resume (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_tcb_resume, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_bind_notification (cptr_t obj, cptr_t notification)
{

  set_cap (0, notification);
  message_info_t _info
      = new_message_info (METHOD_tcb_bind_notification, 0, 1, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_set_affinity (cptr_t obj, word_t affinity)
{
  set_mr (0, (word_t)affinity);

  message_info_t _info = new_message_info (METHOD_tcb_set_affinity, 0, 0, 1);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_set_ipc_buffer (cptr_t obj, word_t buffer, cptr_t buffer_frame)
{
  set_mr (0, (word_t)buffer);
  set_cap (0, buffer_frame);
  message_info_t _info = new_message_info (METHOD_tcb_set_ipc_buffer, 0, 1, 1);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_set_space (cptr_t obj, word_t fault_ep, cptr_t cspace_root,
               word_t cspace_root_data, cptr_t vspace_root,
               word_t vspace_root_data)
{
  set_mr (0, (word_t)fault_ep);
  set_mr (1, (word_t)cspace_root_data);
  set_mr (2, (word_t)vspace_root_data);
  set_cap (0, cspace_root);
  set_cap (1, vspace_root);
  message_info_t _info = new_message_info (METHOD_tcb_set_space, 0, 2, 3);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_set_tls_base (cptr_t obj, word_t tls_base)
{
  set_mr (0, (word_t)tls_base);

  message_info_t _info = new_message_info (METHOD_tcb_set_tls_base, 0, 0, 1);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_suspend (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_tcb_suspend, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
tcb_write_registers (cptr_t obj, bool resume_target, word_t arch_flags,
                     word_t count, user_context_t *regs)
{
  set_mr (0, (word_t)resume_target);
  set_mr (1, (word_t)arch_flags);
  set_mr (2, (word_t)count);
  set_mr (3, (word_t)regs);

  message_info_t _info
      = new_message_info (METHOD_tcb_write_registers, 0, 0, 4);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
untyped_retype (cptr_t obj, word_t type, word_t size_bits, cptr_t root,
                word_t node_index, uint8_t node_depth, word_t node_offset,
                word_t num_objects)
{
  set_mr (0, (word_t)type);
  set_mr (1, (word_t)size_bits);
  set_mr (2, (word_t)node_index);
  set_mr (3, (word_t)node_depth);
  set_mr (4, (word_t)node_offset);
  set_mr (5, (word_t)num_objects);
  set_cap (0, root);
  message_info_t _info = new_message_info (METHOD_untyped_retype, 0, 1, 6);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_in8 (cptr_t obj, word_t port)
{
  set_mr (0, (word_t)port);

  message_info_t _info = new_message_info (METHOD_x86_64_io_port_in8, 0, 0, 1);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_in16 (cptr_t obj, word_t port)
{
  set_mr (0, (word_t)port);

  message_info_t _info
      = new_message_info (METHOD_x86_64_io_port_in16, 0, 0, 1);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_in32 (cptr_t obj, word_t port)
{
  set_mr (0, (word_t)port);

  message_info_t _info
      = new_message_info (METHOD_x86_64_io_port_in32, 0, 0, 1);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_out8 (cptr_t obj, word_t port, word_t value)
{
  set_mr (0, (word_t)port);
  set_mr (1, (word_t)value);

  message_info_t _info
      = new_message_info (METHOD_x86_64_io_port_out8, 0, 0, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_out16 (cptr_t obj, word_t port, word_t value)
{
  set_mr (0, (word_t)port);
  set_mr (1, (word_t)value);

  message_info_t _info
      = new_message_info (METHOD_x86_64_io_port_out16, 0, 0, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_out32 (cptr_t obj, word_t port, word_t value)
{
  set_mr (0, (word_t)port);
  set_mr (1, (word_t)value);

  message_info_t _info
      = new_message_info (METHOD_x86_64_io_port_out32, 0, 0, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_io_port_control_issue (cptr_t obj, word_t first_port, word_t last_port,
                              cptr_t root, word_t index, uint8_t depth)
{
  set_mr (0, (word_t)first_port);
  set_mr (1, (word_t)last_port);
  set_mr (2, (word_t)index);
  set_mr (3, (word_t)depth);
  set_cap (0, root);
  message_info_t _info
      = new_message_info (METHOD_x86_64_io_port_control_issue, 0, 1, 4);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_pdpt_map (cptr_t obj, cptr_t vspace, word_t vaddr,
                 x86_vm_attributes_t attr)
{
  set_mr (0, (word_t)vaddr);
  set_mr (1, (word_t)attr);
  set_cap (0, vspace);
  message_info_t _info = new_message_info (METHOD_x86_64_pdpt_map, 0, 1, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_pdpt_unmap (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_x86_64_pdpt_unmap, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_pd_map (cptr_t obj, cptr_t vspace, word_t vaddr,
               x86_vm_attributes_t attr)
{
  set_mr (0, (word_t)vaddr);
  set_mr (1, (word_t)attr);
  set_cap (0, vspace);
  message_info_t _info = new_message_info (METHOD_x86_64_pd_map, 0, 1, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_pd_unmap (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_x86_64_pd_unmap, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_pt_map (cptr_t obj, cptr_t vspace, word_t vaddr,
               x86_vm_attributes_t attr)
{
  set_mr (0, (word_t)vaddr);
  set_mr (1, (word_t)attr);
  set_cap (0, vspace);
  message_info_t _info = new_message_info (METHOD_x86_64_pt_map, 0, 1, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_pt_unmap (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_x86_64_pt_unmap, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_page_map (cptr_t obj, cptr_t vspace, word_t vaddr,
                 x86_vm_attributes_t attr)
{
  set_mr (0, (word_t)vaddr);
  set_mr (1, (word_t)attr);
  set_cap (0, vspace);
  message_info_t _info = new_message_info (METHOD_x86_64_page_map, 0, 1, 2);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
x86_64_page_unmap (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_x86_64_page_unmap, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
irq_control_get (cptr_t obj, word_t irq, cptr_t root, word_t index,
                 uint8_t depth)
{
  set_mr (0, (word_t)irq);
  set_mr (1, (word_t)index);
  set_mr (2, (word_t)depth);
  set_cap (0, root);
  message_info_t _info = new_message_info (METHOD_irq_control_get, 0, 1, 3);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
irq_handler_ack (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_irq_handler_ack, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
irq_handler_clear (cptr_t obj)
{

  message_info_t _info = new_message_info (METHOD_irq_handler_clear, 0, 0, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
static inline int
irq_handler_set_notification (cptr_t obj, cptr_t notification)
{

  set_cap (0, notification);
  message_info_t _info
      = new_message_info (METHOD_irq_handler_set_notification, 0, 1, 0);
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}
