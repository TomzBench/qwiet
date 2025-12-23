#include <stdbool.h>
#include <unity.h>

#include "stubs.h"
#include "unity_mock___wrap_stubs.h"

void
setUp(void)
{
}

void
tearDown(void)
{
}

void
test_sanity_check(void)
{
  TEST_ASSERT_TRUE(true);
}

void
test_mock_get_value(void)
{
  /* Set expectation: stub_get_value() will return 42 */
  __wrap_stub_get_value_ExpectAndReturn(42);

  /* Call the mocked function */
  int result = stub_get_value();

  TEST_ASSERT_EQUAL_INT(42, result);
}

void
test_mock_add(void)
{
  /* Set expectation: stub_add(10, 20) will return 30 */
  __wrap_stub_add_ExpectAndReturn(10, 20, 30);

  /* Call the mocked function */
  int result = stub_add(10, 20);

  TEST_ASSERT_EQUAL_INT(30, result);
}

extern int
unity_main(void);

int
main(void)
{
  return unity_main();
}
