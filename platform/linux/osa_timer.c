/**
 * @file osa_timer.c
 * @brief Linux implementation of OS Abstraction Layer - Timer
 *
 * This implementation uses POSIX timers (timer_create/timer_settime).
 * The callback is invoked via SIGEV_THREAD notification.
 */

#include "osa_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

/* Timer implementation structure */
struct osa_timer_impl {
    timer_t timer_id;               /* POSIX timer ID */
    osa_timer_cb_t callback;        /* User callback */
    void *arg;                      /* User argument */
    bool active;                    /* Timer active flag */
};

/* Wrapper callback for POSIX timer to match osa_timer_cb_t signature */
static void posix_timer_callback(union sigval sv)
{
    struct osa_timer_impl *timer = (struct osa_timer_impl *)sv.sival_ptr;
    if (timer && timer->callback) {
        timer->callback(timer->arg);
    }
}

int osa_timer_create(osa_timer_t *timer, unsigned int period_ms,
                     bool cycle, osa_timer_cb_t callback, void *arg)
{
    struct osa_timer_impl *t;
    struct sigevent se;
    struct itimerspec its;

    if (!timer || !callback || period_ms == 0) {
        return -EINVAL;
    }

    /* Allocate timer structure */
    t = (struct osa_timer_impl *)calloc(1, sizeof(struct osa_timer_impl));
    if (!t) {
        return -ENOMEM;
    }

    t->callback = callback;
    t->arg = arg;
    t->active = true;

    /* Setup sigevent for thread notification */
    memset(&se, 0, sizeof(struct sigevent));
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = t;
    se.sigev_notify_function = posix_timer_callback;
    se.sigev_notify_attributes = NULL;

    /* Create timer */
    if (timer_create(CLOCK_MONOTONIC, &se, &t->timer_id) == -1) {
        free(t);
        return -errno;
    }

    /* Calculate time values */
    time_t sec = period_ms / 1000;
    long nsec = (period_ms % 1000) * 1000000;

    /* Configure timer */
    memset(&its, 0, sizeof(struct itimerspec));
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = nsec;

    if (cycle) {
        its.it_interval.tv_sec = sec;
        its.it_interval.tv_nsec = nsec;
    }

    /* Start timer */
    if (timer_settime(t->timer_id, 0, &its, NULL) == -1) {
        timer_delete(t->timer_id);
        free(t);
        return -errno;
    }

    *timer = (osa_timer_t)t;
    return 0;
}

int osa_timer_stop(osa_timer_t timer)
{
    struct osa_timer_impl *t = (struct osa_timer_impl *)timer;
    struct itimerspec its;

    if (!t) {
        return -EINVAL;
    }

    /* Stop timer by setting zero interval and value */
    memset(&its, 0, sizeof(struct itimerspec));
    if (timer_settime(t->timer_id, 0, &its, NULL) == -1) {
        return -errno;
    }

    t->active = false;
    return 0;
}

int osa_timer_delete(osa_timer_t timer)
{
    struct osa_timer_impl *t = (struct osa_timer_impl *)timer;

    if (!t) {
        return -EINVAL;
    }

    /* Delete POSIX timer */
    if (timer_delete(t->timer_id) == -1) {
        return -errno;
    }

    free(t);
    return 0;
}
