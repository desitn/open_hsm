/*
 * ESP32 Error Code Implementation
 */

#include "osa_platform.h"
#include "osa_error.h"
#include "freertos/FreeRTOS.h"
#include <string.h>

/* ESP32 specific error strings */
static const char* esp32_error_strings[] = {
    "ESP32: Task creation failed",
    "ESP32: Out of memory",
    "ESP32: Invalid handle",
    "ESP32: Queue full",
    "ESP32: Semaphore timeout",
};

/* ESP32 specific error codes (base offset) */
#define OSA_ESP32_ERR_BASE  0x100

const char* osa_error_string(int err)
{
    /* First check standard OSA errors */
    const char* std_str = osa_error_string_std(err);
    if (std_str) {
        return std_str;
    }
    
    /* ESP32 specific errors */
    int esp32_err = err - OSA_ESP32_ERR_BASE;
    if (esp32_err >= 0 && esp32_err < sizeof(esp32_error_strings)/sizeof(esp32_error_strings[0])) {
        return esp32_error_strings[esp32_err];
    }
    
    return "ESP32: Unknown error";
}

const char* osa_error_category(int err)
{
    if (err >= OSA_ESP32_ERR_BASE && err < OSA_ESP32_ERR_BASE + 0x100) {
        return "ESP32";
    }
    return osa_error_category_std(err);
}

/* ESP32 specific helper functions */

int osa_error_from_esp_err(esp_err_t esp_err)
{
    switch (esp_err) {
        case ESP_OK:
            return OSA_OK;
        case ESP_ERR_NO_MEM:
            return OSA_ERR_NO_MEMORY;
        case ESP_ERR_INVALID_ARG:
            return OSA_ERR_INVALID_PARAM;
        case ESP_ERR_INVALID_STATE:
            return OSA_ERR_NOT_INITIALIZED;
        case ESP_ERR_INVALID_SIZE:
            return OSA_ERR_INVALID_PARAM;
        case ESP_ERR_NOT_FOUND:
            return OSA_ERR_GENERIC;
        case ESP_ERR_NOT_SUPPORTED:
            return OSA_ERR_GENERIC;
        case ESP_ERR_TIMEOUT:
            return OSA_ERR_TIMEOUT;
        case ESP_ERR_INVALID_RESPONSE:
            return OSA_ERR_GENERIC;
        case ESP_ERR_INVALID_CRC:
            return OSA_ERR_GENERIC;
        case ESP_ERR_INVALID_VERSION:
            return OSA_ERR_GENERIC;
        case ESP_ERR_INVALID_MAC:
            return OSA_ERR_GENERIC;
        default:
            return OSA_ERR_GENERIC;
    }
}

/* Standard OSA error strings (from osa_error.h) */
static const char* osa_std_error_strings[] = {
    "Success",
    "Generic error",
    "Invalid parameter",
    "Null pointer",
    "Not initialized",
    "No memory",
    "Timeout",
    "Mutex lock failed",
    "Thread creation failed",
    "Queue full",
    "Worker pool init failed",
    "Worker pool queue full",
    "HSM init failed",
    "HSM invalid state",
};

static const char* osa_std_error_categories[] = {
    "General",
    "General",
    "Parameter",
    "Memory",
    "State",
    "Memory",
    "Timeout",
    "Sync",
    "Thread",
    "Queue",
    "Worker",
    "Worker",
    "HSM",
    "HSM",
};

const char* osa_error_string_std(int err)
{
    if (err >= 0 && err < sizeof(osa_std_error_strings)/sizeof(osa_std_error_strings[0])) {
        return osa_std_error_strings[err];
    }
    return NULL;
}

const char* osa_error_category_std(int err)
{
    if (err >= 0 && err < sizeof(osa_std_error_categories)/sizeof(osa_std_error_categories[0])) {
        return osa_std_error_categories[err];
    }
    return "Unknown";
}

/* ESP32 memory stats (optional) */
void osa_print_memory_stats(void)
{
    printf("ESP32 Memory Stats:\n");
    printf("  Free heap: %d bytes\n", esp_get_free_heap_size());
    printf("  Minimum free heap: %d bytes\n", esp_get_minimum_free_heap_size());
    printf("  Free internal heap: %d bytes\n", esp_get_free_internal_heap_size());
}
