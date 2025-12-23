/**
 * Generic test suite teardown for Unity test framework.
 *
 * This is called after all tests complete. The unity_cfg.yaml configures
 * the generated runner to call test_suiteTearDown(num_failures).
 */

#include <stdio.h>

int
test_suiteTearDown(int num_failures)
{
  if (num_failures == 0) {
    printf("\n*** ALL TESTS PASSED ***\n");
  } else {
    printf("\n*** %d TEST(S) FAILED ***\n", num_failures);
  }
  return num_failures;
}
