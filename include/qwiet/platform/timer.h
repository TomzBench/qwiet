/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Timer abstraction using timerfd on Linux.
 */
#ifndef QWIET_TIMER_H
#define QWIET_TIMER_H

#include <qwiet/platform/common.h>
#include <qwiet/platform/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int fd;
} pal_timer_t;

void
pal_timer_init(pal_timer_t *timer);

void
pal_timer_start_oneshot(pal_timer_t *timer, pal_timeout_t duration);

void
pal_timer_start_periodic(pal_timer_t *timer,
                         pal_timeout_t delay,
                         pal_timeout_t period);

void
pal_timer_stop(pal_timer_t *timer);

int
pal_timer_fd(pal_timer_t *timer);

uint64_t
pal_timer_ack(pal_timer_t *timer);

bool
pal_timer_is_ready(pal_timer_t *timer);

int
pal_timer_wait_ready(pal_timer_t *timer, pal_timeout_t timeout);

void
pal_timer_cleanup(pal_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif
