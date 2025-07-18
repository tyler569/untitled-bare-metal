#include "stdio.h"
#include "string.h"
#include "x86_64.h"

// fw_cfg I/O port definitions
#define FW_CFG_SELECTOR_PORT 0x510
#define FW_CFG_DATA_PORT 0x511

// fw_cfg selectors
#define FW_CFG_SIGNATURE 0x00
#define FW_CFG_ID 0x01
#define FW_CFG_FILE_DIR 0x19

// fw_cfg I/O port helper functions
static inline uint8_t
fw_cfg_read_byte (void)
{
  return read_port_b (FW_CFG_DATA_PORT);
}

static inline void
fw_cfg_select (uint16_t selector)
{
  write_port_w (FW_CFG_SELECTOR_PORT, selector);
}

// Read fw_cfg file by name
// Returns number of bytes read, or 0 if not found
size_t
fw_cfg_read_file (const char *name, void *buffer, size_t max_size)
{
  // Select file directory
  fw_cfg_select (FW_CFG_FILE_DIR);

  // Read file count (big-endian 32-bit)
  uint32_t file_count = 0;
  file_count |= (uint32_t)fw_cfg_read_byte () << 24;
  file_count |= (uint32_t)fw_cfg_read_byte () << 16;
  file_count |= (uint32_t)fw_cfg_read_byte () << 8;
  file_count |= (uint32_t)fw_cfg_read_byte ();

  // Search through file directory
  for (uint32_t i = 0; i < file_count; i++)
    {
      // Read file entry: size (32-bit BE), selector (16-bit BE), reserved
      // (16-bit), name (56 bytes)
      uint32_t size = 0;
      size |= (uint32_t)fw_cfg_read_byte () << 24;
      size |= (uint32_t)fw_cfg_read_byte () << 16;
      size |= (uint32_t)fw_cfg_read_byte () << 8;
      size |= (uint32_t)fw_cfg_read_byte ();

      uint16_t selector = 0;
      selector |= (uint16_t)fw_cfg_read_byte () << 8;
      selector |= (uint16_t)fw_cfg_read_byte ();

      // Skip reserved field
      fw_cfg_read_byte ();
      fw_cfg_read_byte ();

      // Read filename (56 bytes, null-terminated)
      char filename[57] = { 0 };
      for (int j = 0; j < 56; j++)
        {
          filename[j] = fw_cfg_read_byte ();
        }

      // Check if this is the file we want
      if (strcmp (filename, name) == 0)
        {
          // Select the file and read its contents
          fw_cfg_select (selector);

          size_t bytes_to_read = (size < max_size) ? size : max_size;
          uint8_t *buf = (uint8_t *)buffer;

          for (size_t k = 0; k < bytes_to_read; k++)
            {
              buf[k] = fw_cfg_read_byte ();
            }

          return bytes_to_read;
        }
    }

  // printf("fw_cfg: File '%s' not found\n", name);
  return 0;
}

// Test fw_cfg functionality
void
test_fw_cfg (void)
{
  // Test reading signature
  fw_cfg_select (FW_CFG_SIGNATURE);
  char signature[5] = { 0 };
  for (int i = 0; i < 4; i++)
    {
      signature[i] = fw_cfg_read_byte ();
    }

  // Try to read a test configuration file
  char test_config[256] = { 0 };
  size_t config_size = fw_cfg_read_file ("opt/verify_fw_cfg", test_config,
                                         sizeof (test_config) - 1);

  if (config_size > 0)
    {
      test_config[config_size] = '\0'; // Ensure null termination
      printf ("    Test configuration: '%s' (%zu bytes)\n", test_config,
              config_size);
    }
  else
    {
      printf ("    No test configuration found\n");
    }
}
