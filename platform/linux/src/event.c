#include <curriculo/platform/event.h>
#include <sys/eventfd.h>
#include <unistd.h>

int
pal_event_fd(void)
{
  int fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
  pal_assert(fd >= 0, "failed to create eventfd");
  return fd;
}

int
pal_event_write(int fd, uint64_t val)
{
  ssize_t ret = write(fd, &val, sizeof(val));
  if (ret == sizeof(val)) {
    return 0;
  } else {
    return -1;
  }
}

int
pal_event_read(int fd, uint64_t *val)
{
  ssize_t ret = read(fd, val, sizeof(*val));
  if (ret == sizeof(*val)) {
    return 0;
  } else {
    return -1; /* EAGAIN if nothing to read, or error */
  }
}
