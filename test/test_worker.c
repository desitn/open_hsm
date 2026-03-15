/**
 * @file test_worker.c
 * @brief Worker Pool Unit Tests
 */

#include "unity/unity.h"
#include "../core/osa_worker.h"
#include "../platform/osa_error.h"
#include <unistd.h>

static struct osa_worker_pool test_pool;
static int job_result = 0;
static int job_count = 0;

static int test_job_func(void *arg) {
    int value = *(int*)arg;
    job_result += value;
    job_count++;
    usleep(1000); /* 1ms delay */
    return value * 2;
}

void test_worker_pool_init_destroy(void) {
    int err;

    /* Test initialization */
    err = osa_worker_pool_init(&test_pool, "test_pool", 2, 8);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Test destroy */
    err = osa_worker_pool_stop(&test_pool);
    TEST_ASSERT_EQUAL(OSA_OK, err);
}

void test_worker_pool_start_stop(void) {
    int err;

    err = osa_worker_pool_init(&test_pool, "test_pool", 2, 8);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Test start */
    err = osa_worker_pool_start(&test_pool);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Test stop */
    err = osa_worker_pool_stop(&test_pool);
    TEST_ASSERT_EQUAL(OSA_OK, err);
}

void test_worker_pool_submit_job(void) {
    int err;
    int arg = 5;
    struct osa_worker_job *job;

    err = osa_worker_pool_init(&test_pool, "test_pool", 2, 8);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    err = osa_worker_pool_start(&test_pool);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Reset counters */
    job_result = 0;
    job_count = 0;

    /* Allocate and initialize job */
    job = osa_worker_job_alloc();
    TEST_ASSERT_NOT_NULL(job);

    osa_worker_job_init(job, test_job_func, &arg, NULL, NULL);

    /* Submit job */
    err = osa_worker_pool_submit(&test_pool, job);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Wait for job to complete */
    usleep(10000); /* 10ms */

    /* Verify job was executed */
    TEST_ASSERT_EQUAL(1, job_count);
    TEST_ASSERT_EQUAL(5, job_result);

    osa_worker_pool_stop(&test_pool);
}

void test_worker_pool_submit_multiple(void) {
    int err;
    int args[5] = {1, 2, 3, 4, 5};
    struct osa_worker_job *jobs[5];
    int i;

    err = osa_worker_pool_init(&test_pool, "test_pool", 2, 8);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    err = osa_worker_pool_start(&test_pool);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Reset counters */
    job_result = 0;
    job_count = 0;

    /* Submit multiple jobs */
    for (i = 0; i < 5; i++) {
        jobs[i] = osa_worker_job_alloc();
        TEST_ASSERT_NOT_NULL(jobs[i]);
        osa_worker_job_init(jobs[i], test_job_func, &args[i], NULL, NULL);
        err = osa_worker_pool_submit(&test_pool, jobs[i]);
        TEST_ASSERT_EQUAL(OSA_OK, err);
    }

    /* Wait for all jobs to complete */
    usleep(50000); /* 50ms */

    /* Verify all jobs were executed */
    TEST_ASSERT_EQUAL(5, job_count);
    TEST_ASSERT_EQUAL(15, job_result); /* 1+2+3+4+5 */

    osa_worker_pool_stop(&test_pool);
}

void test_worker_pool_stats(void) {
    int err;
    struct osa_worker_pool_stats stats;
    int arg = 1;
    struct osa_worker_job *job;

    err = osa_worker_pool_init(&test_pool, "test_pool", 2, 8);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    err = osa_worker_pool_start(&test_pool);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Get initial stats */
    osa_worker_pool_get_stats(&test_pool, &stats);
    TEST_ASSERT_EQUAL(0, stats.total_submitted);
    TEST_ASSERT_EQUAL(0, stats.total_completed);

    /* Allocate and submit a job */
    job = osa_worker_job_alloc();
    TEST_ASSERT_NOT_NULL(job);
    osa_worker_job_init(job, test_job_func, &arg, NULL, NULL);
    osa_worker_pool_submit(&test_pool, job);

    /* Wait for job */
    usleep(10000);

    /* Get updated stats */
    osa_worker_pool_get_stats(&test_pool, &stats);
    TEST_ASSERT_EQUAL(1, stats.total_submitted);
    TEST_ASSERT_EQUAL(1, stats.total_completed);

    osa_worker_pool_stop(&test_pool);
}

void test_worker_suite(void) {
    printf("\n--- Worker Pool Tests ---\n");
    RUN_TEST(test_worker_pool_init_destroy);
    RUN_TEST(test_worker_pool_start_stop);
    RUN_TEST(test_worker_pool_submit_job);
    RUN_TEST(test_worker_pool_submit_multiple);
    RUN_TEST(test_worker_pool_stats);
}
