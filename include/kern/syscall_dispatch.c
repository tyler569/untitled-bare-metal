// This file is autogenerated by './gen.rb'. Do not edit.

error_t
dispatch_method (cap_t obj, message_info_t info)
{
  error_t error = no_error;
  switch (get_message_label (info))
    {
    case METHOD_cnode_copy:
      {
        word_t dest_index = (word_t)get_mr (0);
        uint8_t dest_depth = (uint8_t)get_mr (1);
        word_t src_index = (word_t)get_mr (2);
        uint8_t src_depth = (uint8_t)get_mr (3);
        cap_rights_t rights = (cap_rights_t)get_mr (4);
        cap_t src_root;
        if (cap_type (obj) != cap_cnode)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 5)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &src_root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf (
            "cnode_copy (cap:%s, dest_index=%#lx, dest_depth=%hhu, "
            "src_root=cap:%s, src_index=%#lx, src_depth=%hhu, rights=%#lx)\n",
            cap_type_string (obj.type), dest_index, dest_depth,
            cap_type_string (src_root.type), src_index, src_depth, rights);
        return cnode_copy (obj, dest_index, dest_depth, src_root, src_index,
                           src_depth, rights);
        break;
      }
    case METHOD_cnode_delete:
      {
        word_t index = (word_t)get_mr (0);
        uint8_t depth = (uint8_t)get_mr (1);

        if (cap_type (obj) != cap_cnode)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("cnode_delete (cap:%s, index=%#lx, depth=%hhu)\n",
                cap_type_string (obj.type), index, depth);
        return cnode_delete (obj, index, depth);
        break;
      }
    case METHOD_cnode_mint:
      {
        word_t dest_index = (word_t)get_mr (0);
        uint8_t dest_depth = (uint8_t)get_mr (1);
        word_t src_index = (word_t)get_mr (2);
        uint8_t src_depth = (uint8_t)get_mr (3);
        cap_rights_t rights = (cap_rights_t)get_mr (4);
        word_t badge = (word_t)get_mr (5);
        cap_t src_root;
        if (cap_type (obj) != cap_cnode)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 6)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &src_root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf ("cnode_mint (cap:%s, dest_index=%#lx, dest_depth=%hhu, "
                "src_root=cap:%s, src_index=%#lx, src_depth=%hhu, "
                "rights=%#lx, badge=%#lx)\n",
                cap_type_string (obj.type), dest_index, dest_depth,
                cap_type_string (src_root.type), src_index, src_depth, rights,
                badge);
        return cnode_mint (obj, dest_index, dest_depth, src_root, src_index,
                           src_depth, rights, badge);
        break;
      }
    case METHOD_cnode_revoke:
      {
        word_t index = (word_t)get_mr (0);
        uint8_t depth = (uint8_t)get_mr (1);

        if (cap_type (obj) != cap_cnode)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("cnode_revoke (cap:%s, index=%#lx, depth=%hhu)\n",
                cap_type_string (obj.type), index, depth);
        return cnode_revoke (obj, index, depth);
        break;
      }
    case METHOD_cnode_debug_print:
      {

        if (cap_type (obj) != cap_cnode)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("cnode_debug_print (cap:%s)\n", cap_type_string (obj.type));
        return cnode_debug_print (obj);
        break;
      }
    case METHOD_tcb_echo:
      {

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_echo (cap:%s)\n", cap_type_string (obj.type));
        return tcb_echo (obj);
        break;
      }
    case METHOD_tcb_configure:
      {
        word_t fault_ep = (word_t)get_mr (0);
        word_t cspace_root_data = (word_t)get_mr (1);
        word_t vspace_root_data = (word_t)get_mr (2);
        word_t buffer = (word_t)get_mr (3);
        cap_t cspace_root;
        cap_t vspace_root;
        cap_t buffer_frame;
        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 4)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 3)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64,
                            &cspace_root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (1), 64,
                            &vspace_root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 1);
            return error;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (2), 64,
                            &buffer_frame);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 2);
            return error;
          }

        printf ("tcb_configure (cap:%s, fault_ep=%#lx, cspace_root=cap:%s, "
                "cspace_root_data=%#lx, vspace_root=cap:%s, "
                "vspace_root_data=%#lx, buffer=%#lx, buffer_frame=cap:%s)\n",
                cap_type_string (obj.type), fault_ep,
                cap_type_string (cspace_root.type), cspace_root_data,
                cap_type_string (vspace_root.type), vspace_root_data, buffer,
                cap_type_string (buffer_frame.type));
        return tcb_configure (obj, fault_ep, cspace_root, cspace_root_data,
                              vspace_root, vspace_root_data, buffer,
                              buffer_frame);
        break;
      }
    case METHOD_tcb_copy_registers:
      {
        bool suspend_source = (bool)get_mr (0);
        bool resume_target = (bool)get_mr (1);
        bool transfer_frame = (bool)get_mr (2);
        bool transfer_integer = (bool)get_mr (3);
        word_t arch_flags = (word_t)get_mr (4);
        cap_t source;
        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 5)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &source);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf ("tcb_copy_registers (cap:%s, source=cap:%s, "
                "suspend_source=%d, resume_target=%d, transfer_frame=%d, "
                "transfer_integer=%d, arch_flags=%#lx)\n",
                cap_type_string (obj.type), cap_type_string (source.type),
                suspend_source, resume_target, transfer_frame,
                transfer_integer, arch_flags);
        return tcb_copy_registers (obj, source, suspend_source, resume_target,
                                   transfer_frame, transfer_integer,
                                   arch_flags);
        break;
      }
    case METHOD_tcb_read_registers:
      {
        bool suspend_source = (bool)get_mr (0);
        word_t arch_flags = (word_t)get_mr (1);
        word_t count = (word_t)get_mr (2);
        user_context_t *regs = (user_context_t *)get_mr (3);

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 4)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_read_registers (cap:%s, suspend_source=%d, "
                "arch_flags=%#lx, count=%#lx, regs=%p)\n",
                cap_type_string (obj.type), suspend_source, arch_flags, count,
                regs);
        return tcb_read_registers (obj, suspend_source, arch_flags, count,
                                   regs);
        break;
      }
    case METHOD_tcb_resume:
      {

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_resume (cap:%s)\n", cap_type_string (obj.type));
        return tcb_resume (obj);
        break;
      }
    case METHOD_tcb_set_affinity:
      {
        word_t affinity = (word_t)get_mr (0);

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_set_affinity (cap:%s, affinity=%#lx)\n",
                cap_type_string (obj.type), affinity);
        return tcb_set_affinity (obj, affinity);
        break;
      }
    case METHOD_tcb_set_ipc_buffer:
      {
        word_t buffer = (word_t)get_mr (0);
        cap_t buffer_frame;
        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64,
                            &buffer_frame);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf (
            "tcb_set_ipc_buffer (cap:%s, buffer=%#lx, buffer_frame=cap:%s)\n",
            cap_type_string (obj.type), buffer,
            cap_type_string (buffer_frame.type));
        return tcb_set_ipc_buffer (obj, buffer, buffer_frame);
        break;
      }
    case METHOD_tcb_set_space:
      {
        word_t fault_ep = (word_t)get_mr (0);
        word_t cspace_root_data = (word_t)get_mr (1);
        word_t vspace_root_data = (word_t)get_mr (2);
        cap_t cspace_root;
        cap_t vspace_root;
        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 3)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64,
                            &cspace_root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (1), 64,
                            &vspace_root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 1);
            return error;
          }

        printf ("tcb_set_space (cap:%s, fault_ep=%#lx, cspace_root=cap:%s, "
                "cspace_root_data=%#lx, vspace_root=cap:%s, "
                "vspace_root_data=%#lx)\n",
                cap_type_string (obj.type), fault_ep,
                cap_type_string (cspace_root.type), cspace_root_data,
                cap_type_string (vspace_root.type), vspace_root_data);
        return tcb_set_space (obj, fault_ep, cspace_root, cspace_root_data,
                              vspace_root, vspace_root_data);
        break;
      }
    case METHOD_tcb_set_tls_base:
      {
        word_t tls_base = (word_t)get_mr (0);

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_set_tls_base (cap:%s, tls_base=%#lx)\n",
                cap_type_string (obj.type), tls_base);
        return tcb_set_tls_base (obj, tls_base);
        break;
      }
    case METHOD_tcb_suspend:
      {

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_suspend (cap:%s)\n", cap_type_string (obj.type));
        return tcb_suspend (obj);
        break;
      }
    case METHOD_tcb_write_registers:
      {
        bool resume_target = (bool)get_mr (0);
        word_t arch_flags = (word_t)get_mr (1);
        word_t count = (word_t)get_mr (2);
        user_context_t *regs = (user_context_t *)get_mr (3);

        if (cap_type (obj) != cap_tcb)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 4)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("tcb_write_registers (cap:%s, resume_target=%d, "
                "arch_flags=%#lx, count=%#lx, regs=%p)\n",
                cap_type_string (obj.type), resume_target, arch_flags, count,
                regs);
        return tcb_write_registers (obj, resume_target, arch_flags, count,
                                    regs);
        break;
      }
    case METHOD_untyped_retype:
      {
        word_t type = (word_t)get_mr (0);
        word_t size_bits = (word_t)get_mr (1);
        word_t node_index = (word_t)get_mr (2);
        uint8_t node_depth = (uint8_t)get_mr (3);
        word_t node_offset = (word_t)get_mr (4);
        word_t num_objects = (word_t)get_mr (5);
        cap_t root;
        if (cap_type (obj) != cap_untyped)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 6)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &root);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf ("untyped_retype (cap:%s, type=%#lx, size_bits=%#lx, "
                "root=cap:%s, node_index=%#lx, node_depth=%hhu, "
                "node_offset=%#lx, num_objects=%#lx)\n",
                cap_type_string (obj.type), type, size_bits,
                cap_type_string (root.type), node_index, node_depth,
                node_offset, num_objects);
        return untyped_retype (obj, type, size_bits, root, node_index,
                               node_depth, node_offset, num_objects);
        break;
      }
    case METHOD_x86_io_port_in8:
      {
        word_t port = (word_t)get_mr (0);
        word_t result = (word_t)get_mr (1);

        if (cap_type (obj) != cap_x86_io_port)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_io_port_in8 (cap:%s, port=%#lx, result=%#lx)\n",
                cap_type_string (obj.type), port, result);
        return x86_io_port_in8 (obj, port, result);
        break;
      }
    case METHOD_x86_io_port_in16:
      {
        word_t port = (word_t)get_mr (0);
        word_t result = (word_t)get_mr (1);

        if (cap_type (obj) != cap_x86_io_port)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_io_port_in16 (cap:%s, port=%#lx, result=%#lx)\n",
                cap_type_string (obj.type), port, result);
        return x86_io_port_in16 (obj, port, result);
        break;
      }
    case METHOD_x86_io_port_in32:
      {
        word_t port = (word_t)get_mr (0);
        word_t result = (word_t)get_mr (1);

        if (cap_type (obj) != cap_x86_io_port)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_io_port_in32 (cap:%s, port=%#lx, result=%#lx)\n",
                cap_type_string (obj.type), port, result);
        return x86_io_port_in32 (obj, port, result);
        break;
      }
    case METHOD_x86_io_port_out8:
      {
        word_t port = (word_t)get_mr (0);
        word_t value = (word_t)get_mr (1);

        if (cap_type (obj) != cap_x86_io_port)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_io_port_out8 (cap:%s, port=%#lx, value=%#lx)\n",
                cap_type_string (obj.type), port, value);
        return x86_io_port_out8 (obj, port, value);
        break;
      }
    case METHOD_x86_io_port_out16:
      {
        word_t port = (word_t)get_mr (0);
        word_t value = (word_t)get_mr (1);

        if (cap_type (obj) != cap_x86_io_port)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_io_port_out16 (cap:%s, port=%#lx, value=%#lx)\n",
                cap_type_string (obj.type), port, value);
        return x86_io_port_out16 (obj, port, value);
        break;
      }
    case METHOD_x86_io_port_out32:
      {
        word_t port = (word_t)get_mr (0);
        word_t value = (word_t)get_mr (1);

        if (cap_type (obj) != cap_x86_io_port)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_io_port_out32 (cap:%s, port=%#lx, value=%#lx)\n",
                cap_type_string (obj.type), port, value);
        return x86_io_port_out32 (obj, port, value);
        break;
      }
    case METHOD_x86_64_pdpt_map:
      {
        word_t vaddr = (word_t)get_mr (0);
        x86_vm_attributes_t attr = (x86_vm_attributes_t)get_mr (1);
        cap_t vspace;
        if (cap_type (obj) != cap_x86_64_pdpt)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &vspace);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf (
            "x86_64_pdpt_map (cap:%s, vspace=cap:%s, vaddr=%#lx, attr=%#lx)\n",
            cap_type_string (obj.type), cap_type_string (vspace.type), vaddr,
            attr);
        return x86_64_pdpt_map (obj, vspace, vaddr, attr);
        break;
      }
    case METHOD_x86_64_pdpt_unmap:
      {

        if (cap_type (obj) != cap_x86_64_pdpt)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_64_pdpt_unmap (cap:%s)\n", cap_type_string (obj.type));
        return x86_64_pdpt_unmap (obj);
        break;
      }
    case METHOD_x86_64_pd_map:
      {
        word_t vaddr = (word_t)get_mr (0);
        x86_vm_attributes_t attr = (x86_vm_attributes_t)get_mr (1);
        cap_t vspace;
        if (cap_type (obj) != cap_x86_64_pd)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &vspace);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf (
            "x86_64_pd_map (cap:%s, vspace=cap:%s, vaddr=%#lx, attr=%#lx)\n",
            cap_type_string (obj.type), cap_type_string (vspace.type), vaddr,
            attr);
        return x86_64_pd_map (obj, vspace, vaddr, attr);
        break;
      }
    case METHOD_x86_64_pd_unmap:
      {

        if (cap_type (obj) != cap_x86_64_pd)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_64_pd_unmap (cap:%s)\n", cap_type_string (obj.type));
        return x86_64_pd_unmap (obj);
        break;
      }
    case METHOD_x86_64_pt_map:
      {
        word_t vaddr = (word_t)get_mr (0);
        x86_vm_attributes_t attr = (x86_vm_attributes_t)get_mr (1);
        cap_t vspace;
        if (cap_type (obj) != cap_x86_64_pt)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &vspace);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf (
            "x86_64_pt_map (cap:%s, vspace=cap:%s, vaddr=%#lx, attr=%#lx)\n",
            cap_type_string (obj.type), cap_type_string (vspace.type), vaddr,
            attr);
        return x86_64_pt_map (obj, vspace, vaddr, attr);
        break;
      }
    case METHOD_x86_64_pt_unmap:
      {

        if (cap_type (obj) != cap_x86_64_pt)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_64_pt_unmap (cap:%s)\n", cap_type_string (obj.type));
        return x86_64_pt_unmap (obj);
        break;
      }
    case METHOD_x86_64_page_map:
      {
        word_t vaddr = (word_t)get_mr (0);
        x86_vm_attributes_t attr = (x86_vm_attributes_t)get_mr (1);
        cap_t vspace;
        if (cap_type (obj) != cap_x86_64_page)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 2)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 1)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }
        error = lookup_cap (this_tcb->cspace_root, get_cap (0), 64, &vspace);
        if (error != no_error)
          {
            return_ipc (error, 1);
            set_mr (0, 0);
            return error;
          }

        printf (
            "x86_64_page_map (cap:%s, vspace=cap:%s, vaddr=%#lx, attr=%#lx)\n",
            cap_type_string (obj.type), cap_type_string (vspace.type), vaddr,
            attr);
        return x86_64_page_map (obj, vspace, vaddr, attr);
        break;
      }
    case METHOD_x86_64_page_unmap:
      {

        if (cap_type (obj) != cap_x86_64_page)
          {
            return_ipc (illegal_operation, 0);
            return illegal_operation;
          }
        if (get_message_length (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return illegal_operation;
          }
        if (get_message_extra_caps (info) < 0)
          {
            return_ipc (truncated_message, 0);
            return truncated_message;
          }

        printf ("x86_64_page_unmap (cap:%s)\n", cap_type_string (obj.type));
        return x86_64_page_unmap (obj);
        break;
      }
    default:
      return_ipc (illegal_operation, 0);
      return illegal_operation;
    }
}
