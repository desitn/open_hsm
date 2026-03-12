#include "hsm_simulate.h"

#define osa_log(msg, ...)	         printf(msg, ##__VA_ARGS__)
#define hsm_log(str, msg,...)        osa_log("[hsm|%s] "msg"",str,##__VA_ARGS__)
#define app_log(msg,...)             hsm_log("app", msg, ##__VA_ARGS__)

#define SIMULATE_TASK_STACK_SIZE     (1024*10)
#define SIMULATE_TASK_PRIO           (10)
#define SIMULATE_TASK_MSG_Q_SIZE     (20)

struct hsm_simulate
{
    struct osa_hsm_active super;
    int cnt;
};

struct hsm_simulate g_simulate1  = {0};
struct hsm_simulate g_simulate2  = {0};

static int hsm_simulate_init(struct hsm_simulate* wrapper, struct hsm_event *event);
static int hsm_simulate_working(struct hsm_simulate* wrapper, struct hsm_event *event);

/** @note wrapper hsm state entry */
static struct hsm_state simulate_init_state =
{
    .name     = "init",
    .handler  = (hsm_state_handler)&hsm_simulate_init,
    .parent   = NULL,
};

static struct hsm_state simulate_working_state =
{
    .name     = "working",
    .handler  = (hsm_state_handler)&hsm_simulate_working,
    .parent   = NULL,
};

/** @note state event signal string array for debug */
static const char* g_sig_str[SIMULATE_SIG_MAX] =
{
    [SIMULATE_SIG_RECV] = "recv",
    [SIMULATE_SIG_SEND] = "send",
};

static const char* hsm_sig_str(int signal)
{
    const char* str = NULL;
    str =  (signal < SIMULATE_SIG_MAX) ? (g_sig_str[signal]) : ("unknown");
    if (str == NULL) {
        str = oas_hsm_sig_str(signal);
    }
    return str;
}

static int hsm_simulate_init(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;

    simulate->cnt = 0;
    /** @note transsit to working state */
    STATE_ENTRY(&(simulate->super.super));
    app_log("(%s) e:%d [%s] \n", entry->current->name, event->signal, hsm_sig_str(event->signal));
    STATE_TRANSIT(&simulate_working_state);
    ret = entry->error;

    return ret;

}

int hsm_simulate_event(struct hsm_simulate* simulate, int signal, char* data)
{
    int ret = 0;
    struct hsm_event event = {0};
    event.signal = signal;
    event.data   = data;
    ret = osa_hsm_active_event_post(&simulate->super, &event, 5);

    return ret;
}

static int do_period(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;
    struct osa_hsm_active *hsm = NULL;

    hsm = &simulate->super;
    if  (simulate->cnt++ > 10) {
        ret = osa_hsm_active_period(hsm, 0);
    }

    return ret;
}

static int hsm_simulate_working(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;

    STATE_ENTRY(&(simulate->super.super));
    app_log("%s (%s) e:%d [%s] \n", simulate->super.name, STATE_NAME(), event->signal, hsm_sig_str(event->signal));
    switch (event->signal)
    {
    case HSM_SIG_ENTRY:
        break;
    case HSM_SIG_PERIOD:
        do_period(simulate, event);
        break;
    case SIMULATE_SIG_RECV:
  
        break;
    case SIMULATE_SIG_SEND:
 
        break;
    default:
        break;
    }

    return ret;
}

int hsm_simulate_create(struct hsm_simulate* simulate)
{
    int ret = 0;
    struct osa_hsm_active *hsm = NULL;

    hsm = &simulate->super;

    osa_hsm_active_init(hsm, &simulate_init_state);
    osa_hsm_active_start(hsm, SIMULATE_TASK_STACK_SIZE, SIMULATE_TASK_PRIO, SIMULATE_TASK_MSG_Q_SIZE);
    osa_hsm_active_period(hsm, 1000);

    return ret;
}


int main(int argc, char* argv[])
{
    int ret = 0;

    g_simulate1.super.name  = "thread1";
    hsm_simulate_create(&g_simulate1);

    g_simulate2.super.name = "thread2";
    hsm_simulate_create(&g_simulate2);

    while (1) {
       sleep(10);
    }
    
    return ret;
}
