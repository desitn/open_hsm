/**
 * @file test_error.c
 * @brief Error Code System Unit Tests
 */

#include "unity/unity.h"
#include "../platform/osa_error.h"

void test_error_code_values(void) {
    /* Test that error codes have correct values */
    TEST_ASSERT_EQUAL(0, OSA_OK);
    TEST_ASSERT_EQUAL(-1, OSA_ERR_GENERIC);
    TEST_ASSERT_EQUAL(-2, OSA_ERR_INVALID_PARAM);
    TEST_ASSERT_EQUAL(-3, OSA_ERR_NULL_POINTER);
    TEST_ASSERT_EQUAL(-100, OSA_ERR_NO_MEMORY);
    TEST_ASSERT_EQUAL(-500, OSA_ERR_WORKER_INIT);
    TEST_ASSERT_EQUAL(-600, OSA_ERR_HSM_INIT);
}

void test_error_string_conversion(void) {
    /* Test error code to string conversion */
    TEST_ASSERT_EQUAL_STRING("Success", osa_error_string(OSA_OK));
    TEST_ASSERT_EQUAL_STRING("Invalid parameter", osa_error_string(OSA_ERR_INVALID_PARAM));
    TEST_ASSERT_EQUAL_STRING("Out of memory", osa_error_string(OSA_ERR_NO_MEMORY));
    TEST_ASSERT_EQUAL_STRING("Worker pool init failed", osa_error_string(OSA_ERR_WORKER_INIT));
    TEST_ASSERT_EQUAL_STRING("HSM init failed", osa_error_string(OSA_ERR_HSM_INIT));
}

void test_error_category(void) {
    /* Test error category detection */
    TEST_ASSERT_EQUAL_STRING("Success", osa_error_category(OSA_OK));
    TEST_ASSERT_EQUAL_STRING("General", osa_error_category(OSA_ERR_GENERIC));
    TEST_ASSERT_EQUAL_STRING("Memory/Resource", osa_error_category(OSA_ERR_NO_MEMORY));
    TEST_ASSERT_EQUAL_STRING("Worker Pool", osa_error_category(OSA_ERR_WORKER_INIT));
    TEST_ASSERT_EQUAL_STRING("HSM", osa_error_category(OSA_ERR_HSM_INIT));
}

void test_error_helper_functions(void) {
    /* Test osa_is_ok() */
    TEST_ASSERT_TRUE(osa_is_ok(OSA_OK));
    TEST_ASSERT_FALSE(osa_is_ok(OSA_ERR_GENERIC));
    TEST_ASSERT_FALSE(osa_is_ok(OSA_ERR_NO_MEMORY));

    /* Test osa_is_err() */
    TEST_ASSERT_FALSE(osa_is_err(OSA_OK));
    TEST_ASSERT_TRUE(osa_is_err(OSA_ERR_GENERIC));
    TEST_ASSERT_TRUE(osa_is_err(OSA_ERR_NO_MEMORY));
}

void test_error_suite(void) {
    printf("\n--- Error Code Tests ---\n");
    RUN_TEST(test_error_code_values);
    RUN_TEST(test_error_string_conversion);
    RUN_TEST(test_error_category);
    RUN_TEST(test_error_helper_functions);
}
