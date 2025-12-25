#include "unity.h"
#include "unity_mock_net.h"
#include <qwiet/platform/common.h>
#include <qwiet/platform/common/list.h>
#include <qwiet/platform/testing/diode/net_poll.h>

static struct pal_list_head __list;

static int
__verify_poll(struct pollfd *fds, int nfds, pal_timeout_t timeout, int ncalls)
{
  int ret, sz = sizeof(struct pollfd) * nfds;
  struct pal_list_head *node = NULL;
  struct poll_expectation *expect = NULL;
  // NOTE we must wait for sem before reading list
  // k_sem_take(&poll_ready, K_FOREVER);
  node = __list.next;
  TEST_ASSERT_NOT_NULL(node);
  expect = pal_list_entry(node, struct poll_expectation, node);
  TEST_ASSERT_EQUAL_INT(expect->nfds, nfds);
  TEST_ASSERT_EQUAL_INT(expect->ns, timeout.ns);
  TEST_ASSERT_EQUAL_MEMORY(fds, expect->fds, sz);
  memcpy(fds, &expect->fds[nfds], sz);
  ret = expect->ret;
  pal_list_del(&expect->node);
  pal_free(expect);
  return ret;
}

void
diode_poll_init(void)
{
  pal_list_init(&__list);
  __wrap_pal_net_socket_poll_Stub(__verify_poll);
}

void
diode_poll_cleanup(void)
{
  struct pal_list_head *pos, *n;
  struct poll_expectation *expectation;
  pal_list_for_each_safe(pos, n, &__list)
  {
    pal_list_del(pos); // unlink from list (not really deleting anything)
    expectation = pal_list_entry(pos, struct poll_expectation, node);
    pal_free(expectation);
  }
}

void
diode_poll_verify()
{
  TEST_ASSERT_TRUE_MESSAGE(pal_list_empty(&__list),
                           "poll called fewer times than expected");
}

struct poll_expectation *
diode_poll_create_expectation( //
    pal_timeout_t timeout,
    int n,
    char *events,
    char *revents,
    int ret,
    ...)
{
  va_list ap;
  int sz = sizeof(struct poll_expectation) + sizeof(struct pollfd) * (n << 1);
  struct poll_expectation *r = pal_malloc(sz);
  struct pollfd *ptr;
  r->nfds = n;
  r->ns = timeout.ns;
  r->ret = ret;
  va_start(ap, ret);
  for (int i = 0; i < n; i++) {
    ptr = r->fds;
    ptr[i].fd = ptr[i + n].fd = va_arg(ap, int);
    ptr[i].revents = 0;

    // Populate events
    if (events[i] == 'i')
      ptr[i].events = ptr[i + n].events = POLLIN;
    else if (events[i] == 'o')
      ptr[i].events = ptr[i + n].events = POLLOUT;
    else if (events[i] == 'z')
      ptr[i].events = ptr[i + n].events = (POLLIN | POLLOUT);
    else
      ptr[i].events = ptr[i + n].events = 0;

    // Populate revents
    // NOTE we use ret here and not r->ret because ret == 0
    //      captures if we have events are not. Once we start looping we
    //      cant use heap->poll.ret == 0 check anymore
    if (ret == 0 && revents) {
      if (revents[i] == 'i') {
        r->ret++;
        ptr[i + n].revents = POLLIN;
      } else if (revents[i] == 'o') {
        r->ret++;
        ptr[i + n].revents = POLLOUT;
      } else if (revents[i] == 'z') {
        r->ret++;
        ptr[i + n].revents = (POLLIN | POLLOUT);
      } else if (revents[i] == 'e') {
        r->ret++;
        ptr[i + n].revents = POLLERR;
      } else {
        ptr[i + n].revents = 0;
      }
    } else {
      ptr[i + n].revents = 0;
    }
  }
  va_end(ap);

  // Add this to our list to be free'd later when tests are complete
  pal_list_init(&r->node);
  pal_list_add(&r->node, &__list);
  return r;
}
