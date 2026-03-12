#include "osa_threadpool.h"
#include "osa_hsm.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#define UNUSED(x)                    (void)(x)
#define osa_log(msg, ...)	         printf(msg, ##__VA_ARGS__)
#define tp_log(str, msg,...)         osa_log("[tp|%s] "msg"",str,##__VA_ARGS__)
#define core_log(msg,...)            tp_log("core", msg, ##__VA_ARGS__)

/* Global work ID counter */
static uint32_t g_work_id_counter = 0;

/* Static work pool for lightweight operation */
#define OSA_WORK_POOL_SIZE           32
static struct osa_work g_work_pool[OSA_WORK_POOL_SIZE];
static int g_work_pool_initialized = 0;

/**
 * @brief Get next work ID
 */
static uint32_t get_next_work_id(void)
{
    return ++g_work_id_counter;
}

/**
 * @brief Initialize the static work pool
 */
static void init_work_pool(void)
{
    int i;
    if (g_work_pool_initialized) {
        return;
    }
    
    for (i = 0; i < OSA_WORK_POOL_SIZE; i++) {
        memset(&g_work_pool[i], 0, sizeof(struct osa_work));
        g_work_pool[i].id = 0;
        g_work_pool[i].state = OSA_WORK_COMPLETED;
    }
    
    g_work_pool_initialized = 1;
}

/**
 * @brief Allocate a work item from the pool
 */
static struct osa_work* work_pool_alloc(void)
{
    int i;
    
    if (!g_work_pool_initialized) {
        init_work_pool();
    }
    
    for (i = 0; i < OSA_WORK_POOL_SIZE; i++) {
        if (g_work_pool[i].state == OSA_WORK_COMPLETED || 
            g_work_pool[i].state == OSA_WORK_CANCELLED) {
            memset(&g_work_pool[i], 0, sizeof(struct osa_work));
            g_work_pool[i].id = get_next_work_id();
            g_work_pool[i].state = OSA_WORK_PENDING;
            return &g_work_pool[i];
        }
    }
    
    return NULL;  /* Pool exhausted */
}

/**
 * @brief Free a work item back to the pool
 */
static void work_pool_free(struct osa_work *work)
{
    if (work >= g_work_pool && work < g_work_pool + OSA_WORK_POOL_SIZE) {
        work->state = OSA_WORK_COMPLETED;
    }
}

/**
 * @brief Worker thread function
 */
static void* osa_threadpool_worker(void *param)
{
    struct osa_threadpool *pool = (struct osa_threadpool *)param;
    struct osa_work *work = NULL;
    int ret;
    
    core_log("Worker thread started\n");
    
    while (1) {
        pthread_mutex_lock(&pool->lock);
        
        /* Wait for work or shutdown */
        while (pool->work_queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->work_available, &pool->lock);
        }
        
        /* Check for shutdown */
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        
        /* Get work from queue */
        work = pool->work_queue_head;
        if (work) {
            pool->work_queue_head = work->next;
            if (pool->work_queue_head == NULL) {
                pool->work_queue_tail = NULL;
            }
            work->next = NULL;
            work->prev = NULL;
            work->state = OSA_WORK_RUNNING;
            pool->stats.active_workers++;
            pool->stats.queue_depth--;
        }
        
        pthread_mutex_unlock(&pool->lock);
        
        if (work) {
            /* Execute the work */
            core_log("Executing work id:%u\n", work->id);
            
            if (work->work_func) {
                ret = work->work_func(work->work_ctx);
                work->result = ret;
            } else {
                work->result = -1;
            }
            
            pthread_mutex_lock(&pool->lock);
            work->state = (work->result < 0) ? OSA_WORK_ERROR : OSA_WORK_COMPLETED;
            pool->stats.active_workers--;
            
            if (work->state == OSA_WORK_ERROR) {
                pool->stats.total_errors++;
            } else {
                pool->stats.total_completed++;
            }
            
            /* Handle completion callback */
            if (work->complete_cb) {
                work->complete_cb(work, work->result, work->complete_ctx);
            }
            
            /* Handle HSM notification */
            if (work->target_hsm) {
                struct hsm_event event = {0};
                event.signal = work->completion_signal;
                event.data = work;
                event.size = sizeof(struct osa_work);
                
                ret = osa_hsm_active_event_post(work->target_hsm, &event, 0);
                if (ret != 0) {
                    core_log("Failed to post completion event to HSM\n");
                }
            }
            
            pthread_cond_broadcast(&pool->work_completed);
            pthread_mutex_unlock(&pool->lock);
        }
    }
    
    core_log("Worker thread exiting\n");
    return NULL;
}

/**
 * @brief Completion notification thread for HSM integration
 */
static void* osa_threadpool_completion_thread(void *param)
{
    struct osa_threadpool *pool = (struct osa_threadpool *)param;
    
    UNUSED(pool);
    
    /* This thread can be used for additional completion processing if needed */
    core_log("Completion thread started\n");
    
    while (!pool->shutdown) {
        /* Sleep and check for shutdown */
        usleep(100000);  /* 100ms */
    }
    
    core_log("Completion thread exiting\n");
    return NULL;
}

int osa_threadpool_init(struct osa_threadpool *pool, const char *name, 
                        uint32_t num_workers, uint32_t queue_size)
{
    int ret;
    
    if (pool == NULL) {
        return -1;
    }
    
    memset(pool, 0, sizeof(struct osa_threadpool));
    
    /* Set defaults */
    pool->num_workers = (num_workers == 0) ? OSA_THREADPOOL_DEFAULT_WORKERS : num_workers;
    pool->queue_size = (queue_size == 0) ? OSA_THREADPOOL_QUEUE_SIZE : queue_size;
    
    if (pool->num_workers > OSA_THREADPOOL_MAX_WORKERS) {
        pool->num_workers = OSA_THREADPOOL_MAX_WORKERS;
    }
    
    /* Copy name */
    if (name) {
        pool->name = strdup(name);
    } else {
        pool->name = strdup("tp");
    }
    
    /* Initialize synchronization primitives */
    ret = pthread_mutex_init(&pool->lock, NULL);
    if (ret != 0) {
        core_log("Failed to init mutex: %s\n", strerror(ret));
        return -1;
    }
    
    ret = pthread_cond_init(&pool->work_available, NULL);
    if (ret != 0) {
        core_log("Failed to init work_available cond: %s\n", strerror(ret));
        pthread_mutex_destroy(&pool->lock);
        return -1;
    }
    
    ret = pthread_cond_init(&pool->work_completed, NULL);
    if (ret != 0) {
        core_log("Failed to init work_completed cond: %s\n", strerror(ret));
        pthread_cond_destroy(&pool->work_available);
        pthread_mutex_destroy(&pool->lock);
        return -1;
    }
    
    /* Allocate worker thread array */
    pool->workers = (pthread_t *)malloc(sizeof(pthread_t) * pool->num_workers);
    if (pool->workers == NULL) {
        core_log("Failed to allocate worker array\n");
        pthread_cond_destroy(&pool->work_completed);
        pthread_cond_destroy(&pool->work_available);
        pthread_mutex_destroy(&pool->lock);
        return -1;
    }
    
    pool->shutdown = false;
    
    core_log("Thread pool '%s' initialized with %u workers, queue size %u\n", 
             pool->name, pool->num_workers, pool->queue_size);
    
    return 0;
}

int osa_threadpool_start(struct osa_threadpool *pool)
{
    int ret;
    uint32_t i;
    pthread_attr_t attr;
    struct sched_param param;
    
    if (pool == NULL || pool->workers == NULL) {
        return -1;
    }
    
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, OSA_THREADPOOL_STACK_SIZE);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = OSA_THREADPOOL_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    
    /* Create worker threads */
    for (i = 0; i < pool->num_workers; i++) {
        ret = pthread_create(&pool->workers[i], &attr, osa_threadpool_worker, pool);
        if (ret != 0) {
            core_log("Failed to create worker thread %u: %s\n", i, strerror(ret));
            pthread_attr_destroy(&attr);
            return -1;
        }
    }
    
    /* Create completion thread */
    ret = pthread_create(&pool->completion_thread, &attr, osa_threadpool_completion_thread, pool);
    if (ret != 0) {
        core_log("Failed to create completion thread: %s\n", strerror(ret));
        /* Non-fatal, continue without completion thread */
    }
    
    pthread_attr_destroy(&attr);
    
    core_log("Thread pool '%s' started with %u workers\n", pool->name, pool->num_workers);
    
    return 0;
}

int osa_threadpool_stop(struct osa_threadpool *pool)
{
    uint32_t i;
    void *retval;
    
    if (pool == NULL) {
        return -1;
    }
    
    core_log("Stopping thread pool '%s'...\n", pool->name);
    
    /* Signal shutdown */
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->work_available);
    pthread_mutex_unlock(&pool->lock);
    
    /* Wait for worker threads to finish */
    for (i = 0; i < pool->num_workers; i++) {
        pthread_join(pool->workers[i], &retval);
    }
    
    /* Wait for completion thread */
    pthread_join(pool->completion_thread, &retval);
    
    /* Cleanup */
    free(pool->workers);
    pool->workers = NULL;
    
    pthread_cond_destroy(&pool->work_completed);
    pthread_cond_destroy(&pool->work_available);
    pthread_mutex_destroy(&pool->lock);
    
    if (pool->name) {
        free(pool->name);
        pool->name = NULL;
    }
    
    core_log("Thread pool stopped\n");
    
    return 0;
}

int osa_threadpool_submit_work(struct osa_threadpool *pool, struct osa_work *work)
{
    int ret = 0;
    
    if (pool == NULL || work == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    /* Check if queue is full */
    if (pool->stats.queue_depth >= pool->queue_size) {
        core_log("Work queue full, cannot submit work\n");
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }
    
    /* Assign work ID if not set */
    if (work->id == 0) {
        work->id = get_next_work_id();
    }
    
    work->state = OSA_WORK_PENDING;
    work->next = NULL;
    work->prev = NULL;
    
    /* Add to queue */
    if (pool->work_queue_tail == NULL) {
        pool->work_queue_head = work;
        pool->work_queue_tail = work;
    } else {
        pool->work_queue_tail->next = work;
        work->prev = pool->work_queue_tail;
        pool->work_queue_tail = work;
    }
    
    pool->stats.queue_depth++;
    pool->stats.total_submitted++;
    
    core_log("Work id:%u submitted, queue depth:%u\n", work->id, pool->stats.queue_depth);
    
    /* Signal worker threads */
    pthread_cond_signal(&pool->work_available);
    pthread_mutex_unlock(&pool->lock);
    
    return ret;
}

int osa_threadpool_submit_work_to_hsm(struct osa_threadpool *pool, struct osa_work *work,
                                       struct osa_hsm_active *target_hsm, 
                                       int completion_signal)
{
    if (work == NULL) {
        return -1;
    }
    
    work->target_hsm = target_hsm;
    work->completion_signal = completion_signal;
    
    return osa_threadpool_submit_work(pool, work);
}

int osa_threadpool_cancel_work(struct osa_threadpool *pool, uint32_t work_id)
{
    struct osa_work *work;
    int found = 0;
    
    if (pool == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    /* Search in queue */
    work = pool->work_queue_head;
    while (work) {
        if (work->id == work_id) {
            /* Remove from queue */
            if (work->prev) {
                work->prev->next = work->next;
            } else {
                pool->work_queue_head = work->next;
            }
            
            if (work->next) {
                work->next->prev = work->prev;
            } else {
                pool->work_queue_tail = work->prev;
            }
            
            work->state = OSA_WORK_CANCELLED;
            pool->stats.queue_depth--;
            pool->stats.total_cancelled++;
            found = 1;
            break;
        }
        work = work->next;
    }
    
    pthread_mutex_unlock(&pool->lock);
    
    if (found) {
        core_log("Work id:%u cancelled\n", work_id);
        return 0;
    }
    
    /* Work not found in queue (might be running or already completed) */
    return -1;
}

int osa_threadpool_get_stats(struct osa_threadpool *pool, struct osa_threadpool_stats *stats)
{
    if (pool == NULL || stats == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->lock);
    memcpy(stats, &pool->stats, sizeof(struct osa_threadpool_stats));
    pthread_mutex_unlock(&pool->lock);
    
    return 0;
}

void osa_work_init(struct osa_work *work,
                   int (*work_func)(void *ctx),
                   void *work_ctx,
                   void (*complete_cb)(struct osa_work *work, int result, void *ctx),
                   void *complete_ctx)
{
    if (work == NULL) {
        return;
    }
    
    memset(work, 0, sizeof(struct osa_work));
    
    work->id = get_next_work_id();
    work->state = OSA_WORK_PENDING;
    work->work_func = work_func;
    work->work_ctx = work_ctx;
    work->complete_cb = complete_cb;
    work->complete_ctx = complete_ctx;
    work->target_hsm = NULL;
    work->completion_signal = 0;
    work->result = 0;
    work->next = NULL;
    work->prev = NULL;
}

int osa_threadpool_wait_for_work(struct osa_threadpool *pool, struct osa_work *work, int timeout_ms)
{
    int ret = 0;
    struct timespec ts;
    struct timeval tv;
    
    if (pool == NULL || work == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    if (timeout_ms == 0) {
        /* No wait - just check status */
        if (work->state != OSA_WORK_COMPLETED && work->state != OSA_WORK_CANCELLED) {
            ret = -1;  /* Still pending or running */
        }
    } else if (timeout_ms < 0) {
        /* Infinite wait */
        while (work->state != OSA_WORK_COMPLETED && work->state != OSA_WORK_CANCELLED) {
            pthread_cond_wait(&pool->work_completed, &pool->lock);
        }
    } else {
        /* Timed wait */
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + timeout_ms / 1000;
        ts.tv_nsec = tv.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        while (work->state != OSA_WORK_COMPLETED && work->state != OSA_WORK_CANCELLED) {
            ret = pthread_cond_timedwait(&pool->work_completed, &pool->lock, &ts);
            if (ret == ETIMEDOUT) {
                ret = -1;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&pool->lock);
    
    return ret;
}

/**
 * @brief Allocate a work item from the static pool
 * @return Pointer to work item, or NULL if pool exhausted
 */
struct osa_work* osa_work_alloc(void)
{
    return work_pool_alloc();
}

/**
 * @brief Free a work item back to the static pool
 * @param work Work item to free
 */
void osa_work_free(struct osa_work *work)
{
    work_pool_free(work);
}
