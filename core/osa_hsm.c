/**
 * @file osa_hsm.c
 * @brief Platform-independent HSM implementation
 *
 * This file contains the platform-independent HSM implementation.
 * All platform-specific operations are abstracted through osa_platform.h
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "osa_hsm.h"

#define UNUSED(x)                    (void)(x)

#define osa_log(msg, ...)	         printf(msg, ##__VA_ARGS__)
#define hsm_log(str, msg,...)        osa_log("[hsm|%s] "msg"",str,##__VA_ARGS__)
#define core_log(msg,...)            hsm_log("core", msg, ##__VA_ARGS__)

static const char* hsm_sig_str[HSM_USER_SIG] =
{
    [HSM_SIG_INIT]   = "init",
	[HSM_SIG_ENTRY]  = "entry",
	[HSM_SIG_EXIT]   = "exit",
    [HSM_SIG_PERIOD] = "period",
};

const char* oas_hsm_sig_str(int signal)
{
    const char* str = NULL;
    str = (signal < HSM_USER_SIG) ? (hsm_sig_str[signal]) : ("unknown");
    return str;
}

int osa_hsm_active_init(struct osa_hsm_active *hsm, struct hsm_state* init)
{
    hsm->super.current = init;
    hsm->super.histroy = init;
    hsm->super.error   = 0;
    return 0;
}

/**
 * @brief HSM event dispatcher thread entry
 *
 * This is the main event loop for an active HSM.
 */
static void osa_hsm_active_run(void *param)
{
    int ret = 0;
    char name[32] = {0};
    struct hsm_state* state = NULL;
    struct osa_hsm_active *hsm = NULL;
    struct hsm_dispatcher *disp = NULL;
    struct hsm_event event = {0};

    /** @note HSM init state entry */
    STATE_ENTRY(param);
    STATE_INIT();

    /** @note HSM dispatcher msg queue init */
    hsm = (struct osa_hsm_active *)param;
    disp = &hsm->disp;
    if (hsm->name != NULL) {
        snprintf(name, sizeof(name), "hsm_%s_q", hsm->name);
    } else {
        snprintf(name, sizeof(name), "hsm_%d_q", (int)getpid());
    }
    ret = osa_queue_create(&disp->msg_q, name, sizeof(struct hsm_event), disp->msg_q_size);
    if (ret != 0) {
        core_log("msg queue:%s create failed, ret=%d.\r\n", name, ret);
        osa_thread_exit();
        return;
    }
    core_log("msg queue:%s created.\r\n", name);

    while(1)
    {
        ret = osa_queue_receive(disp->msg_q, &event, sizeof(struct hsm_event), -1);
        if (ret != 0) {
            if (ret != -ETIMEDOUT) {
                core_log("msg queue receive failed: %d\n", ret);
            }
            continue;
        }
        state = hsm->super.current;
        STATE_DISPATCH(state, &event);
    }

    osa_thread_exit();
}

int osa_hsm_active_start(struct osa_hsm_active *hsm, int stack_size, int priority, int msgq_size)
{
    int ret = 0;
    struct osa_thread_param thread_param = {0};
    struct hsm_dispatcher *disp = NULL;

    if (hsm == NULL) {
        return -1;
    }

    disp = &hsm->disp;
    disp->msg_q_size = msgq_size;

    /* Setup thread parameters */
    thread_param.name = hsm->name;
    thread_param.stack_size = stack_size;
    thread_param.priority = priority;
    thread_param.stack_buf = NULL;

    /* Create HSM dispatcher thread */
    ret = osa_thread_create(&disp->task_ref, &thread_param,
                            osa_hsm_active_run, hsm);
    if (ret != 0) {
        core_log("thread create failed: %d\n", ret);
        return -1;
    }

    return 0;
}

int osa_hsm_active_event_post(struct osa_hsm_active *hsm, struct hsm_event *event, uint32_t timeout)
{
    int ret = -1;
    struct hsm_dispatcher *disp = NULL;

    if (hsm == NULL || event == NULL) {
        return -1;
    }

    disp = &hsm->disp;
    if (disp->msg_q != NULL) {
        ret = osa_queue_send(disp->msg_q, event, sizeof(struct hsm_event), (int)timeout);
    } else {
        core_log("msg queue not open or exit!");
    }

    return ret;
}

/**
 * @brief Timer callback handler
 *
 * Posts a periodic event to the HSM when timer expires.
 */
static void oas_hsm_timer_handler(void *arg)
{
    struct hsm_event event = {0};
    struct osa_hsm_active *hsm = (struct osa_hsm_active *)arg;

    if (hsm != NULL) {
        event.signal = HSM_SIG_PERIOD;
        osa_hsm_active_event_post(hsm, &event, 0);
    }
}

int osa_hsm_active_period(struct osa_hsm_active *hsm, uint32_t period)
{
    int ret = -1;

    if (hsm == NULL) {
        return -1;
    }

    if (period != 0) {
        ret = osa_timer_create(&hsm->timer.timer_id, period, true,
                               oas_hsm_timer_handler, hsm);
        hsm->timer.tick_period = period;
    } else {
        ret = osa_timer_stop(hsm->timer.timer_id);
        hsm->timer.tick_period = 0;
    }

    return ret;
}
