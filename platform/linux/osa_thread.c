/**
 * @file osa_thread.c
 * @brief Linux pthread implementation of thread abstraction
 */

#include "osa_thread.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

/* Internal thread structure */
struct osa_thread_internal {
    pthread_t pthread;
    char name[32];
    int detached;
};

int osa_thread_create(osa_thread_t *thread,
                      const struct osa_thread_param *param,
                      osa_thread_entry_t entry,
                      void *arg)
{
    struct osa_thread_internal *t;
    pthread_attr_t attr;
    int ret;

    if (!thread || !param || !entry) {
        return -EINVAL;
    }

    t = calloc(1, sizeof(*t));
    if (!t) {
        return -ENOMEM;
    }

    pthread_attr_init(&attr);

    /* Set stack size */
    if (param->stack_size > 0) {
        pthread_attr_setstacksize(&attr, param->stack_size);
    }

    /* Set stack buffer if provided */
    if (param->stack_buf != NULL && param->stack_size > 0) {
        pthread_attr_setstack(&attr, param->stack_buf, param->stack_size);
    }

    /* Set real-time scheduling if priority is specified */
    if (param->priority > 0) {
        struct sched_param sched_param = {0};
        sched_param.sched_priority = param->priority;
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        pthread_attr_setschedparam(&attr, &sched_param);
    }

    /* Store name */
    if (param->name) {
        strncpy(t->name, param->name, sizeof(t->name) - 1);
    }

    /* Create thread */
    ret = pthread_create(&t->pthread, &attr, (void* (*)(void*))entry, arg);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        free(t);
        return -ret;
    }

    /* Don't detach - we need to join later for proper cleanup */
    t->detached = 0;

    *thread = t;
    return 0;
}

int osa_thread_delete(osa_thread_t thread)
{
    struct osa_thread_internal *t = (struct osa_thread_internal *)thread;

    if (!t) {
        return -EINVAL;
    }

    /* For detached threads, we can't cancel them cleanly
     * In a real implementation, you might want to use a cancellation flag */
    pthread_cancel(t->pthread);
    free(t);

    return 0;
}

void osa_thread_exit(void)
{
    pthread_exit(NULL);
}

osa_thread_t osa_thread_self(void)
{
    /* Return pthread_t cast to handle
     * Note: This is a simplified implementation */
    static __thread struct osa_thread_internal self_thread;
    self_thread.pthread = pthread_self();
    return &self_thread;
}

void osa_thread_sleep(uint32_t ms)
{
    usleep(ms * 1000);
}

void osa_thread_yield(void)
{
    sched_yield();
}

int osa_thread_join(osa_thread_t thread)
{
    struct osa_thread_internal *t = (struct osa_thread_internal *)thread;
    int ret;

    if (!t) {
        return -EINVAL;
    }

    /* If thread was detached, we can't join it
     * In this case, we just wait for it to complete */
    if (t->detached) {
        /* For detached threads, pthread_join will fail
         * We return success as the thread will clean up itself */
        return 0;
    }

    ret = pthread_join(t->pthread, NULL);
    if (ret != 0) {
        return -ret;
    }

    free(t);
    return 0;
}
