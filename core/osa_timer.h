#ifndef OSA_TIMER_H
#define OSA_TIMER_H

#include <time.h>
#include <signal.h>

typedef void (*timer_callback_t)(int sig, siginfo_t *si, void *uc);

/**
 * Create and start a POSIX timer
 * @param timer_id: pointer to timer_t to store the timer ID
 * @param period_ms: timer period in milliseconds
 * @param cycle_enable: 1 for periodic timer, 0 for one-shot timer
 * @param handler: callback function to handle timer expiration
 * @param arg: user-defined argument passed to handler
 * @return 0 on success, -1 on failure
 */
int osa_timer_create(timer_t *timer_id, unsigned int period_ms, 
                     int cycle_enable, timer_callback_t handler, void *arg);

 /**
 * Stops the specified timer by disabling its periodic execution.
 * This function sets the timer's interval and value to zero, effectively stopping it.
 * 
 * @param timer_id The identifier of the timer to stop.
 * @return 0 on success, -1 on failure with an error message printed to stderr.
 */
int osa_timer_stop(timer_t timer_id);

/**
 * Delete a POSIX timer
 * @param timer_id: timer ID to delete
 * @return 0 on success, -1 on failure
 */
int osa_timer_delete(timer_t timer_id);

#endif /* OSA_TIMER_H */