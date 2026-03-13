/**
 * @file osa_sync.c
 * @brief Linux pthread implementation of synchronization primitives
 */

#include "osa_sync.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* Internal mutex structure */
struct osa_mutex_internal {
    pthread_mutex_t pthread_mutex;
};

/* Internal semaphore structure */
struct osa_sem_internal {
    sem_t posix_sem;
    int max_count;
};

int osa_mutex_create(osa_mutex_t *mutex)
{
    struct osa_mutex_internal *m;
    int ret;

    if (!mutex) {
        return -EINVAL;
    }

    m = calloc(1, sizeof(*m));
    if (!m) {
        return -ENOMEM;
    }

    ret = pthread_mutex_init(&m->pthread_mutex, NULL);
    if (ret != 0) {
        free(m);
        return -ret;
    }

    *mutex = m;
    return 0;
}

void osa_mutex_destroy(osa_mutex_t mutex)
{
    struct osa_mutex_internal *m = (struct osa_mutex_internal *)mutex;

    if (m) {
        pthread_mutex_destroy(&m->pthread_mutex);
        free(m);
    }
}

void osa_mutex_lock(osa_mutex_t mutex)
{
    struct osa_mutex_internal *m = (struct osa_mutex_internal *)mutex;

    if (m) {
        pthread_mutex_lock(&m->pthread_mutex);
    }
}

void osa_mutex_unlock(osa_mutex_t mutex)
{
    struct osa_mutex_internal *m = (struct osa_mutex_internal *)mutex;

    if (m) {
        pthread_mutex_unlock(&m->pthread_mutex);
    }
}

int osa_mutex_trylock(osa_mutex_t mutex)
{
    struct osa_mutex_internal *m = (struct osa_mutex_internal *)mutex;
    int ret;

    if (!m) {
        return -EINVAL;
    }

    ret = pthread_mutex_trylock(&m->pthread_mutex);
    if (ret == 0) {
        return 0;
    }
    return -1;
}

int osa_sem_create(osa_sem_t *sem, int initial_count, int max_count)
{
    struct osa_sem_internal *s;
    int ret;

    if (!sem) {
        return -EINVAL;
    }

    s = calloc(1, sizeof(*s));
    if (!s) {
        return -ENOMEM;
    }

    /* POSIX semaphore: 0 = thread-shared, initial_count */
    ret = sem_init(&s->posix_sem, 0, initial_count);
    if (ret != 0) {
        free(s);
        return -errno;
    }

    s->max_count = max_count;
    *sem = s;
    return 0;
}

void osa_sem_destroy(osa_sem_t sem)
{
    struct osa_sem_internal *s = (struct osa_sem_internal *)sem;

    if (s) {
        sem_destroy(&s->posix_sem);
        free(s);
    }
}

int osa_sem_wait(osa_sem_t sem, int timeout_ms)
{
    struct osa_sem_internal *s = (struct osa_sem_internal *)sem;
    int ret;

    if (!s) {
        return -EINVAL;
    }

    if (timeout_ms < 0) {
        /* Wait forever */
        ret = sem_wait(&s->posix_sem);
        if (ret != 0) {
            return -errno;
        }
        return 0;
    } else if (timeout_ms == 0) {
        /* Non-blocking */
        ret = sem_trywait(&s->posix_sem);
        if (ret != 0) {
            if (errno == EAGAIN) {
                return -EAGAIN;
            }
            return -errno;
        }
        return 0;
    } else {
        /* Timed wait */
        struct timespec ts;
        struct timespec abs_timeout;

        /* Get current time */
        ret = clock_gettime(CLOCK_REALTIME, &ts);
        if (ret != 0) {
            return -errno;
        }

        /* Calculate absolute timeout */
        abs_timeout.tv_sec = ts.tv_sec + timeout_ms / 1000;
        abs_timeout.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
        if (abs_timeout.tv_nsec >= 1000000000) {
            abs_timeout.tv_sec++;
            abs_timeout.tv_nsec -= 1000000000;
        }

        ret = sem_timedwait(&s->posix_sem, &abs_timeout);
        if (ret != 0) {
            if (errno == ETIMEDOUT) {
                return -ETIMEDOUT;
            }
            return -errno;
        }
        return 0;
    }
}

void osa_sem_post(osa_sem_t sem)
{
    struct osa_sem_internal *s = (struct osa_sem_internal *)sem;

    if (s) {
        sem_post(&s->posix_sem);
    }
}

int osa_sem_get_count(osa_sem_t sem)
{
    struct osa_sem_internal *s = (struct osa_sem_internal *)sem;
    int val;
    int ret;

    if (!s) {
        return -EINVAL;
    }

    ret = sem_getvalue(&s->posix_sem, &val);
    if (ret != 0) {
        return -errno;
    }

    return val;
}
