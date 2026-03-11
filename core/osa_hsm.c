/********************************************************************
* @file    osa_hsm.c
* @brief   hierarchical state machine os layer

* @author  destin.zhang@quectel.com
* @date    2025-04-24
*
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2023-10-14 <td>1.0 <td>destin.zhang <td> Init
* </table>
**********************************************************************/
#include "osa_hsm.h"

#define UNUSED(x)         (void)(x)

int osa_hsm_active_init(struct osa_hsm_active *hsm, struct hsm_state* init)
{
    hsm->super.current = init;
    hsm->super.histroy = init;
    hsm->super.error   = 0;
    return 0;
}

static void osa_hsm_active_run(void *param)
{
    char name[32] = {0};
    struct osa_hsm_active *entry = NULL;
    struct hsm_state* state = NULL;
    struct hsm_event event = {0};

    entry = (struct osa_hsm_active *)param;
    STATE_INIT(entry->super.current); // hsm init state enter;

#if defined(__linux__)  
    struct mq_attr attr = {0};
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = entry->msg_q_size;
    attr.mq_msgsize = sizeof(struct hsm_event);
    attr.mq_curmsgs = 0;
    if (entry->name != NULL) {
        snprintf(name, sizeof(name), "/hsm_%s_q", entry->name);
    } else {
        snprintf(name, sizeof(name), "/hsm_%d_q", getpid());
    }
    entry->msg_q = mq_open(name, O_CREAT | O_RDWR, 0644, &attr);
    if (entry->msg_q == (mqd_t)-1) {
        return;
    }
    printf("mq:%s create:%d.\r\n", name, entry->msg_q);
#endif

    while(1)
    {
#if defined(__linux__)
        mq_receive(entry->msg_q, (char *)&event, sizeof(struct hsm_event), NULL);
#else
        ql_rtos_queue_wait(entry->msg_q, (uint8 *)&event, sizeof(struct hsm_event), QL_WAIT_FOREVER);
#endif
        state = entry->super.current;
        STATE_DISPATCH(state, &event);
    }
}

#if defined(__linux__)
static void* osa_hsm_active_run_l(void *param)
{
    osa_hsm_active_run(param);
    return NULL;
}
#endif

int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size)
{
    int ret = 0;
#if defined(__linux__)
    pthread_attr_t attr;
    struct sched_param param;
#endif

    if (hsm == NULL){
        return -1;
    }
#if defined(__linux__)
    hsm->msg_q_size = msgq_size;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);
    param.sched_priority = priority;                // 优先级值（1\~99，值越大优先级越高）
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO); // FIFO实时调度
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&hsm->task_ref, &attr, osa_hsm_active_run_l, hsm);
    if (ret != 0) {
        return -1;
    }
    pthread_detach(hsm->task_ref);
    pthread_attr_destroy(&attr);
#else
    char name[32] = {0};
    ret = ql_rtos_queue_create(&hsm->msg_q, sizeof(struct hsm_event), msgq_size);
    if (ret != 0) {
        return ret;
    }
    if (hsm->name != NULL){
        snprintf(name, sizeof(name), "hsm_%s", hsm->name);
    }else {
        snprintf(name, sizeof(name), "hsm");
    }
    ret = ql_rtos_task_create(&hsm->task_ref, stack_size, priority, name, osa_hsm_active_run, hsm, 0);
    if (ret != QL_OSI_SUCCESS){
        ql_rtos_queue_delete(&hsm->msg_q);
        return ret;
    }
#endif

    return ret;

}

int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout)
{
#if defined(__linux__)
    int ret = -1;
    if(hsm->msg_q != -1) {
        ret = mq_send(hsm->msg_q, (char *)event, sizeof(struct hsm_event), 0);
    } else {
        printf("mq not open or exit!");
    }
    return ret;
#else
    return ql_rtos_queue_release(hsm->msg_q, sizeof(struct hsm_event), (uint8 *)event, timeout);
#endif
}
