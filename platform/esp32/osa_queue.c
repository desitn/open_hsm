/*
 * ESP32 FreeRTOS Queue Implementation
 */

#include "osa_platform.h"
#include "osa_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>

/* Static allocation for queues */
#define OSA_MAX_QUEUES 8

static struct {
    QueueHandle_t handle;
    StaticQueue_t buffer;
    uint8_t storage_buffer[OSA_QUEUE_MAX_SIZE * OSA_QUEUE_MAX_MSG_SIZE];
    bool used;
    size_t msg_size;
} queue_pool[OSA_MAX_QUEUES];

static int queue_pool_init = 0;

static void queue_pool_init_once(void)
{
    if (!queue_pool_init) {
        memset(queue_pool, 0, sizeof(queue_pool));
        queue_pool_init = 1;
    }
}

static int find_free_queue_slot(void)
{
    for (int i = 0; i < OSA_MAX_QUEUES; i++) {
        if (!queue_pool[i].used) {
            return i;
        }
    }
    return -1;
}

osa_queue_t osa_queue_create(const char *name, size_t msg_size, int max_msgs)
{
    queue_pool_init_once();
    
    if (msg_size == 0 || max_msgs == 0) {
        return NULL;
    }
    
    /* Limit to static buffer size */
    if (msg_size > OSA_QUEUE_MAX_MSG_SIZE) {
        msg_size = OSA_QUEUE_MAX_MSG_SIZE;
    }
    if (max_msgs > OSA_QUEUE_MAX_SIZE) {
        max_msgs = OSA_QUEUE_MAX_SIZE;
    }
    
    int slot = find_free_queue_slot();
    if (slot < 0) {
        return NULL;
    }
    
    QueueHandle_t handle = xQueueCreateStatic(
        max_msgs,
        msg_size,
        queue_pool[slot].storage_buffer,
        &queue_pool[slot].buffer
    );
    
    if (!handle) {
        return NULL;
    }
    
    queue_pool[slot].handle = handle;
    queue_pool[slot].used = true;
    queue_pool[slot].msg_size = msg_size;
    
    /* Note: FreeRTOS doesn't store name in queue, but we can use vQueueAddToRegistry */
    #if (configQUEUE_REGISTRY_SIZE > 0)
    if (name) {
        vQueueAddToRegistry(handle, name);
    }
    #endif
    
    return (osa_queue_t)handle;
}

int osa_queue_destroy(osa_queue_t queue)
{
    if (!queue) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    
    /* Find and mark slot as free */
    for (int i = 0; i < OSA_MAX_QUEUES; i++) {
        if (queue_pool[i].handle == handle && queue_pool[i].used) {
            queue_pool[i].used = false;
            queue_pool[i].handle = NULL;
            break;
        }
    }
    
    vQueueDelete(handle);
    return OSA_OK;
}

int osa_queue_send(osa_queue_t queue, const void *msg, size_t msg_size, int32_t timeout_ms)
{
    if (!queue || !msg) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    TickType_t ticks = (timeout_ms < 0) ? 
                       portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    BaseType_t result;
    
    /* Use FromISR version if in interrupt context */
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xQueueSendFromISR(handle, msg, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xQueueSend(handle, msg, ticks);
    }
    
    if (result != pdTRUE) {
        return (timeout_ms == 0) ? OSA_ERR_QUEUE_FULL : OSA_ERR_TIMEOUT;
    }
    
    return OSA_OK;
}

int osa_queue_receive(osa_queue_t queue, void *msg, size_t msg_size, int32_t timeout_ms)
{
    if (!queue || !msg) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    TickType_t ticks = (timeout_ms < 0) ? 
                       portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xQueueReceive(handle, msg, ticks) != pdTRUE) {
        return (timeout_ms == 0) ? OSA_ERR_TIMEOUT : OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_queue_peek(osa_queue_t queue, void *msg, size_t msg_size)
{
    if (!queue || !msg) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    
    if (xQueuePeek(handle, msg, 0) != pdTRUE) {
        return OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_queue_get_count(osa_queue_t queue, int *count)
{
    if (!queue || !count) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    
    *count = (int)uxQueueMessagesWaiting(handle);
    return OSA_OK;
}

int osa_queue_get_space(osa_queue_t queue, int *space)
{
    if (!queue || !space) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    
    *space = (int)uxQueueSpacesAvailable(handle);
    return OSA_OK;
}

int osa_queue_flush(osa_queue_t queue)
{
    if (!queue) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    QueueHandle_t handle = (QueueHandle_t)queue;
    
    xQueueReset(handle);
    return OSA_OK;
}
