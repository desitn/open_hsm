/**
 * @file threadpool_test.c
 * @brief Simple test for thread pool functionality
 */

#include "osa_threadpool.h"
#include <stdio.h>
#include <unistd.h>

#define UNUSED(x)  (void)(x)

static int work_count = 0;
static pthread_mutex_t result_lock = PTHREAD_MUTEX_INITIALIZER;

static int test_work_func(void *ctx)
{
    int id = *(int *)ctx;
    printf("[WORK] Starting work item %d\n", id);
    usleep(500000);  /* 500ms */
    printf("[WORK] Completed work item %d\n", id);
    
    pthread_mutex_lock(&result_lock);
    work_count++;
    pthread_mutex_unlock(&result_lock);
    
    return id * 10;  /* Return some result */
}

static void completion_callback(struct osa_work *work, int result, void *ctx)
{
    UNUSED(ctx);
    printf("[CB] Work %u completed with result %d\n", work->id, result);
}

int main(int argc, char *argv[])
{
    struct osa_threadpool pool;
    struct osa_work work_items[10];
    int ids[10];
    int i;
    struct osa_threadpool_stats stats;
    
    UNUSED(argc);
    UNUSED(argv);
    
    printf("=== Thread Pool Test ===\n\n");
    
    /* Initialize thread pool */
    if (osa_threadpool_init(&pool, "test_pool", 2, 16) != 0) {
        printf("Failed to init thread pool\n");
        return -1;
    }
    
    if (osa_threadpool_start(&pool) != 0) {
        printf("Failed to start thread pool\n");
        return -1;
    }
    
    printf("Thread pool started with 2 workers\n\n");
    
    /* Submit 5 work items */
    printf("Submitting 5 work items...\n");
    for (i = 0; i < 5; i++) {
        ids[i] = i + 1;
        osa_work_init(&work_items[i], test_work_func, &ids[i], 
                      completion_callback, NULL);
        
        if (osa_threadpool_submit_work(&pool, &work_items[i]) != 0) {
            printf("Failed to submit work %d\n", i);
        } else {
            printf("Submitted work %d (id=%u)\n", i + 1, work_items[i].id);
        }
    }
    
    /* Wait for all work to complete */
    printf("\nWaiting for completion...\n");
    for (i = 0; i < 5; i++) {
        osa_threadpool_wait_for_work(&pool, &work_items[i], -1);
    }
    
    /* Get stats */
    if (osa_threadpool_get_stats(&pool, &stats) == 0) {
        printf("\n=== Statistics ===\n");
        printf("Submitted:  %u\n", stats.total_submitted);
        printf("Completed:  %u\n", stats.total_completed);
        printf("Cancelled:  %u\n", stats.total_cancelled);
        printf("Errors:     %u\n", stats.total_errors);
        printf("Queue:      %u\n", stats.queue_depth);
        printf("Active:     %u\n", stats.active_workers);
    }
    
    printf("\nWork count: %d\n", work_count);
    
    /* Stop thread pool */
    osa_threadpool_stop(&pool);
    
    printf("\n=== Test Complete ===\n");
    
    return (work_count == 5) ? 0 : -1;
}
