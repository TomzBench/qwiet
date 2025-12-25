#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <qwiet/platform/net.h>
#include <sys/socket.h>
#include <unistd.h>

int
pal_net_socket_tcp(bool non_blocking)
{
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  pal_assert(sock >= 0,
             "failed to create a %s tcp socket",
             non_blocking ? "non_blocking" : "blocking");
  pal_net_socket_set_nonblocking(sock, non_blocking);
  return sock;
}

void
pal_net_socketpair(bool non_blocking, int sv[2])
{
  int err = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  pal_assert(err == 0,
             "failed to create a %s socket pair",
             non_blocking ? "non_blocking" : "blocking");
  pal_net_socket_set_nonblocking(sv[0], non_blocking);
  pal_net_socket_set_nonblocking(sv[1], non_blocking);
}

int
pal_net_socket_poll(struct pollfd *fds, int nfds, pal_timeout_t timeout)
{
  int ms = pal_timeout_to_ms(timeout);
  return poll(fds, nfds, ms);
}

int
pal_net_socket_ready(int sock)
{
  int optval;
  socklen_t optlen = sizeof(optval);

  int ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen);
  if (ret < 0) {
    return -1;
  } else if (optval == 0) {
    return 1; /* ready */
  } else if (optval == EINPROGRESS) {
    return 0; /* still connecting */
  } else {
    return -1; /* error */
  }
}

void
pal_net_socket_set_nonblocking(int sock, bool non_blocking)
{
  int flags = fcntl(sock, F_GETFL, 0);
  pal_assert(flags >= 0, "fcntl F_GETFL failed on sock %d", sock);

  if (non_blocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  int ret = fcntl(sock, F_SETFL, flags);
  pal_assert(ret == 0, "fcntl F_SETFL failed on sock %d", sock);
}

int
pal_net_send(int sock, const void *buf, uint16_t len, int flags)
{
  return (int)send(sock, buf, len, flags);
}

int
pal_net_recv(int sock, uint8_t *buf, uint16_t len, int flags)
{
  return (int)recv(sock, buf, len, flags);
}

int
pal_net_connect(int sock, const char *ip, int port)
{
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
    return -1; /* invalid IP */
  }

  int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == 0) {
    return 1; /* connected immediately */
  } else if (errno == EINPROGRESS) {
    return 0; /* connection in progress (non-blocking) */
  } else {
    return -1; /* error */
  }
}

int
pal_net_listen(int sock, int port, int backlog)
{
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);
  addr.sin_addr.s_addr = INADDR_ANY;

  int opt = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return -1;
  } else {
    return listen(sock, backlog);
  }
}

int
pal_net_close(int sock)
{
  return close(sock);
}
