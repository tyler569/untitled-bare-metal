#include "stdio.h"

#include "./lib.h"
#include "./pci_util.h"

enum pci_manager_caps
{
  endpoint_cap,
  pci_port_cap,
  cnode_cap,
  tmp_cap,
};

enum pci_manager_message
{
  pci_manager_issue,
  pci_manager_read,
  pci_manager_write,
};

message_info_t handle_issue (message_info_t, word_t badge);
message_info_t handle_read (message_info_t, word_t badge);
message_info_t handle_write (message_info_t, word_t badge);

int
main ()
{
  bool done = false;
  message_info_t info, resp;
  word_t badge;

  info = recv (endpoint_cap, &badge);

  while (!done)
    {
      word_t label = get_message_label (info);

      switch (label)
        {
        case pci_manager_issue:
          resp = handle_issue (info, badge);
          break;
        case pci_manager_read:
          resp = handle_read (info, badge);
          break;
        case pci_manager_write:
          resp = handle_write (info, badge);
          break;
        default:
          set_mr (0, 0);
          resp = new_message_info (invalid_argument, 0, 0, 1);
          break;
        }

      if (done)
        reply (resp);
      else
        info = reply_recv (endpoint_cap, resp, &badge);
    }

  exit (0);
}

message_info_t
handle_issue (message_info_t info, uint64_t badge)
{
  if (badge != 0)
    return new_message_info (illegal_operation, 0, 0, 0);

  if (get_message_length (info) < 1)
    {
      set_mr (0, 1);
      return new_message_info (truncated_message, 0, 0, 1);
    }

  cnode_delete (cnode_cap, tmp_cap, 64); // potential previous iteration
  cnode_mint (cnode_cap, tmp_cap, 64, cnode_cap, endpoint_cap, 64,
              cap_rights_all, get_mr (1));
  set_cap (0, tmp_cap);
  return new_message_info (no_error, 0, 1, 0);
}

message_info_t
handle_read (message_info_t info, uint64_t badge)
{
  if (get_message_length (info) < 2)
    {
      set_mr (0, 2);
      return new_message_info (truncated_message, 0, 0, 1);
    }
  switch (get_mr (0))
    {
    case 1:
      set_mr (0, pci_read_b (pci_port_cap, badge | (get_mr (1) & 0xff)));
      return new_message_info (no_error, 0, 0, 1);
    case 2:
      set_mr (0, pci_read_w (pci_port_cap, badge | (get_mr (1) & 0xff)));
      return new_message_info (no_error, 0, 0, 1);
    case 4:
      set_mr (0, pci_read_l (pci_port_cap, badge | (get_mr (1) & 0xff)));
      return new_message_info (no_error, 0, 0, 1);
    default:
      set_mr (0, 1);
      return new_message_info (invalid_argument, 0, 0, 1);
    }
}

message_info_t
handle_write (message_info_t info, uint64_t badge)
{
  if (get_message_length (info) < 3)
    {
      set_mr (0, 3);
      return new_message_info (truncated_message, 0, 0, 1);
    }
  switch (get_mr (0))
    {
    case 1:
      pci_write_b (pci_port_cap, badge | (get_mr (1) & 0xff), get_mr (2));
      return new_message_info (no_error, 0, 0, 0);
    case 2:
      pci_write_w (pci_port_cap, badge | (get_mr (1) & 0xff), get_mr (2));
      return new_message_info (no_error, 0, 0, 0);
    case 4:
      pci_write_l (pci_port_cap, badge | (get_mr (1) & 0xff), get_mr (2));
      return new_message_info (no_error, 0, 0, 0);
    default:
      set_mr (0, 1);
      return new_message_info (invalid_argument, 0, 0, 1);
    }
}
