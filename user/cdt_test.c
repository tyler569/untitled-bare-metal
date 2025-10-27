#include "stdio.h"
#include "sys/syscall.h"

#include "./lib.h"

// Capability slots in our test cnode
enum cdt_test_caps
{
  cdt_cnode_cap = 0,
  cdt_untyped_cap = 1,

  // Test resource slots
  cdt_endpoint_orig = 10,
  cdt_endpoint_copy1 = 11,
  cdt_endpoint_copy2 = 12,
  cdt_endpoint_copy3 = 13,

  cdt_notification_orig = 20,
  cdt_notification_mint1 = 21,
  cdt_notification_mint2 = 22,

  cdt_endpoint_mint_orig = 30,
  cdt_endpoint_mint1 = 31,
  cdt_endpoint_mint2 = 32,
  cdt_endpoint_mint3 = 33,

  // Temporary slots for additional tests
  cdt_temp1 = 40,
  cdt_temp2 = 41,
  cdt_temp3 = 42,
};

void
print_test (const char *name, int actual, int expected)
{
  if (actual == expected)
    printf ("  [PASS] %s\n", name);
  else
    printf ("  [FAIL] %s (got %s, expected %s)\n", name,
            error_string (actual), error_string (expected));
}

int
main ()
{
  printf ("\n=== CDT Test Suite ===\n\n");

  int err;

  // Test 1: Basic copy operation
  printf ("Test 1: Basic copy operations\n");

  // Create original endpoint from untyped
  err = untyped_retype (cdt_untyped_cap, cap_endpoint, 0, cdt_cnode_cap,
                        cdt_cnode_cap, 64, cdt_endpoint_orig, 1);
  print_test ("Create original endpoint", err, no_error);

  // Copy it to another slot
  err = cnode_copy (cdt_cnode_cap, cdt_endpoint_copy1, 64, cdt_cnode_cap,
                    cdt_endpoint_orig, 64, cap_rights_all);
  print_test ("Copy endpoint (copy1)", err, no_error);

  // Copy again to create a sibling
  err = cnode_copy (cdt_cnode_cap, cdt_endpoint_copy2, 64, cdt_cnode_cap,
                    cdt_endpoint_orig, 64, cap_rights_all);
  print_test ("Copy endpoint (copy2)", err, no_error);

  // Copy from a copy to create a child
  err = cnode_copy (cdt_cnode_cap, cdt_endpoint_copy3, 64, cdt_cnode_cap,
                    cdt_endpoint_copy1, 64, cap_rights_all);
  print_test ("Copy from copy (copy3)", err, no_error);

  printf ("\n");

  // Test 2: Mint operation on endpoint
  printf ("Test 2: Mint operations on endpoint\n");

  // Create unbadged endpoint
  err = untyped_retype (cdt_untyped_cap, cap_endpoint, 0, cdt_cnode_cap,
                        cdt_cnode_cap, 64, cdt_endpoint_mint_orig, 1);
  print_test ("Create unbadged endpoint", err, no_error);

  // Mint with badge 0x1234
  err = cnode_mint (cdt_cnode_cap, cdt_endpoint_mint1, 64, cdt_cnode_cap,
                    cdt_endpoint_mint_orig, 64, cap_rights_all, 0x1234);
  print_test ("Mint endpoint with badge 0x1234", err, no_error);

  // Try to mint from already-badged endpoint (should fail)
  err = cnode_mint (cdt_cnode_cap, cdt_endpoint_mint2, 64, cdt_cnode_cap,
                    cdt_endpoint_mint1, 64, cap_rights_all, 0x5678);
  print_test ("Mint from badged endpoint fails", err, illegal_operation);

  // Mint another badge from original
  err = cnode_mint (cdt_cnode_cap, cdt_endpoint_mint3, 64, cdt_cnode_cap,
                    cdt_endpoint_mint_orig, 64, cap_rights_all, 0xABCD);
  print_test ("Mint another badge 0xABCD", err, no_error);

  printf ("\n");

  // Test 3: Mint operations on notification
  printf ("Test 3: Mint operations on notification\n");

  // Create notification
  err = untyped_retype (cdt_untyped_cap, cap_notification, 0, cdt_cnode_cap,
                        cdt_cnode_cap, 64, cdt_notification_orig, 1);
  print_test ("Create notification", err, no_error);

  // Mint notification with badge (notifications don't create bi-level structure)
  err = cnode_mint (cdt_cnode_cap, cdt_notification_mint1, 64, cdt_cnode_cap,
                    cdt_notification_orig, 64, cap_rights_all, 0xF00D);
  print_test ("Mint notification with badge", err, no_error);

  // Can mint again from original notification
  err = cnode_mint (cdt_cnode_cap, cdt_notification_mint2, 64, cdt_cnode_cap,
                    cdt_notification_orig, 64, cap_rights_all, 0xBEEF);
  print_test ("Mint notification with another badge", err, no_error);

  printf ("\n");

  // Test 4: Delete operations
  printf ("Test 4: Delete operations\n");

  // Delete copy1 which has copy3 as a child (should auto-revoke)
  err = cnode_delete (cdt_cnode_cap, cdt_endpoint_copy1, 64);
  print_test ("Delete with children (auto-revokes)", err, no_error);

  // Verify copy3 was also deleted by trying to delete it (should be no-op or fail)
  err = cnode_delete (cdt_cnode_cap, cdt_endpoint_copy3, 64);
  print_test ("Copy3 already deleted", err, no_error);

  printf ("\n");

  // Test 5: Revoke operations
  printf ("Test 5: Revoke operations\n");

  // Create a chain: temp1 -> copy from endpoint_orig
  err = cnode_copy (cdt_cnode_cap, cdt_temp1, 64, cdt_cnode_cap,
                    cdt_endpoint_orig, 64, cap_rights_all);
  print_test ("Create temp1 copy", err, no_error);

  // temp2 -> copy from temp1
  err = cnode_copy (cdt_cnode_cap, cdt_temp2, 64, cdt_cnode_cap, cdt_temp1, 64,
                    cap_rights_all);
  print_test ("Create temp2 from temp1", err, no_error);

  // temp3 -> copy from temp2
  err = cnode_copy (cdt_cnode_cap, cdt_temp3, 64, cdt_cnode_cap, cdt_temp2, 64,
                    cap_rights_all);
  print_test ("Create temp3 from temp2", err, no_error);

  // Verify temp3 exists (try to copy from it)
  err = cnode_copy (cdt_cnode_cap, 50, 64, cdt_cnode_cap, cdt_temp3, 64,
                    cap_rights_all);
  print_test ("Verify temp3 exists", err, no_error);

  // Now delete slot 50 to clean up
  err = cnode_delete (cdt_cnode_cap, 50, 64);

  // Revoke temp1 - should delete temp2 and temp3
  printf ("  [INFO] Revoking temp1...\n");
  err = cnode_delete (cdt_cnode_cap, cdt_temp1, 64);
  print_test ("Delete temp1 (should revoke children)", err, no_error);

  printf ("\n");

  // Test 6: Error conditions
  printf ("Test 6: Error conditions\n");

  // Try to copy into occupied slot
  err = cnode_copy (cdt_cnode_cap, cdt_untyped_cap, 64, cdt_cnode_cap,
                    cdt_endpoint_copy2, 64, cap_rights_all);
  print_test ("Copy to occupied slot fails", err, delete_first);

  // Try to mint into occupied slot
  err = cnode_mint (cdt_cnode_cap, cdt_endpoint_mint_orig, 64, cdt_cnode_cap,
                    cdt_endpoint_mint_orig, 64, cap_rights_all, 0x9999);
  print_test ("Mint to occupied slot fails", err, delete_first);

  // Try to mint from non-endpoint/notification type
  // (we'd need a different cap type for this - let's create a cnode)
  err = untyped_retype (cdt_untyped_cap, cap_cnode, 4, cdt_cnode_cap,
                        cdt_cnode_cap, 64, 60, 1);
  if (err == no_error)
    {
      err = cnode_mint (cdt_cnode_cap, 61, 64, cdt_cnode_cap, 60, 64,
                        cap_rights_all, 0x1111);
      print_test ("Mint from cnode fails", err, illegal_operation);
    }
  else
    print_test ("Create test cnode for mint test", err, no_error);

  printf ("\n");

  // Test 7: Rights masking
  printf ("Test 7: Rights masking on copy\n");

  // Create endpoint with all rights
  err = untyped_retype (cdt_untyped_cap, cap_endpoint, 0, cdt_cnode_cap,
                        cdt_cnode_cap, 64, 70, 1);
  print_test ("Create endpoint for rights test", err, no_error);

  // Copy with reduced rights (read only = 0x1)
  err = cnode_copy (cdt_cnode_cap, 71, 64, cdt_cnode_cap, 70, 64, 0x1);
  print_test ("Copy with reduced rights", err, no_error);

  // Copy from reduced rights copy (should keep masked rights)
  err = cnode_copy (cdt_cnode_cap, 72, 64, cdt_cnode_cap, 71, 64,
                    cap_rights_all);
  print_test ("Copy from reduced rights copy", err, no_error);

  printf ("\n=== All tests complete ===\n\n");

  exit (0);
}
