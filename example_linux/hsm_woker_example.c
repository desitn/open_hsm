/**
 * @file hsm_threadpool_example.c
 * @brief Example demonstrating HSM integration with worker pool for async operations
 *
 * This example shows how to use the worker pool to offload time-consuming
 * operations from the HSM state machine and receive completion events asynchronously.
 */

#include "hsm_simulate.h"
#include "osa_worker.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define UNUSED(x)  (void)(x)

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
#define wp_log(msg,...)              hsm_log("wp", msg, ##__VA_ARGS__)

#define EXAMPLE_TASK_STACK_SIZE      (1024*10)
#define EXAMPLE_TASK_PRIO            (10)
#define EXAMPLE_TASK_MSG_Q_SIZE      (20)

/* Custom signals for worker pool example */
enum worker_pool_example_signals {
    SIG_WORK_START = HSM_USER_SIG,
    SIG_WORK_COMPLETE,
    SIG_WORK_CANCEL,
    SIG_WORK_PROGRESS,
    SIG_WORK_ERROR,
    SIG_MAX
};

/* Example context structure */
struct worker_pool_example {
    struct osa_hsm_active super;
    struct osa_worker_pool *worker_pool;

    /* Job tracking */
    struct osa_worker_job *current_job;
    uint32_t job_count;
    uint32_t completed_count;
    uint32_t error_count;

    /* Simulated async operation state */
    int operation_progress;
    int operation_result;
};

/* Global instances */
struct worker_pool_example g_example1 = {0};
struct worker_pool_example g_example2 = {0};
struct osa_worker_pool g_worker_pool = {0};

/* State handlers */
static int state_idle_handler(struct worker_pool_example *example, struct hsm_event *event);
static int state_working_handler(struct worker_pool_example *example, struct hsm_event *event);
static int state_processing_handler(struct worker_pool_example *example, struct hsm_event *event);

/* State definitions */
static struct hsm_state state_idle = {
    .name = "idle",
    .handler = (hsm_state_handler)&state_idle_handler,
    .parent = NULL,
};

static struct hsm_state state_working = {
    .name = "working",
    .handler = (hsm_state_handler)&state_working_handler,
    .parent = NULL,
};

static struct hsm_state state_processing = {
    .name = "processing",
    .handler = (hsm_state_handler)&state_processing_handler,
    .parent = &state_working,  /* Substate of working */
};

/* Signal names for debugging */
static const char* g_sig_str[SIG_MAX - HSM_USER_SIG] = {
    [SIG_WORK_START - HSM_USER_SIG] = "work_start",
    [SIG_WORK_COMPLETE - HSM_USER_SIG] = "work_complete",
    [SIG_WORK_CANCEL - HSM_USER_SIG] = "work_cancel",
    [SIG_WORK_PROGRESS - HSM_USER_SIG] = "work_progress",
    [SIG_WORK_ERROR - HSM_USER_SIG] = "work_error",
};

static const char* get_sig_name(int signal)
{
    const char* str = NULL;
    if (signal >= HSM_USER_SIG && signal < SIG_MAX) {
        str = g_sig_str[signal - HSM_USER_SIG];
    }
    if (str == NULL) {
        str = oas_hsm_sig_str(signal);
    }
    return str;
}

/**
 * @brief Simulated time-consuming job function
 * This runs in a worker thread, not in the HSM context
 */
static int simulate_heavy_computation(void *ctx)
{
    struct worker_pool_example *example = (struct worker_pool_example *)ctx;
    int i;
    int result = 0;

    wp_log("Starting heavy computation for %s\n", example->super.name);

    /* Simulate CPU-intensive work */
    for (i = 0; i < 5; i++) {
        /* Check if we should cancel */
        if (example->current_job &&
            example->current_job->state == OSA_JOB_CANCELLED) {
            wp_log("Job cancelled for %s\n", example->super.name);
            return -1;
        }

        /* Simulate work */
        usleep(500000);  /* 500ms of work */

        example->operation_progress = (i + 1) * 20;
        wp_log("Progress for %s: %d%%\n", example->super.name, example->operation_progress);
    }

    /* Simulate occasional errors */
    if (example->job_count % 7 == 0) {
        wp_log("Simulated error for %s\n", example->super.name);
        result = -1;
    } else {
        result = example->job_count * 10;
    }

    wp_log("Heavy computation completed for %s with result %d\n",
           example->super.name, result);

    return result;
}

/**
 * @brief Completion callback - called when job finishes
 * This runs in the worker thread context
 */
static void job_completion_callback(struct osa_worker_job *job, int result, void *ctx)
{
    struct worker_pool_example *example = (struct worker_pool_example *)ctx;

    wp_log("Completion callback for %s: job_id=%u, result=%d\n",
           example->super.name, job->id, result);

    example->operation_result = result;
}

/**
 * @brief Idle state handler
 */
static int state_idle_handler(struct worker_pool_example *example, struct hsm_event *event)
{
    int ret = 0;

    STATE_ENTRY(&(example->super.super));
    app_log("%s (%s) event:%d [%s]\n",
            example->super.name, STATE_NAME(),
            event->signal, get_sig_name(event->signal));

    switch (event->signal) {
    case HSM_SIG_ENTRY:
        /* Reset progress */
        example->operation_progress = 0;
        example->operation_result = 0;
        break;

    case SIG_WORK_START:
        /* Transition to working state to start async operation */
        STATE_TRANSIT(&state_working);
        ret = STATE_ERROR();
        break;

    default:
        break;
    }

    return ret;
}

/**
 * @brief Working state handler (superstate)
 */
static int state_working_handler(struct worker_pool_example *example, struct hsm_event *event)
{
    int ret = 0;
    struct osa_worker_job *job;

    STATE_ENTRY(&(example->super.super));
    app_log("%s (%s) event:%d [%s]\n",
            example->super.name, STATE_NAME(),
            event->signal, get_sig_name(event->signal));

    switch (event->signal) {
    case HSM_SIG_ENTRY:
        /* Initialize and submit job to worker pool */
        example->job_count++;

        /* Allocate job from pool */
        job = osa_worker_job_alloc();
        if (job == NULL) {
            app_log("Failed to allocate job item\n");
            /* Transition back to idle on error */
            STATE_TRANSIT(&state_idle);
            ret = STATE_ERROR();
            break;
        }

        example->current_job = job;

        /* Initialize job item */
        osa_worker_job_init(job,
                      simulate_heavy_computation,   /* Job function */
                      example,                       /* Job context */
                      job_completion_callback,       /* Completion callback */
                      example);                      /* Callback context */

        /* Submit job to worker pool with HSM notification */
        ret = osa_worker_pool_submit_to_hsm(
            example->worker_pool,
            job,
            &example->super,      /* Target HSM */
            SIG_WORK_COMPLETE);   /* Signal to send on completion */

        if (ret != 0) {
            app_log("Failed to submit job to worker pool\n");
            osa_worker_job_free(job);
            example->current_job = NULL;
            STATE_TRANSIT(&state_idle);
            ret = STATE_ERROR();
        } else {
            app_log("Job submitted to worker pool, id=%u\n", job->id);
            /* Transition to processing substate */
            STATE_TRANSIT(&state_processing);
            ret = STATE_ERROR();
        }
        break;

    case SIG_WORK_COMPLETE:
        /* Job completed - handle result */
        job = (struct osa_worker_job *)event->data;
        if (job) {
            app_log("Job completed: id=%u, result=%d\n", job->id, job->result);

            if (job->result < 0) {
                example->error_count++;
                /* Could transition to error state here */
            } else {
                example->completed_count++;
            }

            /* Free job item back to pool */
            osa_worker_job_free(job);
            example->current_job = NULL;
        }

        /* Transition back to idle */
        STATE_TRANSIT(&state_idle);
        ret = STATE_ERROR();
        break;

    case SIG_WORK_CANCEL:
        /* Cancel pending job */
        if (example->current_job) {
            osa_worker_pool_cancel(example->worker_pool, example->current_job->id);
            app_log("Job cancellation requested for id=%u\n", example->current_job->id);
        }
        break;

    case SIG_WORK_START:
        /* Ignore duplicate job start when already working */
        break;

    default:
        /* Let superstate handle it */
        STATE_SUPER(event);
        break;
    }

    return ret;
}

/**
 * @brief Processing state handler (substate of working)
 * This state represents active processing with progress updates
 */
static int state_processing_handler(struct worker_pool_example *example, struct hsm_event *event)
{
    int ret = 0;

    STATE_ENTRY(&(example->super.super));
    app_log("%s (%s) event:%d [%s] progress:%d%%\n",
            example->super.name, STATE_NAME(),
            event->signal, get_sig_name(event->signal),
            example->operation_progress);

    switch (event->signal) {
    case HSM_SIG_ENTRY:
        /* Could start a timer for progress updates here */
        break;

    case HSM_SIG_PERIOD:
        /* Periodic check - could poll job status */
        app_log("Processing... %d%% complete\n", example->operation_progress);
        break;

    case SIG_WORK_PROGRESS:
        /* Progress update received */
        app_log("Progress update: %d%%\n", example->operation_progress);
        break;

    case SIG_WORK_ERROR:
        /* Error during processing */
        app_log("Error during processing\n");
        example->error_count++;
        /* Transition to parent working state which will handle completion */
        STATE_TRANSIT(&state_working);
        ret = STATE_ERROR();
        break;

    default:
        /* Delegate to parent state */
        STATE_SUPER(event);
        break;
    }

    return ret;
}

/**
 * @brief Create and start the example HSM
 */
static int example_create(struct worker_pool_example *example, const char *name)
{
    int ret;
    struct osa_hsm_active *hsm = &example->super;

    example->super.name = (char *)name;
    example->worker_pool = &g_worker_pool;

    ret = osa_hsm_active_init(hsm, &state_idle);
    if (ret != 0) {
        app_log("Failed to init HSM\n");
        return ret;
    }

    ret = osa_hsm_active_start(hsm, EXAMPLE_TASK_STACK_SIZE,
                               EXAMPLE_TASK_PRIO, EXAMPLE_TASK_MSG_Q_SIZE);
    if (ret != 0) {
        app_log("Failed to start HSM\n");
        return ret;
    }

    app_log("Example HSM '%s' created\n", name);
    return 0;
}

/**
 * @brief Post an event to the example HSM
 */
static int example_post_event(struct worker_pool_example *example, int signal)
{
    struct hsm_event event = {0};
    event.signal = signal;
    return osa_hsm_active_event_post(&example->super, &event, 0);
}

/**
 * @brief Print worker pool statistics
 */
static void print_worker_pool_stats(struct osa_worker_pool *pool)
{
    struct osa_worker_pool_stats stats;

    if (osa_worker_pool_get_stats(pool, &stats) == 0) {
        app_log("Worker Pool Stats:\n");
        app_log("  Submitted:  %u\n", stats.total_submitted);
        app_log("  Completed:  %u\n", stats.total_completed);
        app_log("  Cancelled:  %u\n", stats.total_cancelled);
        app_log("  Errors:     %u\n", stats.total_errors);
        app_log("  Queue:      %u\n", stats.queue_depth);
        app_log("  Active:     %u\n", stats.active_workers);
    }
}

int main(int argc, char* argv[])
{
    int ret;
    int i;

    UNUSED(argc);
    UNUSED(argv);

    app_log("=== HSM Worker Pool Example ===\n");
    app_log("Demonstrating async work offload with HSM integration\n\n");

    /* Initialize worker pool */
    ret = osa_worker_pool_init(&g_worker_pool, "main_pool", 4, 64);
    if (ret != 0) {
        app_log("Failed to init worker pool\n");
        return -1;
    }

    ret = osa_worker_pool_start(&g_worker_pool);
    if (ret != 0) {
        app_log("Failed to start worker pool\n");
        return -1;
    }

    app_log("Worker pool started with 4 workers\n\n");

    /* Create example HSMs */
    example_create(&g_example1, "worker1");
    example_create(&g_example2, "worker2");

    /* Start periodic timer for progress updates */
    osa_hsm_active_period(&g_example1.super, 1000);  /* 1 second */
    osa_hsm_active_period(&g_example2.super, 1000);  /* 1 second */

    /* Main loop - submit jobs periodically */
    app_log("\nStarting job submission loop...\n");

    for (i = 0; i < 3; i++) {
        app_log("\n--- Iteration %d ---\n", i + 1);

        /* Submit jobs to both HSMs */
        example_post_event(&g_example1, SIG_WORK_START);
        usleep(100000);  /* Small delay */
        example_post_event(&g_example2, SIG_WORK_START);

        /* Wait for completion (each job takes ~2.5 seconds) */
        sleep(4);

        /* Print stats */
        print_worker_pool_stats(&g_worker_pool);
        app_log("Example1: jobs=%u, completed=%u, errors=%u\n",
                g_example1.job_count, g_example1.completed_count, g_example1.error_count);
        app_log("Example2: jobs=%u, completed=%u, errors=%u\n",
                g_example2.job_count, g_example2.completed_count, g_example2.error_count);
    }

    app_log("\n=== Test complete, cleaning up ===\n");

    /* Stop periodic timers */
    osa_hsm_active_period(&g_example1.super, 0);
    osa_hsm_active_period(&g_example2.super, 0);

    /* Stop worker pool */
    osa_worker_pool_stop(&g_worker_pool);

    app_log("Cleanup complete\n");

    return 0;
}
