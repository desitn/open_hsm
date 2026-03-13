/**
 * @file osa_worker.c
 * @brief Platform-independent worker pool implementation
 *
 * This file contains the platform-independent worker pool implementation.
 * All platform-specific operations are abstracted through osa_platform.h
 */

#include "osa_worker.h"
#include "osa_hsm.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define UNUSED(x)                    (void)(x)
#define osa_log(msg, ...)	         printf(msg, ##__VA_ARGS__)
#define wp_log(str, msg,...)         osa_log("[wp|%s] "msg"",str,##__VA_ARGS__)
#define core_log(msg,...)            wp_log("core", msg, ##__VA_ARGS__)

/* Global job ID counter */
static uint32_t g_job_id_counter = 0;

/* Static job pool for lightweight operation */
#define OSA_JOB_POOL_SIZE            32
static struct osa_worker_job g_job_pool[OSA_JOB_POOL_SIZE];
static int g_job_pool_initialized = 0;

/**
 * @brief Get next job ID
 */
static uint32_t get_next_job_id(void)
{
    return ++g_job_id_counter;
}

/**
 * @brief Initialize the static job pool
 */
static void init_job_pool(void)
{
    int i;
    if (g_job_pool_initialized) {
        return;
    }

    for (i = 0; i < OSA_JOB_POOL_SIZE; i++) {
        memset(&g_job_pool[i], 0, sizeof(struct osa_worker_job));
        g_job_pool[i].id = 0;
        g_job_pool[i].state = OSA_JOB_COMPLETED;
    }

    g_job_pool_initialized = 1;
}

/**
 * @brief Allocate a job item from the pool
 */
static struct osa_worker_job* job_pool_alloc(void)
{
    int i;

    if (!g_job_pool_initialized) {
        init_job_pool();
    }

    for (i = 0; i < OSA_JOB_POOL_SIZE; i++) {
        if (g_job_pool[i].state == OSA_JOB_COMPLETED ||
            g_job_pool[i].state == OSA_JOB_CANCELLED) {
            memset(&g_job_pool[i], 0, sizeof(struct osa_worker_job));
            g_job_pool[i].id = get_next_job_id();
            g_job_pool[i].state = OSA_JOB_PENDING;
            return &g_job_pool[i];
        }
    }

    return NULL;  /* Pool exhausted */
}

/**
 * @brief Free a job item back to the pool
 */
static void job_pool_free(struct osa_worker_job *job)
{
    if (job >= g_job_pool && job < g_job_pool + OSA_JOB_POOL_SIZE) {
        job->state = OSA_JOB_COMPLETED;
    }
}

/**
 * @brief Worker thread function
 */
static void osa_worker_pool_thread(void *param)
{
    struct osa_worker_pool *pool = (struct osa_worker_pool *)param;
    struct osa_worker_job *job = NULL;
    int ret;

    core_log("Worker thread started\n");

    while (1) {
        /* Wait for job using semaphore */
        ret = osa_sem_wait(pool->work_sem, -1);
        if (ret != 0) {
            continue;
        }

        osa_mutex_lock(pool->lock);

        /* Check for shutdown */
        if (pool->shutdown) {
            osa_mutex_unlock(pool->lock);
            break;
        }

        /* Get job from queue */
        job = pool->job_queue_head;
        if (job) {
            pool->job_queue_head = job->next;
            if (pool->job_queue_head == NULL) {
                pool->job_queue_tail = NULL;
            }
            job->next = NULL;
            job->prev = NULL;
            job->state = OSA_JOB_RUNNING;
            pool->stats.active_workers++;
            pool->stats.queue_depth--;
        }

        osa_mutex_unlock(pool->lock);

        if (job) {
            /* Execute the job */
            core_log("Executing job id:%u\n", job->id);

            if (job->job_func) {
                ret = job->job_func(job->job_ctx);
                job->result = ret;
            } else {
                job->result = -1;
            }

            osa_mutex_lock(pool->lock);
            job->state = (job->result < 0) ? OSA_JOB_ERROR : OSA_JOB_COMPLETED;
            pool->stats.active_workers--;

            if (job->state == OSA_JOB_ERROR) {
                pool->stats.total_errors++;
            } else {
                pool->stats.total_completed++;
            }

            /* Handle completion callback */
            if (job->complete_cb) {
                job->complete_cb(job, job->result, job->complete_ctx);
            }

            /* Handle HSM notification */
            if (job->target_hsm) {
                struct hsm_event event = {0};
                event.signal = job->completion_signal;
                event.data = job;
                event.size = sizeof(struct osa_worker_job);

                ret = osa_hsm_active_event_post(job->target_hsm, &event, 0);
                if (ret != 0) {
                    core_log("Failed to post completion event to HSM\n");
                }
            }

            /* Signal completion */
            osa_sem_post(pool->complete_sem);
            osa_mutex_unlock(pool->lock);
        }
    }

    core_log("Worker thread exiting\n");
}

/**
 * @brief Completion notification thread for HSM integration
 */
static void osa_worker_completion_thread(void *param)
{
    struct osa_worker_pool *pool = (struct osa_worker_pool *)param;

    UNUSED(pool);

    /* This thread can be used for additional completion processing if needed */
    core_log("Completion thread started\n");

    while (!pool->shutdown) {
        /* Sleep and check for shutdown */
        osa_thread_sleep(100);  /* 100ms */
    }

    core_log("Completion thread exiting\n");
}

int osa_worker_pool_init(struct osa_worker_pool *pool, const char *name,
                         uint32_t num_workers, uint32_t queue_size)
{
    int ret;

    if (pool == NULL) {
        return -1;
    }

    memset(pool, 0, sizeof(struct osa_worker_pool));

    /* Set defaults */
    pool->num_workers = (num_workers == 0) ? OSA_WORKER_POOL_DEFAULT_WORKERS : num_workers;
    pool->queue_size = (queue_size == 0) ? OSA_WORKER_POOL_QUEUE_SIZE : queue_size;

    if (pool->num_workers > OSA_WORKER_POOL_MAX_WORKERS) {
        pool->num_workers = OSA_WORKER_POOL_MAX_WORKERS;
    }

    /* Copy name */
    if (name) {
        pool->name = strdup(name);
    } else {
        pool->name = strdup("wp");
    }

    /* Initialize synchronization primitives */
    ret = osa_mutex_create(&pool->lock);
    if (ret != 0) {
        core_log("Failed to create mutex: %d\n", ret);
        return -1;
    }

    ret = osa_sem_create(&pool->work_sem, 0, pool->queue_size);
    if (ret != 0) {
        core_log("Failed to create work semaphore: %d\n", ret);
        osa_mutex_destroy(pool->lock);
        return -1;
    }

    ret = osa_sem_create(&pool->complete_sem, 0, pool->queue_size);
    if (ret != 0) {
        core_log("Failed to create complete semaphore: %d\n", ret);
        osa_sem_destroy(pool->work_sem);
        osa_mutex_destroy(pool->lock);
        return -1;
    }

    /* Allocate worker thread array */
    pool->workers = (osa_thread_t *)malloc(sizeof(osa_thread_t) * pool->num_workers);
    if (pool->workers == NULL) {
        core_log("Failed to allocate worker array\n");
        osa_sem_destroy(pool->complete_sem);
        osa_sem_destroy(pool->work_sem);
        osa_mutex_destroy(pool->lock);
        return -1;
    }

    pool->shutdown = false;

    core_log("Worker pool '%s' initialized with %u workers, queue size %u\n",
             pool->name, pool->num_workers, pool->queue_size);

    return 0;
}

int osa_worker_pool_start(struct osa_worker_pool *pool)
{
    int ret;
    uint32_t i;
    struct osa_thread_param thread_param = {0};

    if (pool == NULL || pool->workers == NULL) {
        return -1;
    }

    /* Setup thread parameters */
    thread_param.name = pool->name;
    thread_param.stack_size = OSA_WORKER_POOL_STACK_SIZE;
    thread_param.priority = OSA_WORKER_POOL_PRIORITY;
    thread_param.stack_buf = NULL;

    /* Create worker threads */
    for (i = 0; i < pool->num_workers; i++) {
        ret = osa_thread_create(&pool->workers[i], &thread_param,
                                osa_worker_pool_thread, pool);
        if (ret != 0) {
            core_log("Failed to create worker thread %u: %d\n", i, ret);
            return -1;
        }
    }

    /* Create completion thread */
    ret = osa_thread_create(&pool->completion_thread, &thread_param,
                            osa_worker_completion_thread, pool);
    if (ret != 0) {
        core_log("Failed to create completion thread: %d\n", ret);
        /* Non-fatal, continue without completion thread */
    }

    core_log("Worker pool '%s' started with %u workers\n", pool->name, pool->num_workers);

    return 0;
}

int osa_worker_pool_stop(struct osa_worker_pool *pool)
{
    uint32_t i;

    if (pool == NULL) {
        return -1;
    }

    core_log("Stopping worker pool '%s'...\n", pool->name);

    /* Signal shutdown */
    osa_mutex_lock(pool->lock);
    pool->shutdown = true;
    osa_mutex_unlock(pool->lock);

    /* Post to semaphore to wake up all workers */
    for (i = 0; i < pool->num_workers; i++) {
        osa_sem_post(pool->work_sem);
    }

    /* Wait for worker threads to finish */
    for (i = 0; i < pool->num_workers; i++) {
        osa_thread_join(pool->workers[i]);
    }

    /* Wait for completion thread */
    osa_thread_join(pool->completion_thread);

    /* Cleanup */
    free(pool->workers);
    pool->workers = NULL;

    osa_sem_destroy(pool->complete_sem);
    osa_sem_destroy(pool->work_sem);
    osa_mutex_destroy(pool->lock);

    if (pool->name) {
        free(pool->name);
        pool->name = NULL;
    }

    core_log("Worker pool stopped\n");

    return 0;
}

int osa_worker_pool_submit(struct osa_worker_pool *pool, struct osa_worker_job *job)
{
    int ret = 0;

    if (pool == NULL || job == NULL) {
        return -1;
    }

    osa_mutex_lock(pool->lock);

    /* Check if queue is full */
    if (pool->stats.queue_depth >= pool->queue_size) {
        core_log("Job queue full, cannot submit job\n");
        osa_mutex_unlock(pool->lock);
        return -1;
    }

    /* Assign job ID if not set */
    if (job->id == 0) {
        job->id = get_next_job_id();
    }

    job->state = OSA_JOB_PENDING;
    job->next = NULL;
    job->prev = NULL;

    /* Add to queue */
    if (pool->job_queue_tail == NULL) {
        pool->job_queue_head = job;
        pool->job_queue_tail = job;
    } else {
        pool->job_queue_tail->next = job;
        job->prev = pool->job_queue_tail;
        pool->job_queue_tail = job;
    }

    pool->stats.queue_depth++;
    pool->stats.total_submitted++;

    core_log("Job id:%u submitted, queue depth:%u\n", job->id, pool->stats.queue_depth);

    /* Signal worker threads */
    osa_sem_post(pool->work_sem);
    osa_mutex_unlock(pool->lock);

    return ret;
}

int osa_worker_pool_submit_to_hsm(struct osa_worker_pool *pool, struct osa_worker_job *job,
                                   struct osa_hsm_active *target_hsm,
                                   int completion_signal)
{
    if (job == NULL) {
        return -1;
    }

    job->target_hsm = target_hsm;
    job->completion_signal = completion_signal;

    return osa_worker_pool_submit(pool, job);
}

int osa_worker_pool_cancel(struct osa_worker_pool *pool, uint32_t job_id)
{
    struct osa_worker_job *job;
    int found = 0;

    if (pool == NULL) {
        return -1;
    }

    osa_mutex_lock(pool->lock);

    /* Search in queue */
    job = pool->job_queue_head;
    while (job) {
        if (job->id == job_id) {
            /* Remove from queue */
            if (job->prev) {
                job->prev->next = job->next;
            } else {
                pool->job_queue_head = job->next;
            }

            if (job->next) {
                job->next->prev = job->prev;
            } else {
                pool->job_queue_tail = job->prev;
            }

            job->state = OSA_JOB_CANCELLED;
            pool->stats.queue_depth--;
            pool->stats.total_cancelled++;
            found = 1;
            break;
        }
        job = job->next;
    }

    osa_mutex_unlock(pool->lock);

    if (found) {
        core_log("Job id:%u cancelled\n", job_id);
        return 0;
    }

    /* Job not found in queue (might be running or already completed) */
    return -1;
}

int osa_worker_pool_get_stats(struct osa_worker_pool *pool, struct osa_worker_pool_stats *stats)
{
    if (pool == NULL || stats == NULL) {
        return -1;
    }

    osa_mutex_lock(pool->lock);
    memcpy(stats, &pool->stats, sizeof(struct osa_worker_pool_stats));
    osa_mutex_unlock(pool->lock);

    return 0;
}

void osa_worker_job_init(struct osa_worker_job *job,
                         int (*job_func)(void *ctx),
                         void *job_ctx,
                         void (*complete_cb)(struct osa_worker_job *job, int result, void *ctx),
                         void *complete_ctx)
{
    if (job == NULL) {
        return;
    }

    memset(job, 0, sizeof(struct osa_worker_job));

    job->id = get_next_job_id();
    job->state = OSA_JOB_PENDING;
    job->job_func = job_func;
    job->job_ctx = job_ctx;
    job->complete_cb = complete_cb;
    job->complete_ctx = complete_ctx;
    job->target_hsm = NULL;
    job->completion_signal = 0;
    job->result = 0;
    job->next = NULL;
    job->prev = NULL;
}

int osa_worker_pool_wait(struct osa_worker_pool *pool, struct osa_worker_job *job, int timeout_ms)
{
    int ret = 0;

    if (pool == NULL || job == NULL) {
        return -1;
    }

    osa_mutex_lock(pool->lock);

    if (timeout_ms == 0) {
        /* No wait - just check status */
        if (job->state != OSA_JOB_COMPLETED && job->state != OSA_JOB_CANCELLED) {
            ret = -1;  /* Still pending or running */
        }
    } else if (timeout_ms < 0) {
        /* Infinite wait - use completion semaphore */
        osa_mutex_unlock(pool->lock);
        ret = osa_sem_wait(pool->complete_sem, -1);
        osa_mutex_lock(pool->lock);
        if (ret != 0) {
            ret = -1;
        }
    } else {
        /* Timed wait */
        osa_mutex_unlock(pool->lock);
        ret = osa_sem_wait(pool->complete_sem, timeout_ms);
        osa_mutex_lock(pool->lock);
        if (ret == -ETIMEDOUT) {
            ret = -1;
        }
    }

    osa_mutex_unlock(pool->lock);

    return ret;
}

/**
 * @brief Allocate a job item from the static pool
 * @return Pointer to job item, or NULL if pool exhausted
 */
struct osa_worker_job* osa_worker_job_alloc(void)
{
    return job_pool_alloc();
}

/**
 * @brief Free a job item back to the static pool
 * @param job Job item to free
 */
void osa_worker_job_free(struct osa_worker_job *job)
{
    job_pool_free(job);
}
