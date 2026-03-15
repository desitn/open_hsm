/**
 * @file error_code_demo.c
 * @brief Error Code System Demo
 *
 * This example demonstrates the unified error code system:
 * - Using OSA error codes instead of raw integers
 * - Converting error codes to human-readable strings
 * - Checking error status with helper functions
 */

#include "osa_worker.h"
#include "../../platform/osa_error.h"
#include <stdio.h>

#define UNUSED(x)  (void)(x)

/**
 * @brief Demo function that may return various errors
 */
static int demo_operation(int scenario)
{
    switch (scenario) {
        case 0:
            return OSA_OK;
        case 1:
            return OSA_ERR_INVALID_PARAM;
        case 2:
            return OSA_ERR_NO_MEMORY;
        case 3:
            return OSA_ERR_TIMEOUT;
        case 4:
            return OSA_ERR_WORKER_QUEUE_FULL;
        case 5:
            return OSA_ERR_HSM_INVALID_STATE;
        default:
            return OSA_ERR_GENERIC;
    }
}

/**
 * @brief Print error information
 */
static void print_error_info(int err)
{
    printf("  Error Code: %d\n", err);
    printf("  Error String: %s\n", osa_error_string(err));
    printf("  Category: %s\n", osa_error_category(err));
    printf("  Is OK: %s\n", osa_is_ok(err) ? "yes" : "no");
    printf("  Is Error: %s\n", osa_is_err(err) ? "yes" : "no");
    printf("\n");
}

int main(int argc, char *argv[])
{
    int err;
    int i;

    UNUSED(argc);
    UNUSED(argv);

    printf("=== OSA Error Code System Demo ===\n\n");

    /* Demo 1: Show all error categories */
    printf("1. Error Categories:\n");
    printf("   OSA_OK (0): %s\n", osa_error_category(OSA_OK));
    printf("   General errors: %s\n", osa_error_category(OSA_ERR_GENERIC));
    printf("   Memory errors: %s\n", osa_error_category(OSA_ERR_NO_MEMORY));
    printf("   Sync errors: %s\n", osa_error_category(OSA_ERR_MUTEX_LOCK));
    printf("   Thread errors: %s\n", osa_error_category(OSA_ERR_THREAD_CREATE));
    printf("   Queue errors: %s\n", osa_error_category(OSA_ERR_QUEUE_FULL));
    printf("   Worker errors: %s\n", osa_error_category(OSA_ERR_WORKER_INIT));
    printf("   HSM errors: %s\n", osa_error_category(OSA_ERR_HSM_INIT));
    printf("\n");

    /* Demo 2: Test various error scenarios */
    printf("2. Error Code Details:\n\n");

    for (i = 0; i <= 5; i++) {
        printf("Scenario %d:\n", i);
        err = demo_operation(i);
        print_error_info(err);
    }

    /* Demo 3: Real-world usage pattern */
    printf("3. Real-world Usage Pattern:\n\n");

    struct osa_worker_pool pool;

    printf("Initializing worker pool...\n");
    err = osa_worker_pool_init(&pool, "demo_pool", 2, 16);

    if (osa_is_err(err)) {
        printf("FAILED: %s (category: %s)\n",
               osa_error_string(err),
               osa_error_category(err));
        return -1;
    }

    printf("SUCCESS: Worker pool initialized\n\n");

    printf("Starting worker pool...\n");
    err = osa_worker_pool_start(&pool);

    if (osa_is_err(err)) {
        printf("FAILED: %s\n", osa_error_string(err));
        osa_worker_pool_stop(&pool);
        return -1;
    }

    printf("SUCCESS: Worker pool started\n\n");

    /* Cleanup */
    osa_worker_pool_stop(&pool);

    printf("4. All Error Codes:\n\n");

    /* Demo 4: Show all defined error codes */
    int errors[] = {
        OSA_OK,
        OSA_ERR_GENERIC,
        OSA_ERR_INVALID_PARAM,
        OSA_ERR_NULL_POINTER,
        OSA_ERR_NOT_INITIALIZED,
        OSA_ERR_NO_MEMORY,
        OSA_ERR_TIMEOUT,
        OSA_ERR_MUTEX_LOCK,
        OSA_ERR_THREAD_CREATE,
        OSA_ERR_QUEUE_FULL,
        OSA_ERR_WORKER_INIT,
        OSA_ERR_WORKER_QUEUE_FULL,
        OSA_ERR_HSM_INIT,
        OSA_ERR_HSM_INVALID_STATE,
    };

    int num_errors = sizeof(errors) / sizeof(errors[0]);

    printf("%-30s %-6s %-20s\n", "Error Name", "Code", "Description");
    printf("%-30s %-6s %-20s\n", "----------", "----", "-----------");

    for (i = 0; i < num_errors; i++) {
        const char *name;
        switch (errors[i]) {
            case OSA_OK: name = "OSA_OK"; break;
            case OSA_ERR_GENERIC: name = "OSA_ERR_GENERIC"; break;
            case OSA_ERR_INVALID_PARAM: name = "OSA_ERR_INVALID_PARAM"; break;
            case OSA_ERR_NULL_POINTER: name = "OSA_ERR_NULL_POINTER"; break;
            case OSA_ERR_NOT_INITIALIZED: name = "OSA_ERR_NOT_INITIALIZED"; break;
            case OSA_ERR_NO_MEMORY: name = "OSA_ERR_NO_MEMORY"; break;
            case OSA_ERR_TIMEOUT: name = "OSA_ERR_TIMEOUT"; break;
            case OSA_ERR_MUTEX_LOCK: name = "OSA_ERR_MUTEX_LOCK"; break;
            case OSA_ERR_THREAD_CREATE: name = "OSA_ERR_THREAD_CREATE"; break;
            case OSA_ERR_QUEUE_FULL: name = "OSA_ERR_QUEUE_FULL"; break;
            case OSA_ERR_WORKER_INIT: name = "OSA_ERR_WORKER_INIT"; break;
            case OSA_ERR_WORKER_QUEUE_FULL: name = "OSA_ERR_WORKER_QUEUE_FULL"; break;
            case OSA_ERR_HSM_INIT: name = "OSA_ERR_HSM_INIT"; break;
            case OSA_ERR_HSM_INVALID_STATE: name = "OSA_ERR_HSM_INVALID_STATE"; break;
            default: name = "UNKNOWN"; break;
        }
        printf("%-30s %-6d %-20s\n", name, errors[i], osa_error_string(errors[i]));
    }

    printf("\n=== Demo Complete ===\n");

    return 0;
}
