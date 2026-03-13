#include "hsm_simulate.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
void hsm_timestamp(char *buf, size_t len)
{
    struct timeval tv;
    struct tm tm_info;
    if (buf == NULL || len < 24) return; 
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_info);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm_info);
    snprintf(&buf[19], len - 19, ".%03ld", tv.tv_usec / 1000);
}

#define osa_log(msg, ...)            do {                                       \
                                        char ts[32];                            \
                                        hsm_timestamp(ts, sizeof(ts));          \
                                        printf("[%s] " msg, ts, ##__VA_ARGS__); \
                                     } while(0)

#define hsm_log(str, msg,...)        osa_log("[hsm|%s] "msg"",str,##__VA_ARGS__)
#define app_log(msg,...)             hsm_log("app", msg, ##__VA_ARGS__)

#define SIMULATE_TASK_STACK_SIZE     (1024*10)
#define SIMULATE_TASK_PRIO           (10)
#define SIMULATE_TASK_MSG_Q_SIZE     (20)

struct hsm_simulate
{
    struct osa_hsm_active super;

    struct osa_hsm_active *watcher; 
    int cnt;
    int send_cnt;
};

struct hsm_simulate g_simulate1  = {0};
struct hsm_simulate g_simulate2  = {0};

static int simulate_init_state_handler(struct hsm_simulate* simulate, struct hsm_event *event);
static int simulate_working_state_handler(struct hsm_simulate* simulate, struct hsm_event *event);
static int simulate_state_idle_handler(struct hsm_simulate* simulate, struct hsm_event *event);

/** @note wrapper hsm state entry */
static struct hsm_state simulate_init_state =
{
    .name     = "init",
    .handler  = (hsm_state_handler)&simulate_init_state_handler,
    .parent   = NULL,
};

static struct hsm_state simulate_working_state =
{
    .name     = "working",
    .handler  = (hsm_state_handler)&simulate_working_state_handler,
    .parent   = NULL,
};

static struct hsm_state simulate_idle_state =
{
    .name     = "idle",
    .handler  = (hsm_state_handler)&simulate_state_idle_handler,
    .parent   = NULL,
};

/** @note state event signal string array for debug */
static const char* g_sig_str[SIMULATE_SIG_MAX] =
{
    [SIMULATE_SIG_RECV] = "recv",
    [SIMULATE_SIG_SEND] = "send",
    [SIMULATE_SIG_IDLE] = "idle",
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

static int simulate_init_state_handler(struct hsm_simulate* simulate, struct hsm_event *event)
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


static int do_period(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;
    struct osa_hsm_active *hsm     = NULL;
    struct osa_hsm_active *watcher = NULL;
    struct hsm_event post_event = {0};

    hsm     = &simulate->super;
    watcher = simulate->watcher;

    post_event.signal = SIMULATE_SIG_SEND;
    post_event.data   = NULL;
    ret = osa_hsm_active_event_post(watcher, &post_event, 5);
    app_log("%s send signal 'send' to watcher\r\n", hsm->name);
    if  (simulate->cnt++ > 10) {
        ret = osa_hsm_active_period(hsm, 0);
        app_log("%s period stop\r\n", hsm->name);
    }

    return ret;
}

static int do_send(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;
    struct osa_hsm_active *hsm = NULL;
    struct osa_hsm_active *watcher = NULL;
    struct hsm_event post_event = {0};

    hsm     = &simulate->super;
    watcher = simulate->watcher;
    post_event.signal = SIMULATE_SIG_RECV;
    post_event.data   = NULL;
    ret = osa_hsm_active_event_post(watcher, &post_event, 5);
    app_log("%s send signal 'recv' to watcher\r\n", hsm->name);
    simulate->send_cnt++;
    if  (simulate->send_cnt%5 == 0) {
        post_event.signal = SIMULATE_SIG_IDLE;
        post_event.data   = NULL;
        // self transsit to idle
        ret = osa_hsm_active_event_post(hsm, &post_event, 5);
        app_log("%s send signal 'idle' to self\r\n", hsm->name);
    }

    return ret;
}

static int simulate_working_state_handler(struct hsm_simulate* simulate, struct hsm_event *event)
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
    case SIMULATE_SIG_SEND:
        do_send(simulate, event);
        break;
    case SIMULATE_SIG_RECV:
  
        break;
    case SIMULATE_SIG_IDLE:
        STATE_TRANSIT(&simulate_idle_state);
        ret = STATE_ERROR();
        break;
    default:
        break;
    }

    return ret;
}

static int simulate_state_idle_handler(struct hsm_simulate* simulate, struct hsm_event *event)
{
    int ret = 0;

    STATE_ENTRY(&(simulate->super.super));
    app_log("%s (%s) e:%d [%s] \n", simulate->super.name, STATE_NAME(), event->signal, hsm_sig_str(event->signal));
    switch (event->signal)
    {
    case HSM_SIG_ENTRY:
        break;
    case HSM_SIG_PERIOD:
        break;
    default:
        break;
    }

    return ret;
}

int simulate_event_post(struct hsm_simulate* simulate, int signal, char* data)
{
    int ret = 0;
    struct hsm_event event = {0};
    event.signal = signal;
    event.data   = data;
    ret = osa_hsm_active_event_post(&simulate->super, &event, 5);

    return ret;
}

int simulate_create(struct hsm_simulate* simulate)
{
    int ret = 0;
    struct osa_hsm_active *hsm = NULL;

    hsm = &simulate->super;

    osa_hsm_active_init(hsm, &simulate_init_state);
    osa_hsm_active_start(hsm, SIMULATE_TASK_STACK_SIZE, SIMULATE_TASK_PRIO, SIMULATE_TASK_MSG_Q_SIZE);

    return ret;
}


int main(int argc, char* argv[])
{
    int ret = 0;

    g_simulate1.super.name  = "thread1";
    g_simulate1.watcher = &g_simulate2.super;
    simulate_create(&g_simulate1);
    osa_hsm_active_period(&g_simulate1.super, 200);

    g_simulate2.super.name = "thread2";
    g_simulate2.watcher = &g_simulate1.super;
    simulate_create(&g_simulate2);

    while (1) {
       sleep(10);
    }
    
    return ret;
}
