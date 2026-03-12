#ifndef __OSA_THREADPOOL_H__
#define __OSA_THREADPOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hsm.h"
#include <stdint.h>
#include <stdbool.h>

#if defined(__linux__)
#include <pthread.h>
#include <mqueue.h>
#else
// RTOS includes would go here
#endif

/* Thread pool configuration */
#define OSA_THREADPOOL_DEFAULT_WORKERS   4
#define OSA_THREADPOOL_MAX_WORKERS       16
#define OSA_THREADPOOL_QUEUE_SIZE        64
#define OSA_THREADPOOL_STACK_SIZE        (1024 * 8)
#define OSA_THREADPOOL_PRIORITY          5

/* Work item states */
enum osa_work_state {
    OSA_WORK_PENDING,      /* Work is queued waiting for execution */
    OSA_WORK_RUNNING,      /* Work is currently being executed */
    OSA_WORK_COMPLETED,    /* Work completed successfully */
    OSA_WORK_CANCELLED,    /* Work was cancelled */
    OSA_WORK_ERROR,        /* Work encountered an error */
};

/* Work item structure */
struct osa_work {
    uint32_t id;                          /* Unique work item ID */
    enum osa_work_state state;            /* Current state of work */
    
    /* Work function and context */
    int (*work_func)(void *ctx);          /* The actual work function */
    void *work_ctx;                       /* Context passed to work function */
    
    /* Completion callback */
    void (*complete_cb)(struct osa_work *work, int result, void *ctx);
    void *complete_ctx;                   /* Context for completion callback */
    
    /* HSM integration - target HSM to receive completion event */
    struct osa_hsm_active *target_hsm;    /* HSM to notify on completion */
    int completion_signal;                /* Signal to send to HSM on completion */
    
    /* Result */
    int result;                           /* Result from work function */
    
    /* Linked list for queue management */
    struct osa_work *next;
    struct osa_work *prev;
};

/* Thread pool statistics */
struct osa_threadpool_stats {
    uint32_t total_submitted;     /* Total work items submitted */
    uint32_t total_completed;     /* Total work items completed */
    uint32_t total_cancelled;     /* Total work items cancelled */
    uint32_t total_errors;        /* Total work items with errors */
    uint32_t queue_depth;         /* Current queue depth */
    uint32_t active_workers;      /* Number of currently active workers */
};

/* Thread pool structure */
struct osa_threadpool {
    char *name;                       /* Thread pool name */
    
#if defined(__linux__)
    pthread_t *workers;               /* Array of worker threads */
    pthread_mutex_t lock;             /* Mutex for thread safety */
    pthread_cond_t work_available;    /* Condition variable for work availability */
    pthread_cond_t work_completed;    /* Condition variable for work completion */
    mqd_t completion_mq;              /* Message queue for completion notifications */
#else
    /* RTOS equivalents */
    void *workers;
    void *lock;
    void *work_sem;
    void *completion_mq;
#endif
    
    struct osa_work *work_queue_head; /* Head of work queue */
    struct osa_work *work_queue_tail; /* Tail of work queue */
    struct osa_work *free_list;       /* Free list of work structures */
    
    uint32_t num_workers;             /* Number of worker threads */
    uint32_t queue_size;              /* Maximum queue size */
    bool shutdown;                    /* Shutdown flag */
    
    struct osa_threadpool_stats stats; /* Statistics */
    
    /* Worker thread for HSM completion notifications */
    pthread_t completion_thread;
};

/* Completion event sent to HSM */
struct osa_work_completion_event {
    uint32_t work_id;                 /* ID of completed work */
    int result;                       /* Result from work function */
    void *user_ctx;                   /* User context from work item */
};

/* Thread pool API */

/**
 * @brief Initialize a thread pool
 * @param pool Thread pool structure to initialize
 * @param name Name for the thread pool
 * @param num_workers Number of worker threads (0 for default)
 * @param queue_size Maximum work queue size (0 for default)
 * @return 0 on success, negative on error
 */
int osa_threadpool_init(struct osa_threadpool *pool, const char *name, 
                        uint32_t num_workers, uint32_t queue_size);

/**
 * @brief Start the thread pool
 * @param pool Thread pool to start
 * @return 0 on success, negative on error
 */
int osa_threadpool_start(struct osa_threadpool *pool);

/**
 * @brief Stop the thread pool and cleanup resources
 * @param pool Thread pool to stop
 * @return 0 on success, negative on error
 */
int osa_threadpool_stop(struct osa_threadpool *pool);

/**
 * @brief Submit work to the thread pool
 * @param pool Thread pool to submit work to
 * @param work Work item to execute
 * @return 0 on success, negative on error
 */
int osa_threadpool_submit_work(struct osa_threadpool *pool, struct osa_work *work);

/**
 * @brief Submit work with automatic HSM notification on completion
 * @param pool Thread pool to submit work to
 * @param work Work item to execute
 * @param target_hsm HSM to notify on completion
 * @param completion_signal Signal to send to HSM
 * @return 0 on success, negative on error
 */
int osa_threadpool_submit_work_to_hsm(struct osa_threadpool *pool, struct osa_work *work,
                                       struct osa_hsm_active *target_hsm, 
                                       int completion_signal);

/**
 * @brief Cancel a pending work item
 * @param pool Thread pool
 * @param work_id ID of work to cancel
 * @return 0 on success, negative on error
 */
int osa_threadpool_cancel_work(struct osa_threadpool *pool, uint32_t work_id);

/**
 * @brief Get thread pool statistics
 * @param pool Thread pool
 * @param stats Structure to fill with statistics
 * @return 0 on success, negative on error
 */
int osa_threadpool_get_stats(struct osa_threadpool *pool, struct osa_threadpool_stats *stats);

/**
 * @brief Initialize a work item structure
 * @param work Work item to initialize
 * @param work_func Function to execute
 * @param work_ctx Context for work function
 * @param complete_cb Optional completion callback
 * @param complete_ctx Context for completion callback
 */
void osa_work_init(struct osa_work *work,
                   int (*work_func)(void *ctx),
                   void *work_ctx,
                   void (*complete_cb)(struct osa_work *work, int result, void *ctx),
                   void *complete_ctx);

/**
 * @brief Wait for a specific work item to complete
 * @param pool Thread pool
 * @param work Work item to wait for
 * @param timeout_ms Timeout in milliseconds (0 for no wait, -1 for infinite)
 * @return 0 on success, negative on timeout or error
 */
int osa_threadpool_wait_for_work(struct osa_threadpool *pool, struct osa_work *work, int timeout_ms);

/**
 * @brief Allocate a work item from the static pool
 * @return Pointer to work item, or NULL if pool exhausted
 */
struct osa_work* osa_work_alloc(void);

/**
 * @brief Free a work item back to the static pool
 * @param work Work item to free
 */
void osa_work_free(struct osa_work *work);

#ifdef __cplusplus
}
#endif

#endif /* __OSA_THREADPOOL_H__ */
