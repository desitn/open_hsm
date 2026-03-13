/**
 * @file hsm.h
 * @brief Hierarchical State Machine (HSM) Core Interface
 *
 * This file defines the core HSM framework interface including:
 * - Signal/event definitions
 * - State structure and handler type
 * - Active state machine structure
 * - State transition macros
 *
 * The HSM framework supports hierarchical state nesting, entry/exit actions,
 * and periodic timer events.
 */

#ifndef __HSM_H__
#define __HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"

/**
 * @brief HSM basic signal enumeration
 *
 * These are the built-in signals used by the HSM framework.
 * User-defined signals should start from HSM_USER_SIG.
 */
enum hsm_signal
{
    HSM_SIG_INIT,       /**< Initial transition signal */
    HSM_SIG_ENTRY,      /**< State entry signal */
    HSM_SIG_EXIT,       /**< State exit signal */
    HSM_SIG_PERIOD,     /**< Periodic timer tick signal */

    HSM_USER_SIG,       /**< Start of user-defined signals */
};

/**
 * @brief HSM event structure
 *
 * Events are passed to state handlers and contain a signal plus
 * optional user data.
 */
struct hsm_event
{
    int signal;         /**< Event signal (from hsm_signal or user-defined) */
    void* data;         /**< Pointer to user data */
    int   size;         /**< Size of user data in bytes */
};

/**
 * @brief HSM state handler function type
 *
 * @param[in] entry     Pointer to the active HSM structure
 * @param[in] event     Event to process
 * @return              0 on success, negative error code on failure
 */
typedef int (*hsm_state_handler)(void* entry, const struct hsm_event* event);

/**
 * @brief HSM state structure
 *
 * Defines a state with its handler, parent state (for hierarchy),
 * and a name for debugging.
 */
struct hsm_state
{
    hsm_state_handler handler;      /**< State handler function */
    struct hsm_state* parent;       /**< Parent state (NULL for top-level) */
    const char* name;               /**< State name for debugging */
};

/**
 * @brief HSM active instance structure
 *
 * Represents a running HSM instance with current state,
 * history state, and error status.
 */
struct hsm_active
{
    int error;                      /**< Last error code */
    struct hsm_state* current;      /**< Current active state */
    struct hsm_state* histroy;      /**< Previous state (history) */
};

/** @brief Define state entry pointer and information */
#define STATE_ENTRY(a)           struct hsm_active* entry = a

/** @brief Get current state name */
#define STATE_NAME()             entry->current->name

/** @brief Get last error code */
#define STATE_ERROR()            entry->error

/** @brief Trigger entry action for current state */
#define STATE_INIT()             entry->current->handler(entry, &(struct hsm_event){HSM_SIG_ENTRY, NULL})

/**
 * @brief Dispatch event to a state
 *
 * @param[in] s     Target state
 * @param[in] e     Event to dispatch
 */
#define STATE_DISPATCH(s, e)    (s)->handler(entry, e)

/**
 * @brief Transition to a new state
 *
 * Performs exit of current state, updates history, enters new state.
 *
 * @param[in] s     Target state to transition to
 */
#define STATE_TRANSIT(s)        do {                                                  \
    ((struct hsm_active*)entry)->histroy = ((struct hsm_active*)entry)->current;      \
    (((struct hsm_active*)entry)->current) = (s);                                     \
    ((struct hsm_active*)entry)->error = STATE_INIT();                                \
} while(0)

/**
 * @brief Delegate event to parent state
 *
 * If the current state has a parent, the event is passed to the
 * parent state's handler.
 *
 * @param[in] e     Event to delegate
 */
#define STATE_SUPER(e)          do {                                                 \
    if(((struct hsm_active*)entry)->current->parent) {                               \
        ((struct hsm_active*)entry)->error =                                         \
        ((struct hsm_active*)entry)->current->parent->handler(entry, e);             \
    }                                                                                \
} while(0)


#ifdef __cplusplus
}
#endif

#endif //__HSM_H__
