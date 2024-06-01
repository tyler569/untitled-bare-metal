// This file is autogenerated by './gen.rb'. Do not edit.

#pragma once

#include "sys/syscall.h"

error_t cnode_copy (cap_t obj, word_t dest_index, uint8_t dest_depth,
                    cap_t src_root, word_t src_index, uint8_t src_depth,
                    cap_rights_t rights);
error_t cnode_delete (cap_t obj, word_t index, uint8_t depth);
error_t cnode_mint (cap_t obj, word_t dest_index, uint8_t dest_depth,
                    cap_t src_root, word_t src_index, uint8_t src_depth,
                    cap_rights_t rights, word_t badge);
error_t cnode_revoke (cap_t obj, word_t index, uint8_t depth);
error_t cnode_debug_print (cap_t obj);
error_t tcb_echo (cap_t obj);
error_t tcb_configure (cap_t obj, word_t fault_ep, cap_t cspace_root,
                       word_t cspace_root_data, cap_t vspace_root,
                       word_t vspace_root_data, word_t buffer,
                       cap_t buffer_frame);
error_t tcb_copy_registers (cap_t obj, cap_t source, bool suspend_source,
                            bool resume_target, bool transfer_frame,
                            bool transfer_integer, word_t arch_flags);
error_t tcb_read_registers (cap_t obj, bool suspend_source, word_t arch_flags,
                            word_t count, user_context_t *regs);
error_t tcb_resume (cap_t obj);
error_t tcb_set_affinity (cap_t obj, word_t affinity);
error_t tcb_set_ipc_buffer (cap_t obj, word_t buffer, cap_t buffer_frame);
error_t tcb_set_space (cap_t obj, word_t fault_ep, cap_t cspace_root,
                       word_t cspace_root_data, cap_t vspace_root,
                       word_t vspace_root_data);
error_t tcb_set_tls_base (cap_t obj, word_t tls_base);
error_t tcb_suspend (cap_t obj);
error_t tcb_write_registers (cap_t obj, bool resume_target, word_t arch_flags,
                             word_t count, user_context_t *regs);
error_t untyped_retype (cap_t obj, word_t type, word_t size_bits, cap_t root,
                        word_t node_index, uint8_t node_depth,
                        word_t node_offset, word_t num_objects);
error_t x86_io_port_in8 (cap_t obj, word_t port, word_t result);
error_t x86_io_port_in16 (cap_t obj, word_t port, word_t result);
error_t x86_io_port_in32 (cap_t obj, word_t port, word_t result);
error_t x86_io_port_out8 (cap_t obj, word_t port, word_t value);
error_t x86_io_port_out16 (cap_t obj, word_t port, word_t value);
error_t x86_io_port_out32 (cap_t obj, word_t port, word_t value);
error_t x86_64_pdpt_map (cap_t obj, cap_t vspace, word_t vaddr,
                         x86_vm_attributes_t attr);
error_t x86_64_pdpt_unmap (cap_t obj);
error_t x86_64_pd_map (cap_t obj, cap_t vspace, word_t vaddr,
                       x86_vm_attributes_t attr);
error_t x86_64_pd_unmap (cap_t obj);
error_t x86_64_pt_map (cap_t obj, cap_t vspace, word_t vaddr,
                       x86_vm_attributes_t attr);
error_t x86_64_pt_unmap (cap_t obj);
error_t x86_64_page_map (cap_t obj, cap_t vspace, word_t vaddr,
                         x86_vm_attributes_t attr);
error_t x86_64_page_unmap (cap_t obj);
