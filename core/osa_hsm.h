
#ifndef __OSA_HSM_H__
#define __OSA_HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hsm.h"
#include "osa_timer.h"

#if defined(__linux__)
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
struct hsm_dispatcher
{
    pthread_t task_ref;        // event dispatcher thread reference
    mqd_t     msg_q;           // event fifo msg queue
    int       msg_q_size;      // event fifo size
};
#else                          // rtos system
struct hsm_dispatcher
{
    task_t   task_ref;        // event dispatcher task reference
    int      msg_q_size;      // event fifo size
    queue_t  msg_q;           // event fifo msg queue
};
#endif

struct hsm_timer
{
    timer_t timer_id;            // timer id
    int     tick_period;         // timer tick period ms     
};

struct osa_hsm_active
{
    struct hsm_active super;     // super class
    char* name;                  // hsm name string

    struct hsm_dispatcher disp;  // os hsm event dispatcher
    struct hsm_timer      timer; // timer period event tick for hsm
};

const char* oas_hsm_sig_str(int signal);

int osa_hsm_active_init(struct osa_hsm_active *hsm, struct hsm_state* init);

int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size);

int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout);

int osa_hsm_active_period(struct osa_hsm_active *hsm, uint32_t period);

#ifdef __cplusplus
}
#endif

#endif //__HSM_H__