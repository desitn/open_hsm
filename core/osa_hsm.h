/**
 * @file osa_hsm.h
 * @brief OS Abstraction Layer - Active HSM Interface
 *
 * This file extends the core HSM framework with OS-specific features:
 * - Thread-based event dispatching
 * - Message queue for event posting
 * - Periodic timer support
 *
 * An "active" HSM runs in its own thread and receives events via
 * a message queue, enabling asynchronous event processing.
 */

#ifndef __OSA_HSM_H__
#define __OSA_HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hsm.h"
#include "../platform/osa_platform.h"

/**
 * @brief HSM event dispatcher structure
 *
 * Manages the thread and message queue for an active HSM.
 * Each active HSM has one dispatcher.
 */
struct hsm_dispatcher
{
    osa_thread_t task_ref;           /**< Event dispatcher thread handle */
    osa_queue_t  msg_q;              /**< Event message queue handle */
    int       msg_q_size;            /**< Maximum queue capacity */
};

/**
 * @brief HSM timer structure
 *
 * Manages a periodic timer that posts HSM_SIG_PERIOD events
 * to the HSM's message queue.
 */
struct hsm_timer
{
    osa_timer_t timer_id;        /**< OS timer handle */
    int     tick_period;         /**< Timer period in milliseconds */
};

/**
 * @brief Active HSM structure
 *
 * Extends the base HSM with OS abstraction features.
 * This is the main structure for creating thread-based state machines.
 */
struct osa_hsm_active
{
    struct hsm_active super;     /**< Base HSM structure (inheritance) */
    char* name;                  /**< HSM name for debugging */

    struct hsm_dispatcher disp;  /**< Event dispatcher (thread + queue) */
    struct hsm_timer      timer; /**< Periodic timer */
};

/**
 * @brief Get string representation of an HSM signal
 *
 * @param[in] signal    HSM signal value
 * @return              Human-readable signal name
 */
const char* oas_hsm_sig_str(int signal);

/**
 * @brief Initialize an active HSM
 *
 * Initializes the base HSM structure and sets the initial state.
 * This must be called before osa_hsm_active_start().
 *
 * @param[in,out] hsm   Active HSM to initialize
 * @param[in] init      Initial state
 * @return              0 on success, negative error code on failure
 */
int osa_hsm_active_init(struct osa_hsm_active *hsm, struct hsm_state* init);

/**
 * @brief Start an active HSM
 *
 * Creates the dispatcher thread and message queue.
 * The HSM begins processing events from its message queue.
 *
 * @param[in,out] hsm       Active HSM to start
 * @param[in] stack_size    Thread stack size in bytes
 * @param[in] priority      Thread priority (platform specific)
 * @param[in] msgq_size     Message queue capacity
 * @return                  0 on success, negative error code on failure
 */
int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size);

/**
 * @brief Post an event to an active HSM
 *
 * Sends an event to the HSM's message queue.
 * This is thread-safe and can be called from any context.
 *
 * @param[in,out] hsm       Target active HSM
 * @param[in] event         Event to post
 * @param[in] timeout       Timeout in milliseconds (0 = non-blocking, -1 = infinite)
 * @return                  0 on success, negative error code on failure
 */
int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout);

/**
 * @brief Set or clear periodic timer
 *
 * When period > 0, creates a timer that posts HSM_SIG_PERIOD events
 * at the specified interval. When period = 0, stops the timer.
 *
 * @param[in,out] hsm       Active HSM
 * @param[in] period        Timer period in milliseconds (0 to disable)
 * @return                  0 on success, negative error code on failure
 */
int osa_hsm_active_period(struct osa_hsm_active *hsm, uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_HSM_H__ */
