/**
 * @file osa_timer.h
 * @brief OS Abstraction Layer - Timer
 *
 * Timer abstraction interface compatible with typical RTOS APIs.
 * Platform-specific implementations are in platform/<platform>/ directories.
 */

#ifndef __OSA_TIMER_H__
#define __OSA_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Timer handle - platform specific implementation */
typedef void* osa_timer_t;

/* Timer callback function type */
typedef void (*osa_timer_cb_t)(void *arg);

/**
 * @brief Create and start a timer
 *
 * @param[out] timer        Pointer to store the created timer handle
 * @param[in] period_ms     Timer period in milliseconds
 * @param[in] cycle         true for periodic timer, false for one-shot
 * @param[in] callback      Callback function to handle timer expiration
 * @param[in] arg           User-defined argument passed to callback
 *
 * @return 0 on success, negative error code on failure
 */
int osa_timer_create(osa_timer_t *timer, unsigned int period_ms,
                     bool cycle, osa_timer_cb_t callback, void *arg);

/**
 * @brief Stop a timer
 *
 * Stops the timer from firing. The timer can be restarted by calling
 * osa_timer_create again or by platform-specific restart function.
 *
 * @param[in] timer         Timer handle to stop
 *
 * @return 0 on success, negative error code on failure
 */
int osa_timer_stop(osa_timer_t timer);

/**
 * @brief Delete a timer
 *
 * Stops and deletes the timer, freeing all associated resources.
 *
 * @param[in] timer         Timer handle to delete
 *
 * @return 0 on success, negative error code on failure
 */
int osa_timer_delete(osa_timer_t timer);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_TIMER_H__ */
