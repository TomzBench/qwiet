#ifndef QWIET_NET_H
#define QWIET_NET_H

#include <qwiet/platform/common.h>
#include <qwiet/platform/time.h>

#ifdef __cplusplus
extern "C" {
#endif

int
pal_net_socket_tcp(bool non_blocking);

void
pal_net_socketpair(bool non_blocking, int sv[2]);

int
pal_net_socket_poll(struct pollfd *, int, pal_timeout_t timeout);

int
pal_net_socket_ready(int sock);

void
pal_net_socket_set_nonblocking(int sock, bool non_blocking);

int
pal_net_send(int sock, const void *buf, uint16_t len, int flags);

int
pal_net_recv(int sock, uint8_t *buf, uint16_t len, int flags);

int
pal_net_connect(int sock, const char *ip, int port);

int
pal_net_listen(int sock, int port, int);

int
pal_net_close(int sock);

#ifdef __cplusplus
}
#endif

#endif
