#include "lib.h"
#include "pci.h"
#include "sys/types.h"

enum pci_manager_caps
{
  PCI_ENDPOINT_CAP,
  PCI_PORT_CAP,
  PCI_CNODE_CAP,
  PCI_TMP_CAP,
};

enum pci_manager_message
{
  PCI_MANAGER_ISSUE,
  PCI_MANAGER_READ,
  PCI_MANAGER_WRITE,
  PCI_MANAGER_DEVICE_INFO,
  PCI_MANAGER_ENUMERATE,
};
