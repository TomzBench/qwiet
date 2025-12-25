#include <stdbool.h>
#include <unity.h>

#include <qwiet/platform/time.h>
#include <qwiet/platform/timer.h>

pal_timer_t test_timer;

void
setUp(void)
{
  pal_timer_init(&test_timer);
}

void
tearDown(void)
{
  pal_timer_cleanup(&test_timer);
}

void
test_timer_oneshot(void)
{
  pal_timer_start_oneshot(&test_timer, PAL_MSEC(20));
  TEST_ASSERT_FALSE(pal_timer_is_ready(&test_timer));

  /* Wait for expiration */
  pal_sleep(PAL_MSEC(30));
  TEST_ASSERT_TRUE(pal_timer_is_ready(&test_timer));

  /* Ack the expiration */
  TEST_ASSERT_EQUAL_UINT64(1, pal_timer_ack(&test_timer));
  TEST_ASSERT_FALSE(pal_timer_is_ready(&test_timer));

  /* Oneshot should not fire again */
  pal_sleep(PAL_MSEC(30));
  TEST_ASSERT_FALSE(pal_timer_is_ready(&test_timer));
}

void
test_timer_periodic(void)
{
  pal_timer_start_periodic(&test_timer, PAL_MSEC(20), PAL_MSEC(20));
  TEST_ASSERT_FALSE(pal_timer_is_ready(&test_timer));

  /* Wait for first expiration */
  TEST_ASSERT_EQUAL_INT(1, pal_timer_wait_ready(&test_timer, PAL_MSEC(50)));
  TEST_ASSERT_TRUE(pal_timer_is_ready(&test_timer));

  /* Ack the expiration */
  TEST_ASSERT_TRUE(pal_timer_ack(&test_timer) >= 1);
  TEST_ASSERT_FALSE(pal_timer_is_ready(&test_timer));

  /* Wait for second expiration */
  TEST_ASSERT_EQUAL_INT(1, pal_timer_wait_ready(&test_timer, PAL_MSEC(50)));
  TEST_ASSERT_TRUE(pal_timer_is_ready(&test_timer));

  /* Ack again */
  TEST_ASSERT_TRUE(pal_timer_ack(&test_timer) >= 1);
  TEST_ASSERT_FALSE(pal_timer_is_ready(&test_timer));
}

extern int
unity_main(void);

int
main(void)
{
  return unity_main();
}
