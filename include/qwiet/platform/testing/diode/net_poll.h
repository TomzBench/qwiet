#ifndef DIODE_POLL_H
#define DIODE_POLL_H

#include <qwiet/platform/common/list.h>
#include <qwiet/platform/net.h>

struct poll_expectation {
  struct pal_list_head node;
  int nfds, ret;
  int64_t ns;
  struct pollfd fds[];
};

#define EXPECT_NET_POLL_ERR(__timeout, __events, __e, ...)                     \
  EXPECT_NET_POLL(__timeout, __events, NULL, __e, __VA_ARGS__)

#define EXPECT_NET_POLL_OK(__timeout, __events, __revents, ...)                \
  EXPECT_NET_POLL(__timeout, __events, __revents, 0, __VA_ARGS__)

#define EXPECT_NET_POLL(__timeout, __events, __revents, __ret, ...)            \
  diode_poll_create_expectation(__timeout,                                     \
                                PAL_NUM_VA_ARGS(__VA_ARGS__),                  \
                                __events,                                      \
                                __revents,                                     \
                                __ret,                                         \
                                __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

void
diode_poll_init(void);

void
diode_poll_cleanup(void);

void
diode_poll_verify(void);

struct poll_expectation *
diode_poll_create_expectation( //
    pal_timeout_t timeout,
    int n,
    char *events,
    char *revents,
    int ret,
    ...);

#ifdef __cplusplus
}
#endif

#endif
