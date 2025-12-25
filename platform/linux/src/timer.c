#include <sys/timerfd.h>
#include <unistd.h>

#include <qwiet/platform/timer.h>

void
pal_timer_init(pal_timer_t *timer)
{
  timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  pal_assert(timer->fd >= 0, "timerfd_create failed");
}

static void
timer_start(pal_timer_t *timer, pal_timeout_t duration, pal_timeout_t period)
{
  struct itimerspec its;
  pal_timeout_to_timespec(duration, &its.it_value);
  pal_timeout_to_timespec(period, &its.it_interval);

  int ret = timerfd_settime(timer->fd, 0, &its, NULL);
  pal_assert(ret == 0, "timerfd_settime failed");
}

void
pal_timer_start_oneshot(pal_timer_t *timer, pal_timeout_t duration)
{
  timer_start(timer, duration, PAL_NO_WAIT);
}

void
pal_timer_start_periodic(pal_timer_t *timer,
                         pal_timeout_t delay,
                         pal_timeout_t period)
{
  timer_start(timer, delay, period);
}

void
pal_timer_stop(pal_timer_t *timer)
{
  struct itimerspec its = {0};
  int ret = timerfd_settime(timer->fd, 0, &its, NULL);
  pal_assert(ret == 0, "timerfd_settime failed");
}

int
pal_timer_fd(pal_timer_t *timer)
{
  return timer->fd;
}

uint64_t
pal_timer_ack(pal_timer_t *timer)
{
  uint64_t expirations;
  ssize_t ret = read(timer->fd, &expirations, sizeof(expirations));
  return ret == sizeof(expirations) ? expirations : 0;
}

bool
pal_timer_is_ready(pal_timer_t *timer)
{
  struct pollfd pfd = {.fd = timer->fd, .events = POLLIN};
  int ret = poll(&pfd, 1, 0);
  return ret > 0 && (pfd.revents & POLLIN);
}

int
pal_timer_wait_ready(pal_timer_t *timer, pal_timeout_t timeout)
{
  struct pollfd pfd = {.fd = timer->fd, .events = POLLIN};
  int ms = pal_timeout_to_ms(timeout);
  int ret = poll(&pfd, 1, ms);
  return ret > 0 && (pfd.revents & POLLIN) ? 1 : ret < 0 ? -1 : ret;
}

void
pal_timer_cleanup(pal_timer_t *timer)
{
  close(timer->fd);
}
