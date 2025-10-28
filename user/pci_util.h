#include "lib.h"
#include "pci.h"
#include "sys/types.h"

enum pci_manager_caps
{
  pci_endpoint_cap,
  pci_port_cap,
  pci_cnode_cap,
  pci_tmp_cap,
};

enum pci_manager_message
{
  pci_manager_issue,
  pci_manager_read,
  pci_manager_write,
  pci_manager_device_info,
  pci_manager_enumerate,
};
