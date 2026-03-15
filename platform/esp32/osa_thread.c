/*
 * ESP32 FreeRTOS Thread Implementation
 */

#include "osa_platform.h"
#include "osa_thread.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* Static allocation buffers for tasks */
#define OSA_MAX_TASKS 16

static struct {
    TaskHandle_t handle;
    StaticTask_t task_buffer;
    StackType_t stack_buffer[OSA_THREAD_STACK_SIZE];
    bool used;
} task_pool[OSA_MAX_TASKS];

static int task_pool_init = 0;

static void task_pool_init_once(void)
{
    if (!task_pool_init) {
        memset(task_pool, 0, sizeof(task_pool));
        task_pool_init = 1;
    }
}

static int find_free_task_slot(void)
{
    for (int i = 0; i < OSA_MAX_TASKS; i++) {
        if (!task_pool[i].used) {
            return i;
        }
    }
    return -1;
}

/* Task wrapper to match OSA signature */
struct task_wrapper_arg {
    osa_thread_func_t func;
    void *arg;
};

static void task_wrapper(void *pvParameters)
{
    struct task_wrapper_arg *wrapper = (struct task_wrapper_arg *)pvParameters;
    osa_thread_func_t func = wrapper->func;
    void *arg = wrapper->arg;
    
    /* Free the wrapper memory */
    free(wrapper);
    
    /* Call the actual thread function */
    func(arg);
    
    /* Task should not return, delete if it does */
    vTaskDelete(NULL);
}

osa_thread_t osa_thread_create(const struct osa_thread_param *param,
                               osa_thread_func_t func,
                               void *arg)
{
    task_pool_init_once();
    
    if (!param || !func) {
        return NULL;
    }
    
    int slot = find_free_task_slot();
    if (slot < 0) {
        return NULL;
    }
    
    /* Create wrapper argument */
    struct task_wrapper_arg *wrapper = malloc(sizeof(*wrapper));
    if (!wrapper) {
        return NULL;
    }
    wrapper->func = func;
    wrapper->arg = arg;
    
    /* Convert OSA priority (1-10) to FreeRTOS priority (1-24) */
    UBaseType_t freertos_priority = param->priority + 1;
    if (freertos_priority > configMAX_PRIORITIES - 1) {
        freertos_priority = configMAX_PRIORITIES - 1;
    }
    
    /* Use static allocation for better memory management */
    TaskHandle_t handle = xTaskCreateStatic(
        task_wrapper,
        param->name ? param->name : "osa_task",
        param->stack_size / sizeof(StackType_t),  /* Convert bytes to words */
        wrapper,
        freertos_priority,
        task_pool[slot].stack_buffer,
        &task_pool[slot].task_buffer
    );
    
    if (!handle) {
        free(wrapper);
        return NULL;
    }
    
    task_pool[slot].handle = handle;
    task_pool[slot].used = true;
    
    return (osa_thread_t)handle;
}

int osa_thread_destroy(osa_thread_t thread)
{
    if (!thread) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TaskHandle_t handle = (TaskHandle_t)thread;
    
    /* Find and mark slot as free */
    for (int i = 0; i < OSA_MAX_TASKS; i++) {
        if (task_pool[i].handle == handle && task_pool[i].used) {
            task_pool[i].used = false;
            task_pool[i].handle = NULL;
            break;
        }
    }
    
    vTaskDelete(handle);
    return OSA_OK;
}

void osa_thread_sleep(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void osa_thread_yield(void)
{
    taskYIELD();
}

osa_thread_t osa_thread_self(void)
{
    return (osa_thread_t)xTaskGetCurrentTaskHandle();
}

int osa_thread_set_name(osa_thread_t thread, const char *name)
{
    /* FreeRTOS doesn't support renaming tasks after creation */
    /* This is a no-op on ESP32 */
    (void)thread;
    (void)name;
    return OSA_OK;
}

const char* osa_thread_get_name(osa_thread_t thread)
{
    TaskHandle_t handle = thread ? (TaskHandle_t)thread : xTaskGetCurrentTaskHandle();
    return pcTaskGetName(handle);
}
