/*********************************************************************
* @file   hsm.h
* @brief  hierarchical state machine
* @author destin.zhang@quectel.com
* @date   2025-02-22
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-02-22 <td>1.0 <td>destin.zhang <td> Init
* </table>
**********************************************************************/

#ifndef __HSM_H__
#define __HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"

/*  hsm basic signal enum */
enum hsm_signal
{
    HSM_SIG_INIT,
    HSM_SIG_ENTRY,
    HSM_SIG_EXIT,
    HSM_SIG_PERIOD, // timer tick period

    HSM_USER_SIG,
};

/*  hsm events structure: signal with user data ptr */
struct hsm_event
{
    int signal;
    void* data;
    int   size;
};

/* hsm state handler */
typedef int (*hsm_state_handler)(void* entry, const struct hsm_event* event);

/* hsm state structure */
struct hsm_state
{
    hsm_state_handler handler;
    struct hsm_state* parent;
    const char* name;
};

/* hsm ative entry structure */
struct hsm_active
{
    int error;
    struct hsm_state* current;
    struct hsm_state* histroy;
};

/** @note  state entry ptr define &information */
#define STATE_ENTRY(a)           struct hsm_active* entry = a
#define STATE_NAME()             entry->current->name
#define STATE_ERROR()            entry->error

/** @note  state entry event */
#define STATE_INIT()             entry->current->handler(entry, &(struct hsm_event){HSM_SIG_ENTRY, NULL})

/** @note fifo msg dispatch event(e) for current state(s) */
#define STATE_DISPATCH(s, e)    (s)->handler(entry, e)


/** @note current state trans to next state(s) */
#define STATE_TRANSIT(s)        do {                                                  \
    ((struct hsm_active*)entry)->histroy = ((struct hsm_active*)entry)->current;      \
    (((struct hsm_active*)entry)->current) = (s);                                     \
    ((struct hsm_active*)entry)->error = STATE_INIT();                                \
} while(0)

/** @note current state's super state handle event(e) */
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