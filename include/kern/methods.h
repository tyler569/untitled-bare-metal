// This file is autogenerated by './gen.rb'. Do not edit.

#pragma once

#include "kern/cap.h"
#include "sys/syscall.h"

error_t cnode_copy (cte_t *slot, word_t dest_index, uint8_t dest_depth,
                    cte_t *src_root, word_t src_index, uint8_t src_depth,
                    cap_rights_t rights);
error_t cnode_delete (cte_t *slot, word_t index, uint8_t depth);
error_t cnode_mint (cte_t *slot, word_t dest_index, uint8_t dest_depth,
                    cte_t *src_root, word_t src_index, uint8_t src_depth,
                    cap_rights_t rights, word_t badge);
error_t cnode_revoke (cte_t *slot, word_t index, uint8_t depth);
error_t cnode_debug_print (cte_t *slot);
error_t tcb_echo (cte_t *slot);
error_t tcb_configure (cte_t *slot, word_t fault_ep, cte_t *cspace_root,
                       word_t cspace_root_data, cte_t *vspace_root,
                       word_t vspace_root_data, word_t buffer,
                       cte_t *buffer_frame);
error_t tcb_copy_registers (cte_t *slot, cte_t *source, bool suspend_source,
                            bool resume_target, bool transfer_frame,
                            bool transfer_integer, word_t arch_flags);
error_t tcb_read_registers (cte_t *slot, bool suspend_source,
                            word_t arch_flags, word_t count,
                            user_context_t *regs);
error_t tcb_resume (cte_t *slot);
error_t tcb_set_affinity (cte_t *slot, word_t affinity);
error_t tcb_set_ipc_buffer (cte_t *slot, word_t buffer, cte_t *buffer_frame);
error_t tcb_set_space (cte_t *slot, word_t fault_ep, cte_t *cspace_root,
                       word_t cspace_root_data, cte_t *vspace_root,
                       word_t vspace_root_data);
error_t tcb_set_tls_base (cte_t *slot, word_t tls_base);
error_t tcb_suspend (cte_t *slot);
error_t tcb_write_registers (cte_t *slot, bool resume_target,
                             word_t arch_flags, word_t count,
                             user_context_t *regs);
error_t untyped_retype (cte_t *slot, word_t type, word_t size_bits,
                        cte_t *root, word_t node_index, uint8_t node_depth,
                        word_t node_offset, word_t num_objects);
error_t x86_64_io_port_in8 (cte_t *slot, word_t port, word_t result);
error_t x86_64_io_port_in16 (cte_t *slot, word_t port, word_t result);
error_t x86_64_io_port_in32 (cte_t *slot, word_t port, word_t result);
error_t x86_64_io_port_out8 (cte_t *slot, word_t port, word_t value);
error_t x86_64_io_port_out16 (cte_t *slot, word_t port, word_t value);
error_t x86_64_io_port_out32 (cte_t *slot, word_t port, word_t value);
error_t x86_64_io_port_control_issue (cte_t *slot, word_t first_port,
                                      word_t last_port, word_t node_index,
                                      uint8_t node_depth, word_t node_offset);
error_t x86_64_pdpt_map (cte_t *slot, cte_t *vspace, word_t vaddr,
                         x86_vm_attributes_t attr);
error_t x86_64_pdpt_unmap (cte_t *slot);
error_t x86_64_pd_map (cte_t *slot, cte_t *vspace, word_t vaddr,
                       x86_vm_attributes_t attr);
error_t x86_64_pd_unmap (cte_t *slot);
error_t x86_64_pt_map (cte_t *slot, cte_t *vspace, word_t vaddr,
                       x86_vm_attributes_t attr);
error_t x86_64_pt_unmap (cte_t *slot);
error_t x86_64_page_map (cte_t *slot, cte_t *vspace, word_t vaddr,
                         x86_vm_attributes_t attr);
error_t x86_64_page_unmap (cte_t *slot);
