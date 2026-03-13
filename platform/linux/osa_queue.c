/**
 * @file osa_queue.c
 * @brief Linux implementation of OS Abstraction Layer - Message Queue
 *
 * This implementation provides a thread-safe message queue using pthread
 * condition variables. It does not require root permissions, unlike POSIX
 * message queues which need write access to /dev/mqueue.
 */

#include "osa_queue.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

/* Message queue structure */
struct osa_queue_impl {
    char name[OSA_QUEUE_NAME_LEN];   /* Queue name for debugging */
    size_t msg_size;                /* Size of each message */
    int max_msgs;                   /* Maximum number of messages */

    pthread_mutex_t mutex;          /* Mutex for thread safety */
    pthread_cond_t cond_send;       /* Condition for senders (queue full) */
    pthread_cond_t cond_recv;       /* Condition for receivers (queue empty) */

    uint8_t *buffer;                /* Ring buffer for messages */
    int head;                       /* Read position */
    int tail;                       /* Write position */
    int count;                      /* Current message count */
    bool closed;                    /* Queue closed flag */
};

/**
 * @brief Calculate timeout timespec from milliseconds
 */
static void calc_timeout(struct timespec *ts, int timeout_ms)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += timeout_ms / 1000;
    ts->tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

int osa_queue_create(osa_queue_t *queue, const char *name,
                     size_t msg_size, int max_msgs)
{
    struct osa_queue_impl *q;

    if (!queue || !name || msg_size == 0 || max_msgs <= 0) {
        return -EINVAL;
    }

    /* Allocate queue structure */
    q = (struct osa_queue_impl *)calloc(1, sizeof(struct osa_queue_impl));
    if (!q) {
        return -ENOMEM;
    }

    /* Initialize fields */
    strncpy(q->name, name, OSA_QUEUE_NAME_LEN - 1);
    q->name[OSA_QUEUE_NAME_LEN - 1] = '\0';
    q->msg_size = msg_size;
    q->max_msgs = max_msgs;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->closed = false;

    /* Allocate ring buffer */
    q->buffer = (uint8_t *)calloc(max_msgs, msg_size);
    if (!q->buffer) {
        free(q);
        return -ENOMEM;
    }

    /* Initialize mutex and condition variables */
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_send, NULL);
    pthread_cond_init(&q->cond_recv, NULL);

    *queue = (osa_queue_t)q;
    return 0;
}

int osa_queue_destroy(osa_queue_t queue)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;

    if (!q) {
        return -EINVAL;
    }

    pthread_mutex_lock(&q->mutex);
    q->closed = true;
    /* Wake up all waiting threads */
    pthread_cond_broadcast(&q->cond_send);
    pthread_cond_broadcast(&q->cond_recv);
    pthread_mutex_unlock(&q->mutex);

    /* Cleanup resources */
    pthread_cond_destroy(&q->cond_send);
    pthread_cond_destroy(&q->cond_recv);
    pthread_mutex_destroy(&q->mutex);

    free(q->buffer);
    free(q);

    return 0;
}

int osa_queue_send(osa_queue_t queue, const void *msg,
                   size_t msg_len, int timeout_ms)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;
    struct timespec ts;
    int ret = 0;

    if (!q || !msg) {
        return -EINVAL;
    }

    if (msg_len != q->msg_size) {
        return -EINVAL;
    }

    pthread_mutex_lock(&q->mutex);

    /* Check if queue is closed */
    if (q->closed) {
        pthread_mutex_unlock(&q->mutex);
        return -EPIPE;
    }

    /* Wait if queue is full */
    if (q->count >= q->max_msgs) {
        if (timeout_ms == 0) {
            /* Non-blocking */
            pthread_mutex_unlock(&q->mutex);
            return -EAGAIN;
        }

        if (timeout_ms > 0) {
            /* Timed wait */
            calc_timeout(&ts, timeout_ms);
            while (q->count >= q->max_msgs && !q->closed) {
                ret = pthread_cond_timedwait(&q->cond_send, &q->mutex, &ts);
                if (ret == ETIMEDOUT) {
                    pthread_mutex_unlock(&q->mutex);
                    return -ETIMEDOUT;
                }
            }
        } else {
            /* Infinite wait */
            while (q->count >= q->max_msgs && !q->closed) {
                pthread_cond_wait(&q->cond_send, &q->mutex);
            }
        }
    }

    /* Check again if queue is closed */
    if (q->closed) {
        pthread_mutex_unlock(&q->mutex);
        return -EPIPE;
    }

    /* Check if still full (could happen with timeout) */
    if (q->count >= q->max_msgs) {
        pthread_mutex_unlock(&q->mutex);
        return -EAGAIN;
    }

    /* Write message to buffer */
    memcpy(q->buffer + (q->tail * q->msg_size), msg, q->msg_size);
    q->tail = (q->tail + 1) % q->max_msgs;
    q->count++;

    /* Signal receivers */
    pthread_cond_signal(&q->cond_recv);

    pthread_mutex_unlock(&q->mutex);

    return 0;
}

int osa_queue_receive(osa_queue_t queue, void *msg,
                      size_t msg_len, int timeout_ms)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;
    struct timespec ts;
    int ret = 0;

    if (!q || !msg) {
        return -EINVAL;
    }

    if (msg_len != q->msg_size) {
        return -EINVAL;
    }

    pthread_mutex_lock(&q->mutex);

    /* Check if queue is closed and empty */
    if (q->closed && q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -EPIPE;
    }

    /* Wait if queue is empty */
    if (q->count == 0) {
        if (timeout_ms == 0) {
            /* Non-blocking */
            pthread_mutex_unlock(&q->mutex);
            return -EAGAIN;
        }

        if (timeout_ms > 0) {
            /* Timed wait */
            calc_timeout(&ts, timeout_ms);
            while (q->count == 0 && !q->closed) {
                ret = pthread_cond_timedwait(&q->cond_recv, &q->mutex, &ts);
                if (ret == ETIMEDOUT) {
                    pthread_mutex_unlock(&q->mutex);
                    return -ETIMEDOUT;
                }
            }
        } else {
            /* Infinite wait */
            while (q->count == 0 && !q->closed) {
                pthread_cond_wait(&q->cond_recv, &q->mutex);
            }
        }
    }

    /* Check if queue is closed and empty after waiting */
    if (q->closed && q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -EPIPE;
    }

    /* Check if still empty (could happen with timeout) */
    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -EAGAIN;
    }

    /* Read message from buffer */
    memcpy(msg, q->buffer + (q->head * q->msg_size), q->msg_size);
    q->head = (q->head + 1) % q->max_msgs;
    q->count--;

    /* Signal senders */
    pthread_cond_signal(&q->cond_send);

    pthread_mutex_unlock(&q->mutex);

    return 0;
}

int osa_queue_count(osa_queue_t queue)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;
    int count;

    if (!q) {
        return -EINVAL;
    }

    pthread_mutex_lock(&q->mutex);
    count = q->count;
    pthread_mutex_unlock(&q->mutex);

    return count;
}

bool osa_queue_is_full(osa_queue_t queue)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;
    bool full;

    if (!q) {
        return false;
    }

    pthread_mutex_lock(&q->mutex);
    full = (q->count >= q->max_msgs);
    pthread_mutex_unlock(&q->mutex);

    return full;
}

bool osa_queue_is_empty(osa_queue_t queue)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;
    bool empty;

    if (!q) {
        return true;
    }

    pthread_mutex_lock(&q->mutex);
    empty = (q->count == 0);
    pthread_mutex_unlock(&q->mutex);

    return empty;
}

const char* osa_queue_get_name(osa_queue_t queue)
{
    struct osa_queue_impl *q = (struct osa_queue_impl *)queue;

    if (!q) {
        return NULL;
    }
    return q->name;
}
