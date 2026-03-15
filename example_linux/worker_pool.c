/**
 * @file worker_pool.c
 * @brief Simple test for worker pool functionality
 *
 * This example demonstrates proper usage of OSA (Operating System Abstraction)
 * interfaces. All synchronization primitives use OSA APIs instead of native
 * platform APIs, ensuring cross-platform compatibility.
 */

#include "osa_worker.h"
#include "../platform/osa_sync.h"
#include <stdio.h>
#include <unistd.h>

#define UNUSED(x)  (void)(x)

static int job_count = 0;
static osa_mutex_t result_lock;  /* Use OSA mutex instead of pthread_mutex_t */

static int test_job_func(void *ctx)
{
    int id = *(int *)ctx;
    printf("[JOB] Starting job item %d\n", id);
    usleep(500000);  /* 500ms */
    printf("[JOB] Completed job item %d\n", id);

    /* Use OSA mutex APIs */
    osa_mutex_lock(result_lock);
    job_count++;
    osa_mutex_unlock(result_lock);

    return id * 10;  /* Return some result */
}

static void completion_callback(struct osa_worker_job *job, int result, void *ctx)
{
    UNUSED(ctx);
    printf("[CB] Job %u completed with result %d\n", job->id, result);
}

int main(int argc, char *argv[])
{
    struct osa_worker_pool pool;
    struct osa_worker_job job_items[10];
    int ids[10];
    int i;
    struct osa_worker_pool_stats stats;

    UNUSED(argc);
    UNUSED(argv);

    printf("=== Worker Pool Test ===\n\n");

    /* Initialize OSA mutex */
    if (osa_mutex_create(&result_lock) != 0) {
        printf("Failed to create mutex\n");
        return -1;
    }

    /* Initialize worker pool */
    if (osa_worker_pool_init(&pool, "test_pool", 2, 16) != 0) {
        printf("Failed to init worker pool\n");
        osa_mutex_destroy(result_lock);
        return -1;
    }

    if (osa_worker_pool_start(&pool) != 0) {
        printf("Failed to start worker pool\n");
        osa_mutex_destroy(result_lock);
        return -1;
    }

    printf("Worker pool started with 2 workers\n\n");

    /* Submit 5 job items */
    printf("Submitting 5 job items...\n");
    for (i = 0; i < 5; i++) {
        ids[i] = i + 1;
        osa_worker_job_init(&job_items[i], test_job_func, &ids[i],
                      completion_callback, NULL);

        if (osa_worker_pool_submit(&pool, &job_items[i]) != 0) {
            printf("Failed to submit job %d\n", i);
        } else {
            printf("Submitted job %d (id=%u)\n", i + 1, job_items[i].id);
        }
    }

    /* Wait for all jobs to complete */
    printf("\nWaiting for completion...\n");
    for (i = 0; i < 5; i++) {
        osa_worker_pool_wait(&pool, &job_items[i], -1);
    }

    /* Get stats */
    if (osa_worker_pool_get_stats(&pool, &stats) == 0) {
        printf("\n=== Statistics ===\n");
        printf("Submitted:  %u\n", stats.total_submitted);
        printf("Completed:  %u\n", stats.total_completed);
        printf("Cancelled:  %u\n", stats.total_cancelled);
        printf("Errors:     %u\n", stats.total_errors);
        printf("Queue:      %u\n", stats.queue_depth);
        printf("Active:     %u\n", stats.active_workers);
    }

    printf("\nJob count: %d\n", job_count);

    /* Stop worker pool */
    osa_worker_pool_stop(&pool);

    /* Destroy OSA mutex */
    osa_mutex_destroy(result_lock);

    printf("\n=== Test Complete ===\n");

    return (job_count == 5) ? 0 : -1;
}
