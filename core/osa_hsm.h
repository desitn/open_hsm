/*********************************************************************
* @file   osa_hsm.h
* @brief  hierarchical state machine os layer 
* @author destin.zhang@quectel.com
* @date   2025-02-22
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-02-22 <td>1.0 <td>destin.zhang <td> Init
* </table>
**********************************************************************/


#ifndef __OSA_HSM_H__
#define __OSA_HSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hsm.h"

#if defined(__linux__)
#include <pthread.h>
#include <mqueue.h>
#include <sys/types.h>
#include <unistd.h>
#endif

struct osa_hsm_active
{
    struct hsm_active super;  // super class
    char* name;              // hsm name string

#if defined(__linux__)
    mqd_t msg_q;
    int   msg_q_size;
    pthread_t task_ref;      // event dispatcher thread reference
#else
    queue_t msg_q;           // event fifo msg queue
    task_t  task_ref;        // event dispatcher task reference
#endif

};

int osa_hsm_active_init(struct osa_hsm_active *hsm, struct hsm_state* init);

int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size);

int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif //__HSM_H__