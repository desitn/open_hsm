#include "hsm_simulate.h"

#define SIMULATE_TASK_STACK_SIZE    (1024*20)
#define SIMULATE_TASK_PRIO          (10)
#define SIMULATE_TASK_MSG_Q_SIZE    (20)

struct hsm_simulate
{
    struct osa_hsm_active super;
 
};

struct hsm_simulate g_simulate = {0};

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
    if(str == NULL){
        str = "basic";
    }
    return str;
}

static int hsm_simulate_init(struct hsm_simulate* simulate, struct hsm_event *event)
{

    int ret = 0;
    struct hsm_active* entry = NULL;

    simulate->super.msg_q    = -1;
    simulate->super.task_ref = 0;
    /** @note transsit to working state */
    entry = &(simulate->super.super);
    printf("(%s) e:%d [%s] \n", entry->current->name, event->signal, hsm_sig_str(event->signal));
    STATE_TRANSIT(&simulate_working_state);
    ret = entry->error;

    return ret;

}

int hsm_simulate_event(int signal, char* data)
{
    int ret = 0;
    struct hsm_event event = {0};
    event.signal = signal;
    event.data   = data;
    ret = osa_hsm_active_event_post(&g_simulate.super, &event, 5);

    return ret;
}

static int hsm_simulate_working(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;
    struct hsm_active* entry = NULL;

    entry = &(simulate->super.super);
    printf("(%s) e:%d [%s] \n", entry->current->name, event->signal, hsm_sig_str(event->signal));
    switch (event->signal)
    {
    case HSM_SIG_ENTRY:
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

int hsm_simulate_create(void)
{
    int ret = 0;
    struct hsm_simulate* simulate = &g_simulate;
    /** @note wrapper hierarchical state machine create */
    simulate->super.name = "simulate";
    osa_hsm_active_init(&simulate->super, &simulate_init_state);
    osa_hsm_active_start(&simulate->super, SIMULATE_TASK_STACK_SIZE, SIMULATE_TASK_PRIO, SIMULATE_TASK_MSG_Q_SIZE);

    return ret;
}


int main(int argc, char* argv[])
{
    int ret = 0;
    hsm_simulate_create();
    while(1){
        sleep(1);
    }
    return ret;
}
