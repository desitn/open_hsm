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

#define UNUSED(x)                    (void)(x)

#define osa_log(msg, ...)	         printf(msg, ##__VA_ARGS__)
#define hsm_log(str, msg,...)        osa_log("[hsm|%s] "msg"",str,##__VA_ARGS__)
#define core_log(msg,...)            hsm_log("core", msg, ##__VA_ARGS__)

static const char* hsm_sig_str[HSM_USER_SIG] =
{
    [HSM_SIG_INIT]  = "init",
	[HSM_SIG_ENTRY] = "entry",
	[HSM_SIG_EXIT]  = "exit",
    [HSM_SIG_PERIOD]  = "period",
};

const char* oas_hsm_sig_str(int signal)
{
    const char* str =NULL;
    str =  (signal < HSM_USER_SIG) ? (hsm_sig_str[signal]) : ("unknown");
    return str;
}
int osa_hsm_active_init(struct osa_hsm_active *hsm, struct hsm_state* init)
{
    hsm->super.current = init;
    hsm->super.histroy = init;
    hsm->super.error   = 0;
    return 0;
}

#if defined(__linux__)
static void* osa_hsm_active_run(void *param)
{
    char name[32] = {0};
    struct hsm_state* state = NULL;
    struct osa_hsm_active *hsm = NULL;   
    struct hsm_dispatcher *disp = NULL;
    struct hsm_event event = {0};
    struct mq_attr attr = {0};

    /** @note hsm init state entery */
    STATE_ENTRY(param);
    STATE_INIT();

    /** @note hsm dispatcher mq init */ 
    hsm = (struct osa_hsm_active *)param;
    disp = &hsm->disp;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = disp->msg_q_size;
    attr.mq_msgsize = sizeof(struct hsm_event);
    attr.mq_curmsgs = 0;
    if (hsm->name != NULL) {
        snprintf(name, sizeof(name), "/hsm_%s_q", hsm->name);
    } else {
        snprintf(name, sizeof(name), "/hsm_%d_q", getpid());
    }
    disp->msg_q = mq_open(name, O_CREAT | O_RDWR, 0644, &attr);
    if (disp->msg_q == (mqd_t)-1) {
        core_log("mq:%s open create faid.\r\n", name);
        core_log("mount -t mqueue none /dev/mqueue or sudo run retry!\r\n");
        return NULL;
    }
    core_log("mq:%s create:%d.\r\n", name, disp->msg_q);

    while(1)
    {
        mq_receive(disp->msg_q, (char *)&event, sizeof(struct hsm_event), NULL);
        state = hsm->super.current;
        STATE_DISPATCH(state, &event);
    }

    return NULL;
}

int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size)
{
    int ret = 0;
    pthread_attr_t attr = {0};
    struct sched_param param = {0};
    struct hsm_dispatcher *disp = NULL;

    if (hsm == NULL) {
        return -1;
    }
    disp = &hsm->disp;
    disp->msg_q_size = msgq_size;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);
    param.sched_priority = priority;                // 优先级值（1\~99，值越大优先级越高）
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO); // FIFO实时调度
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&disp->task_ref, &attr, osa_hsm_active_run, hsm);
    if (ret != 0) {
        return -1;
    }
    pthread_detach(disp->task_ref);
    pthread_attr_destroy(&attr);

    return ret;

}

int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout)
{
    int ret = -1;
    struct hsm_dispatcher *disp = NULL;

    if (hsm == NULL) {
        return -1;
    }
    disp = &hsm->disp;
    if (disp->msg_q != -1) {
        ret = mq_send(disp->msg_q, (char *)event, sizeof(struct hsm_event), 0);
    } else {
        core_log("mq not open or exit!");
    }

    return ret;
}
void oas_hsm_timer_handler(int sig, siginfo_t *si, void *uc)
{
    struct hsm_event event = {0};
    struct osa_hsm_active *hsm = NULL;
    hsm = (struct osa_hsm_active *)si->si_value.sival_ptr;
    event.signal = HSM_SIG_PERIOD;
    osa_hsm_active_event_post(hsm, &event, 0);
}

int osa_hsm_active_period(struct osa_hsm_active *hsm, uint32_t period)
{
    int ret = -1;

    if (period != 0) {
        ret = osa_timer_create(&hsm->timer.timer_id, period, 1, oas_hsm_timer_handler, hsm);
    } else {
        ret = osa_timer_stop(hsm->timer.timer_id);
    }

    return ret;
}

#else //rtos system

static void osa_hsm_active_run(void *param)
{
    char name[32] = {0};
    struct osa_hsm_active *entry = NULL;
    struct hsm_state* state = NULL;
    struct hsm_event event = {0};

    /** @note hsm init state entery */
    STATE_ENTRY(param);
    STATE_INIT();

    while(1)
    {
        rtos_queue_wait(entry->msg_q, (uint8 *)&event, sizeof(struct hsm_event), WAIT_FOREVER);
        state = entry->super.current;
        STATE_DISPATCH(state, &event);
    }
}

int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size)
{
    int ret = 0;
    if (hsm == NULL){
        return -1;
    }

    char name[32] = {0};
    ret = rtos_queue_create(&hsm->msg_q, sizeof(struct hsm_event), msgq_size);
    if (ret != 0) {
        return ret;
    }
    if (hsm->name != NULL){
        snprintf(name, sizeof(name), "hsm_%s", hsm->name);
    }else {
        snprintf(name, sizeof(name), "hsm");
    }
    ret = rtos_task_create(&hsm->task_ref, stack_size, priority, name, osa_hsm_active_run, hsm, 0);
    if (ret != OSI_SUCCESS){
        rtos_queue_delete(&hsm->msg_q);
        return ret;
    }

    return ret;

}

int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout)
{
    return rtos_queue_release(hsm->msg_q, sizeof(struct hsm_event), (uint8 *)event, timeout);
}

void oas_hsm_timer_handler(void *params)
{
    struct hsm_event event = {0};
    struct osa_hsm_active *hsm = NULL;

    hsm = (struct osa_hsm_active *)params;
    event.signal = HSM_SIG_PERIOD;
    osa_hsm_active_event_post(hsm, &event, 0);
}

int osa_hsm_active_period(struct osa_hsm_active *hsm, uint32_t period)
{
    int ret = -1;

    if (period != 0) {
        ret = rtos_timer_create(&hsm->timer.timer_id, period, 1, oas_hsm_timer_handler, hsm);
    } else {
        ret = rtos_timer_stop(hsm->timer.timer_id);
    }

    return ret;
}
#endif