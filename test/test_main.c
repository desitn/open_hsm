/**
 * @file test_main.c
 * @brief Unit Test Main Entry
 *
 * Uses Unity test framework for embedded C testing.
 */

#include "unity/unity.h"
#include <stdio.h>

/* Test suite declarations */
extern void test_error_suite(void);
extern void test_sync_suite(void);
extern void test_worker_suite(void);

/* Unity setUp and tearDown */
void setUp(void) {
    /* Setup before each test */
}

void tearDown(void) {
    /* Cleanup after each test */
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("    OSA Unit Test Suite\n");
    printf("========================================\n\n");

    UNITY_BEGIN();

    /* Run test suites */
    test_error_suite();
    test_sync_suite();
    test_worker_suite();

    return UNITY_END();
}
