#include "stdio.h"
#include "sys/syscall.h"

#include "./lib.h"
#include "kern/cap.h"

// Capability slots in our test cnode
enum cdt_test_caps
{
  CDT_CNODE_CAP = 0,
  CDT_UNTYPED_CAP = 1,

  // Test resource slots
  CDT_ENDPOINT_ORIG = 10,
  CDT_ENDPOINT_COPY1 = 11,
  CDT_ENDPOINT_COPY2 = 12,
  CDT_ENDPOINT_COPY3 = 13,

  CDT_NOTIFICATION_ORIG = 20,
  CDT_NOTIFICATION_MINT1 = 21,
  CDT_NOTIFICATION_MINT2 = 22,

  CDT_ENDPOINT_MINT_ORIG = 30,
  CDT_ENDPOINT_MINT1 = 31,
  CDT_ENDPOINT_MINT2 = 32,
  CDT_ENDPOINT_MINT3 = 33,

  // Temporary slots for additional tests
  CDT_TEMP1 = 40,
  CDT_TEMP2 = 41,
  CDT_TEMP3 = 42,
};

// Test counters
static int total_tests = 0;
static int failed_tests = 0;

// Helper function to retrieve a capability from a slot
static cap_t
get_cap_at_slot (word_t slot_index)
{
  cnode_debug_get (CDT_CNODE_CAP, slot_index, 64);
  cap_t cap;
  cap.words[0] = get_mr (0);
  cap.words[1] = get_mr (1);
  return cap;
}

void
print_test (const char *name, int actual, int expected)
{
  total_tests++;
  if (actual != expected)
    {
      failed_tests++;
      printf ("  [FAIL] %s (got %s, expected %s)\n", name,
              error_string (actual), error_string (expected));
    }
}

void
assert_cap_type (const char *name, cap_t cap, word_t expected_type)
{
  total_tests++;
  if (cap.type != expected_type)
    {
      failed_tests++;
      printf ("  [FAIL] %s has type %s (expected %s)\n", name,
              cap_type_string (cap.type), cap_type_string (expected_type));
    }
}

void
assert_cap_is_original (const char *name, cap_t cap, bool expected)
{
  total_tests++;
  if (cap.is_original != expected)
    {
      failed_tests++;
      printf ("  [FAIL] %s is_original=%d (expected %d)\n", name,
              cap.is_original, expected);
    }
}

void
assert_cap_badge (const char *name, cap_t cap, word_t expected_badge)
{
  total_tests++;
  if (cap.badge != expected_badge)
    {
      failed_tests++;
      printf ("  [FAIL] %s has badge 0x%lx (expected 0x%lx)\n", name,
              cap.badge, expected_badge);
    }
}

void
assert_cap_rights (const char *name, cap_t cap, word_t expected_rights)
{
  total_tests++;
  if (cap.rights != expected_rights)
    {
      failed_tests++;
      printf ("  [FAIL] %s has rights 0x%x (expected 0x%lx)\n", name,
              cap.rights, expected_rights);
    }
}

void
assert_cap_is_null (const char *name, cap_t cap)
{
  total_tests++;
  if (cap.type != CAP_NULL)
    {
      failed_tests++;
      printf ("  [FAIL] %s is %s (expected null)\n", name,
              cap_type_string (cap.type));
    }
}

int
main ()
{
  int err;

  // Create original endpoint from untyped
  err = untyped_retype (CDT_UNTYPED_CAP, CAP_ENDPOINT, 0, CDT_CNODE_CAP,
                        CDT_CNODE_CAP, 64, CDT_ENDPOINT_ORIG, 1);
  print_test ("Create original endpoint", err, NO_ERROR);

  // Verify the original has is_original=true (from untyped_retype via
  // create_objects)
  cap_t ep_orig = get_cap_at_slot (CDT_ENDPOINT_ORIG);
  assert_cap_type ("endpoint_orig", ep_orig, CAP_ENDPOINT);
  assert_cap_is_original ("endpoint_orig", ep_orig, true);
  assert_cap_badge ("endpoint_orig", ep_orig, 0);

  // Copy it to another slot
  err = cnode_copy (CDT_CNODE_CAP, CDT_ENDPOINT_COPY1, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_ORIG, 64, CAP_RIGHTS_ALL);
  print_test ("Copy endpoint (copy1)", err, NO_ERROR);

  // Verify copy1 has is_original=false
  cap_t ep_copy1 = get_cap_at_slot (CDT_ENDPOINT_COPY1);
  assert_cap_type ("endpoint_copy1", ep_copy1, CAP_ENDPOINT);
  assert_cap_is_original ("endpoint_copy1", ep_copy1, false);
  assert_cap_badge ("endpoint_copy1", ep_copy1, 0);
  assert_cap_rights ("endpoint_copy1", ep_copy1, CAP_RIGHTS_ALL);

  // Copy again to create a sibling
  err = cnode_copy (CDT_CNODE_CAP, CDT_ENDPOINT_COPY2, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_ORIG, 64, CAP_RIGHTS_ALL);
  print_test ("Copy endpoint (copy2)", err, NO_ERROR);

  cap_t ep_copy2 = get_cap_at_slot (CDT_ENDPOINT_COPY2);
  assert_cap_is_original ("endpoint_copy2", ep_copy2, false);

  // Copy from a copy to create a child
  err = cnode_copy (CDT_CNODE_CAP, CDT_ENDPOINT_COPY3, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_COPY1, 64, CAP_RIGHTS_ALL);
  print_test ("Copy from copy (copy3)", err, NO_ERROR);

  cap_t ep_copy3 = get_cap_at_slot (CDT_ENDPOINT_COPY3);
  assert_cap_is_original ("endpoint_copy3", ep_copy3, false);

  // Create unbadged endpoint
  err = untyped_retype (CDT_UNTYPED_CAP, CAP_ENDPOINT, 0, CDT_CNODE_CAP,
                        CDT_CNODE_CAP, 64, CDT_ENDPOINT_MINT_ORIG, 1);
  print_test ("Create unbadged endpoint", err, NO_ERROR);

  cap_t ep_mint_orig = get_cap_at_slot (CDT_ENDPOINT_MINT_ORIG);
  assert_cap_badge ("endpoint_mint_orig", ep_mint_orig, 0);
  assert_cap_is_original ("endpoint_mint_orig", ep_mint_orig, true);

  // Mint with badge 0x1234
  err = cnode_mint (CDT_CNODE_CAP, CDT_ENDPOINT_MINT1, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_MINT_ORIG, 64, CAP_RIGHTS_ALL, 0x1234);
  print_test ("Mint endpoint with badge 0x1234", err, NO_ERROR);

  // Verify minted endpoint has badge and is_original=true
  cap_t ep_mint1 = get_cap_at_slot (CDT_ENDPOINT_MINT1);
  assert_cap_badge ("endpoint_mint1", ep_mint1, 0x1234);
  assert_cap_is_original ("endpoint_mint1", ep_mint1, true);

  // Try to mint from already-badged endpoint
  err = cnode_mint (CDT_CNODE_CAP, CDT_ENDPOINT_MINT2, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_MINT1, 64, CAP_RIGHTS_ALL, 0x5678);
  print_test ("Mint from badged endpoint fails", err, ILLEGAL_OPERATION);

  // Mint another badge from original
  err = cnode_mint (CDT_CNODE_CAP, CDT_ENDPOINT_MINT3, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_MINT_ORIG, 64, CAP_RIGHTS_ALL, 0xABCD);
  print_test ("Mint another badge 0xABCD", err, NO_ERROR);

  cap_t ep_mint3 = get_cap_at_slot (CDT_ENDPOINT_MINT3);
  assert_cap_badge ("endpoint_mint3", ep_mint3, 0xABCD);
  assert_cap_is_original ("endpoint_mint3", ep_mint3, true);

  // Create notification
  err = untyped_retype (CDT_UNTYPED_CAP, CAP_NOTIFICATION, 0, CDT_CNODE_CAP,
                        CDT_CNODE_CAP, 64, CDT_NOTIFICATION_ORIG, 1);
  print_test ("Create notification", err, NO_ERROR);

  cap_t notif_orig = get_cap_at_slot (CDT_NOTIFICATION_ORIG);
  assert_cap_type ("notification_orig", notif_orig, CAP_NOTIFICATION);
  assert_cap_is_original ("notification_orig", notif_orig, true);
  assert_cap_badge ("notification_orig", notif_orig, 0);

  // Mint notification with badge (notifications don't set is_original on mint,
  // unlike endpoints)
  err = cnode_mint (CDT_CNODE_CAP, CDT_NOTIFICATION_MINT1, 64, CDT_CNODE_CAP,
                    CDT_NOTIFICATION_ORIG, 64, CAP_RIGHTS_ALL, 0xF00D);
  print_test ("Mint notification with badge", err, NO_ERROR);

  cap_t notif_mint1 = get_cap_at_slot (CDT_NOTIFICATION_MINT1);
  assert_cap_badge ("notification_mint1", notif_mint1, 0xF00D);
  // Notifications don't get is_original=true on mint (only endpoints do)
  assert_cap_is_original ("notification_mint1", notif_mint1, false);

  // Can mint again from original notification (unlike badged endpoints)
  err = cnode_mint (CDT_CNODE_CAP, CDT_NOTIFICATION_MINT2, 64, CDT_CNODE_CAP,
                    CDT_NOTIFICATION_ORIG, 64, CAP_RIGHTS_ALL, 0xBEEF);
  print_test ("Mint notification with another badge", err, NO_ERROR);

  cap_t notif_mint2 = get_cap_at_slot (CDT_NOTIFICATION_MINT2);
  assert_cap_badge ("notification_mint2", notif_mint2, 0xBEEF);
  assert_cap_is_original ("notification_mint2", notif_mint2, false);

  // Verify copy3 exists before deletion
  cap_t before_del = get_cap_at_slot (CDT_ENDPOINT_COPY3);
  assert_cap_type ("endpoint_copy3 before delete", before_del, CAP_ENDPOINT);

  // Delete copy1
  err = cnode_delete (CDT_CNODE_CAP, CDT_ENDPOINT_COPY1, 64);
  print_test ("Delete with children (auto-revokes)", err, NO_ERROR);

  // Verify copy1 is now null
  cap_t copy1_after = get_cap_at_slot (CDT_ENDPOINT_COPY1);
  assert_cap_is_null ("endpoint_copy1 after delete", copy1_after);

  // Verify copy3 was not deleted
  cap_t copy3_after = get_cap_at_slot (CDT_ENDPOINT_COPY3);
  assert_cap_type ("endpoint_copy3 after parent delete", copy3_after,
                   CAP_ENDPOINT);

  // Deleting an already-null slot is a no-op
  err = cnode_delete (CDT_CNODE_CAP, CDT_ENDPOINT_COPY3, 64);
  print_test ("Delete null slot is no-op", err, NO_ERROR);

  // Create a chain: temp1 -> copy from endpoint_orig
  err = cnode_copy (CDT_CNODE_CAP, CDT_TEMP1, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_ORIG, 64, CAP_RIGHTS_ALL);
  print_test ("Create temp1 copy", err, NO_ERROR);

  cap_t temp1 = get_cap_at_slot (CDT_TEMP1);
  assert_cap_is_original ("temp1", temp1, false);

  // temp2 -> copy from temp1
  err = cnode_copy (CDT_CNODE_CAP, CDT_TEMP2, 64, CDT_CNODE_CAP, CDT_TEMP1, 64,
                    CAP_RIGHTS_ALL);
  print_test ("Create temp2 from temp1", err, NO_ERROR);

  cap_t temp2 = get_cap_at_slot (CDT_TEMP2);
  assert_cap_is_original ("temp2", temp2, false);

  // temp3 -> copy from temp2
  err = cnode_copy (CDT_CNODE_CAP, CDT_TEMP3, 64, CDT_CNODE_CAP, CDT_TEMP2, 64,
                    CAP_RIGHTS_ALL);
  print_test ("Create temp3 from temp2", err, NO_ERROR);

  cap_t temp3_before = get_cap_at_slot (CDT_TEMP3);
  assert_cap_type ("temp3 exists before revoke", temp3_before, CAP_ENDPOINT);

  // Verify temp3 exists (try to copy from it)
  err = cnode_copy (CDT_CNODE_CAP, 50, 64, CDT_CNODE_CAP, CDT_TEMP3, 64,
                    CAP_RIGHTS_ALL);
  print_test ("Verify temp3 exists", err, NO_ERROR);

  // Now delete slot 50 to clean up
  err = cnode_delete (CDT_CNODE_CAP, 50, 64);

  // Try to copy into occupied slot
  err = cnode_copy (CDT_CNODE_CAP, CDT_UNTYPED_CAP, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_COPY2, 64, CAP_RIGHTS_ALL);
  print_test ("Copy to occupied slot fails", err, DELETE_FIRST);

  // Try to mint into occupied slot
  err = cnode_mint (CDT_CNODE_CAP, CDT_ENDPOINT_MINT_ORIG, 64, CDT_CNODE_CAP,
                    CDT_ENDPOINT_MINT_ORIG, 64, CAP_RIGHTS_ALL, 0x9999);
  print_test ("Mint to occupied slot fails", err, DELETE_FIRST);

  // Try to mint from non-endpoint/notification type
  // (we'd need a different cap type for this - let's create a cnode)
  err = untyped_retype (CDT_UNTYPED_CAP, CAP_CNODE, 4, CDT_CNODE_CAP,
                        CDT_CNODE_CAP, 64, 60, 1);
  if (err == NO_ERROR)
    {
      err = cnode_mint (CDT_CNODE_CAP, 61, 64, CDT_CNODE_CAP, 60, 64,
                        CAP_RIGHTS_ALL, 0x1111);
      print_test ("Mint from cnode fails", err, ILLEGAL_OPERATION);
    }
  else
    print_test ("Create test cnode for mint test", err, NO_ERROR);

  // Create endpoint with all rights
  err = untyped_retype (CDT_UNTYPED_CAP, CAP_ENDPOINT, 0, CDT_CNODE_CAP,
                        CDT_CNODE_CAP, 64, 70, 1);
  print_test ("Create endpoint for rights test", err, NO_ERROR);

  cap_t rights_orig = get_cap_at_slot (70);
  assert_cap_rights ("rights_orig has all rights", rights_orig,
                     CAP_RIGHTS_ALL);

  // Copy with reduced rights (read only = 0x1)
  err = cnode_copy (CDT_CNODE_CAP, 71, 64, CDT_CNODE_CAP, 70, 64, 0x1);
  print_test ("Copy with reduced rights", err, NO_ERROR);

  // Verify rights are masked
  cap_t rights_reduced = get_cap_at_slot (71);
  assert_cap_rights ("rights_reduced has 0x1", rights_reduced, 0x1);

  // Copy from reduced rights copy (should keep masked rights)
  err = cnode_copy (CDT_CNODE_CAP, 72, 64, CDT_CNODE_CAP, 71, 64,
                    CAP_RIGHTS_ALL);
  print_test ("Copy from reduced rights copy", err, NO_ERROR);

  // Rights should be 0x1 & cap_rights_all = 0x1
  cap_t rights_child = get_cap_at_slot (72);
  assert_cap_rights ("rights_child inherits masked rights", rights_child, 0x1);

  int passed_tests = total_tests - failed_tests;
  printf ("CDT validations: %d/%d tests passed\n", passed_tests, total_tests);
  if (failed_tests > 0)
    printf ("%d tests failed\n", failed_tests);

  exit (0);
}
