/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Timeout representation and construction macros.
 *
 * API design inspired by Zephyr RTOS (include/zephyr/sys_clock.h).
 * Original authors: Zephyr Project contributors
 */
#ifndef QWIET_TIME_H
#define QWIET_TIME_H

#include <qwiet/platform/common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int64_t ns; /* nanoseconds: -1 = forever, 0 = no wait, >0 = duration */
} pal_timeout_t;

/* Construction macros */
#define PAL_NO_WAIT ((pal_timeout_t){.ns = 0})
#define PAL_FOREVER ((pal_timeout_t){.ns = -1})
#define PAL_NSEC(n) ((pal_timeout_t){.ns = (int64_t)(n)})
#define PAL_USEC(u) ((pal_timeout_t){.ns = (int64_t)(u) * 1000LL})
#define PAL_MSEC(m) ((pal_timeout_t){.ns = (int64_t)(m) * 1000000LL})
#define PAL_SEC(s) ((pal_timeout_t){.ns = (int64_t)(s) * 1000000000LL})

/* Predicates */
static inline bool
pal_timeout_is_forever(pal_timeout_t t)
{
  return t.ns < 0;
}

static inline bool
pal_timeout_is_nowait(pal_timeout_t t)
{
  return t.ns == 0;
}

int
pal_timeout_to_ms(pal_timeout_t t);

void
pal_timeout_to_abs_timespec(pal_timeout_t t, struct timespec *out);

void
pal_timeout_to_timespec(pal_timeout_t t, struct timespec *out);

void
pal_sleep(pal_timeout_t duration);

#ifdef __cplusplus
}
#endif

#endif
