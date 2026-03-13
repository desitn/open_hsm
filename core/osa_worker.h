#ifndef __OSA_WORKER_H__
#define __OSA_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hsm.h"
#include "../platform/osa_platform.h"
#include <stdint.h>
#include <stdbool.h>

/* Worker pool configuration */
#define OSA_WORKER_POOL_DEFAULT_WORKERS   4
#define OSA_WORKER_POOL_MAX_WORKERS       16
#define OSA_WORKER_POOL_QUEUE_SIZE        64
#define OSA_WORKER_POOL_STACK_SIZE        (1024 * 8)
#define OSA_WORKER_POOL_PRIORITY          5

/* Job states */
enum osa_worker_job_state {
    OSA_JOB_PENDING,      /* Job is queued waiting for execution */
    OSA_JOB_RUNNING,      /* Job is currently being executed */
    OSA_JOB_COMPLETED,    /* Job completed successfully */
    OSA_JOB_CANCELLED,    /* Job was cancelled */
    OSA_JOB_ERROR,        /* Job encountered an error */
};

/* Job item structure */
struct osa_worker_job {
    uint32_t id;                          /* Unique job item ID */
    enum osa_worker_job_state state;      /* Current state of job */

    /* Job function and context */
    int (*job_func)(void *ctx);           /* The actual job function */
    void *job_ctx;                        /* Context passed to job function */

    /* Completion callback */
    void (*complete_cb)(struct osa_worker_job *job, int result, void *ctx);
    void *complete_ctx;                   /* Context for completion callback */

    /* HSM integration - target HSM to receive completion event */
    struct osa_hsm_active *target_hsm;    /* HSM to notify on completion */
    int completion_signal;                /* Signal to send to HSM on completion */

    /* Result */
    int result;                           /* Result from job function */

    /* Linked list for queue management */
    struct osa_worker_job *next;
    struct osa_worker_job *prev;
};

/* Worker pool statistics */
struct osa_worker_pool_stats {
    uint32_t total_submitted;     /* Total jobs submitted */
    uint32_t total_completed;     /* Total jobs completed */
    uint32_t total_cancelled;     /* Total jobs cancelled */
    uint32_t total_errors;        /* Total jobs with errors */
    uint32_t queue_depth;         /* Current queue depth */
    uint32_t active_workers;      /* Number of currently active workers */
};

/* Worker pool structure - platform independent */
struct osa_worker_pool {
    char *name;                       /* Worker pool name */

    /* Worker threads - using abstract thread type */
    osa_thread_t *workers;            /* Array of worker threads */
    osa_thread_t completion_thread;   /* Completion notification thread */

    /* Synchronization primitives - using abstract types */
    osa_mutex_t lock;                 /* Mutex for thread safety */
    osa_sem_t work_sem;               /* Semaphore for work availability */
    osa_sem_t complete_sem;           /* Semaphore for work completion */

    struct osa_worker_job *job_queue_head; /* Head of job queue */
    struct osa_worker_job *job_queue_tail; /* Tail of job queue */
    struct osa_worker_job *free_list;      /* Free list of job structures */

    uint32_t num_workers;             /* Number of worker threads */
    uint32_t queue_size;              /* Maximum queue size */
    bool shutdown;                    /* Shutdown flag */

    struct osa_worker_pool_stats stats; /* Statistics */
};

/* Completion event sent to HSM */
struct osa_worker_completion_event {
    uint32_t job_id;                  /* ID of completed job */
    int result;                       /* Result from job function */
    void *user_ctx;                   /* User context from job item */
};

/* Worker pool API */

/**
 * @brief Initialize a worker pool
 * @param pool Worker pool structure to initialize
 * @param name Name for the worker pool
 * @param num_workers Number of worker threads (0 for default)
 * @param queue_size Maximum job queue size (0 for default)
 * @return 0 on success, negative on error
 */
int osa_worker_pool_init(struct osa_worker_pool *pool, const char *name,
                         uint32_t num_workers, uint32_t queue_size);

/**
 * @brief Start the worker pool
 * @param pool Worker pool to start
 * @return 0 on success, negative on error
 */
int osa_worker_pool_start(struct osa_worker_pool *pool);

/**
 * @brief Stop the worker pool and cleanup resources
 * @param pool Worker pool to stop
 * @return 0 on success, negative on error
 */
int osa_worker_pool_stop(struct osa_worker_pool *pool);

/**
 * @brief Submit job to the worker pool
 * @param pool Worker pool to submit job to
 * @param job Job item to execute
 * @return 0 on success, negative on error
 */
int osa_worker_pool_submit(struct osa_worker_pool *pool, struct osa_worker_job *job);

/**
 * @brief Submit job with automatic HSM notification on completion
 * @param pool Worker pool to submit job to
 * @param job Job item to execute
 * @param target_hsm HSM to notify on completion
 * @param completion_signal Signal to send to HSM
 * @return 0 on success, negative on error
 */
int osa_worker_pool_submit_to_hsm(struct osa_worker_pool *pool, struct osa_worker_job *job,
                                   struct osa_hsm_active *target_hsm,
                                   int completion_signal);

/**
 * @brief Cancel a pending job item
 * @param pool Worker pool
 * @param job_id ID of job to cancel
 * @return 0 on success, negative on error
 */
int osa_worker_pool_cancel(struct osa_worker_pool *pool, uint32_t job_id);

/**
 * @brief Get worker pool statistics
 * @param pool Worker pool
 * @param stats Structure to fill with statistics
 * @return 0 on success, negative on error
 */
int osa_worker_pool_get_stats(struct osa_worker_pool *pool, struct osa_worker_pool_stats *stats);

/**
 * @brief Initialize a job item structure
 * @param job Job item to initialize
 * @param job_func Function to execute
 * @param job_ctx Context for job function
 * @param complete_cb Optional completion callback
 * @param complete_ctx Context for completion callback
 */
void osa_worker_job_init(struct osa_worker_job *job,
                         int (*job_func)(void *ctx),
                         void *job_ctx,
                         void (*complete_cb)(struct osa_worker_job *job, int result, void *ctx),
                         void *complete_ctx);

/**
 * @brief Wait for a specific job item to complete
 * @param pool Worker pool
 * @param job Job item to wait for
 * @param timeout_ms Timeout in milliseconds (0 for no wait, -1 for infinite)
 * @return 0 on success, negative on timeout or error
 */
int osa_worker_pool_wait(struct osa_worker_pool *pool, struct osa_worker_job *job, int timeout_ms);

/**
 * @brief Allocate a job item from the static pool
 * @return Pointer to job item, or NULL if pool exhausted
 */
struct osa_worker_job* osa_worker_job_alloc(void);

/**
 * @brief Free a job item back to the static pool
 * @param job Job item to free
 */
void osa_worker_job_free(struct osa_worker_job *job);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_WORKER_H__ */
