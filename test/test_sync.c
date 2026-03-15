/**
 * @file test_sync.c
 * @brief Synchronization Primitives Unit Tests
 */

#include "unity/unity.h"
#include "../platform/osa_sync.h"
#include "../platform/osa_error.h"

static osa_mutex_t test_mutex;
static osa_sem_t test_sem;

void test_mutex_create_destroy(void) {
    int err;

    /* Test mutex creation */
    err = osa_mutex_create(&test_mutex);
    TEST_ASSERT_EQUAL(OSA_OK, err);
    TEST_ASSERT_NOT_NULL(test_mutex);

    /* Test mutex destroy */
    osa_mutex_destroy(test_mutex);
}

void test_mutex_lock_unlock(void) {
    int err;

    err = osa_mutex_create(&test_mutex);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Test lock */
    osa_mutex_lock(test_mutex);

    /* Test unlock */
    osa_mutex_unlock(test_mutex);

    osa_mutex_destroy(test_mutex);
}

void test_mutex_trylock(void) {
    int err;

    err = osa_mutex_create(&test_mutex);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Test trylock (should succeed) */
    err = osa_mutex_trylock(test_mutex);
    TEST_ASSERT_EQUAL(0, err);

    /* Test trylock again (should fail - already locked) */
    err = osa_mutex_trylock(test_mutex);
    TEST_ASSERT_EQUAL(-1, err);

    osa_mutex_unlock(test_mutex);
    osa_mutex_destroy(test_mutex);
}

void test_sem_create_destroy(void) {
    int err;

    /* Test semaphore creation with value 0 */
    err = osa_sem_create(&test_sem, 0, 1);
    TEST_ASSERT_EQUAL(OSA_OK, err);
    TEST_ASSERT_NOT_NULL(test_sem);

    /* Test semaphore destroy */
    osa_sem_destroy(test_sem);
}

void test_sem_wait_post(void) {
    int err;

    err = osa_sem_create(&test_sem, 0, 1);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Post to semaphore */
    osa_sem_post(test_sem);

    /* Wait on semaphore (should succeed immediately) */
    err = osa_sem_wait(test_sem, -1); /* Wait forever */
    TEST_ASSERT_EQUAL(OSA_OK, err);

    osa_sem_destroy(test_sem);
}

void test_sem_timeout(void) {
    int err;

    err = osa_sem_create(&test_sem, 0, 1);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Wait with timeout (should timeout) */
    err = osa_sem_wait(test_sem, 100); /* 100ms timeout */
    /* ETIMEDOUT is -110 on Linux, we just check it's an error */
    TEST_ASSERT_TRUE(err < 0);

    osa_sem_destroy(test_sem);
}

void test_sem_get_count(void) {
    int err;
    int count;

    err = osa_sem_create(&test_sem, 2, 5);
    TEST_ASSERT_EQUAL(OSA_OK, err);

    /* Check initial count */
    count = osa_sem_get_count(test_sem);
    TEST_ASSERT_EQUAL(2, count);

    /* Post and check count */
    osa_sem_post(test_sem);
    count = osa_sem_get_count(test_sem);
    TEST_ASSERT_EQUAL(3, count);

    /* Wait and check count */
    osa_sem_wait(test_sem, -1);
    count = osa_sem_get_count(test_sem);
    TEST_ASSERT_EQUAL(2, count);

    osa_sem_destroy(test_sem);
}

void test_sync_suite(void) {
    printf("\n--- Synchronization Tests ---\n");
    RUN_TEST(test_mutex_create_destroy);
    RUN_TEST(test_mutex_lock_unlock);
    RUN_TEST(test_mutex_trylock);
    RUN_TEST(test_sem_create_destroy);
    RUN_TEST(test_sem_wait_post);
    RUN_TEST(test_sem_timeout);
    RUN_TEST(test_sem_get_count);
}
