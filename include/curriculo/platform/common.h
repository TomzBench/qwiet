#ifndef CURRICULO_COMMON_H
#define CURRICULO_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PLATFORM_LINUX
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>  /* IWYU pragma: keep (pal_assert) */
#include <stdlib.h> /* IWYU pragma: keep (pal_assert) */
#include <sys/poll.h>

#define pal_assert(cond, fmt, ...)                                             \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr,                                                          \
              "ASSERT FAILED: %s:%d (%s): " fmt "\n",                          \
              __FILE__,                                                        \
              __LINE__,                                                        \
              __func__,                                                        \
              ##__VA_ARGS__);                                                  \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif /* PLATFORM_LINUX */

#ifdef __cplusplus
}
#endif

#endif
