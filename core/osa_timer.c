#include "osa_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int osa_timer_create(timer_t *timer_id, unsigned int period_ms, 
                     int cycle_enable, timer_callback_t handler, void *arg)
{
    struct sigevent se;
    struct itimerspec its;
    struct sigaction sa;

    if (timer_id == NULL || handler == NULL) {
        fprintf(stderr, "osa_timer_create: Invalid parameters\n");
        return -1;
    }
    // Setup signal handler
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }
    // Create timer
    memset(&se, 0, sizeof(struct sigevent));
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGRTMIN;
    se.sigev_value.sival_ptr = arg;

    if (timer_create(CLOCK_REALTIME, &se, timer_id) == -1) {
        perror("timer_create");
        return -1;
    }

    // 1 millisecond = 1,000,000 nanoseconds
    time_t sec = period_ms / 1000;
    long nsec = (period_ms % 1000) * 1000000;

    // Configure timer
    memset(&its, 0, sizeof(struct itimerspec));
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = nsec;

    if (cycle_enable) {
        its.it_interval.tv_sec = sec;
        its.it_interval.tv_nsec = nsec;
    }

    if (timer_settime(*timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        timer_delete(*timer_id);
        return -1;
    }

    return 0;
}

int osa_timer_stop(timer_t timer_id)
{
    struct itimerspec its;
    // Set timer to stop by setting it_value and it_interval to zero
    memset(&its, 0, sizeof(struct itimerspec));
    if (timer_settime(timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        return -1;
    }

    return 0;
}

int osa_timer_delete(timer_t timer_id)
{
    if (timer_delete(timer_id) == -1) {
        perror("timer_delete");
        return -1;
    }
    return 0;
}