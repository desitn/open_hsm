/*
 * ESP32 FreeRTOS Synchronization Primitives Implementation
 */

#include "osa_platform.h"
#include "osa_sync.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

/* Static allocation for mutexes */
#define OSA_MAX_MUTEXES 16

static struct {
    SemaphoreHandle_t handle;
    StaticSemaphore_t buffer;
    bool used;
} mutex_pool[OSA_MAX_MUTEXES];

static int mutex_pool_init = 0;

static void mutex_pool_init_once(void)
{
    if (!mutex_pool_init) {
        memset(mutex_pool, 0, sizeof(mutex_pool));
        mutex_pool_init = 1;
    }
}

static int find_free_mutex_slot(void)
{
    for (int i = 0; i < OSA_MAX_MUTEXES; i++) {
        if (!mutex_pool[i].used) {
            return i;
        }
    }
    return -1;
}

/* Mutex Implementation */
osa_mutex_t osa_mutex_create(void)
{
    mutex_pool_init_once();
    
    int slot = find_free_mutex_slot();
    if (slot < 0) {
        return NULL;
    }
    
    SemaphoreHandle_t handle = xSemaphoreCreateMutexStatic(&mutex_pool[slot].buffer);
    if (!handle) {
        return NULL;
    }
    
    mutex_pool[slot].handle = handle;
    mutex_pool[slot].used = true;
    
    return (osa_mutex_t)handle;
}

int osa_mutex_destroy(osa_mutex_t mutex)
{
    if (!mutex) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    SemaphoreHandle_t handle = (SemaphoreHandle_t)mutex;
    
    /* Find and mark slot as free */
    for (int i = 0; i < OSA_MAX_MUTEXES; i++) {
        if (mutex_pool[i].handle == handle && mutex_pool[i].used) {
            mutex_pool[i].used = false;
            mutex_pool[i].handle = NULL;
            break;
        }
    }
    
    vSemaphoreDelete(handle);
    return OSA_OK;
}

int osa_mutex_lock(osa_mutex_t mutex)
{
    if (!mutex) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    if (xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY) != pdTRUE) {
        return OSA_ERR_MUTEX_LOCK;
    }
    
    return OSA_OK;
}

int osa_mutex_unlock(osa_mutex_t mutex)
{
    if (!mutex) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    if (xSemaphoreGive((SemaphoreHandle_t)mutex) != pdTRUE) {
        return OSA_ERR_MUTEX_LOCK;
    }
    
    return OSA_OK;
}

int osa_mutex_trylock(osa_mutex_t mutex)
{
    if (!mutex) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    if (xSemaphoreTake((SemaphoreHandle_t)mutex, 0) != pdTRUE) {
        return OSA_ERR_TIMEOUT;
    }
    
    return OSA_OK;
}

int osa_mutex_timedlock(osa_mutex_t mutex, uint32_t timeout_ms)
{
    if (!mutex) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TickType_t ticks = (timeout_ms == OSA_WAIT_FOREVER) ? 
                       portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xSemaphoreTake((SemaphoreHandle_t)mutex, ticks) != pdTRUE) {
        return OSA_ERR_TIMEOUT;
    }
    
    return OSA_OK;
}

/* Static allocation for semaphores */
#define OSA_MAX_SEMAPHORES 16

static struct {
    SemaphoreHandle_t handle;
    StaticSemaphore_t buffer;
    bool used;
} sem_pool[OSA_MAX_SEMAPHORES];

static int sem_pool_init = 0;

static void sem_pool_init_once(void)
{
    if (!sem_pool_init) {
        memset(sem_pool, 0, sizeof(sem_pool));
        sem_pool_init = 1;
    }
}

static int find_free_sem_slot(void)
{
    for (int i = 0; i < OSA_MAX_SEMAPHORES; i++) {
        if (!sem_pool[i].used) {
            return i;
        }
    }
    return -1;
}

/* Semaphore Implementation */
osa_sem_t osa_sem_create(int init_count, int max_count)
{
    sem_pool_init_once();
    
    int slot = find_free_sem_slot();
    if (slot < 0) {
        return NULL;
    }
    
    SemaphoreHandle_t handle;
    
    if (max_count == 1) {
        /* Binary semaphore */
        handle = xSemaphoreCreateBinaryStatic(&sem_pool[slot].buffer);
        if (init_count > 0) {
            xSemaphoreGive(handle);  /* Initial signal */
        }
    } else {
        /* Counting semaphore */
        handle = xSemaphoreCreateCountingStatic(max_count, init_count, 
                                                &sem_pool[slot].buffer);
    }
    
    if (!handle) {
        return NULL;
    }
    
    sem_pool[slot].handle = handle;
    sem_pool[slot].used = true;
    
    return (osa_sem_t)handle;
}

int osa_sem_destroy(osa_sem_t sem)
{
    if (!sem) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    SemaphoreHandle_t handle = (SemaphoreHandle_t)sem;
    
    /* Find and mark slot as free */
    for (int i = 0; i < OSA_MAX_SEMAPHORES; i++) {
        if (sem_pool[i].handle == handle && sem_pool[i].used) {
            sem_pool[i].used = false;
            sem_pool[i].handle = NULL;
            break;
        }
    }
    
    vSemaphoreDelete(handle);
    return OSA_OK;
}

int osa_sem_wait(osa_sem_t sem, int32_t timeout_ms)
{
    if (!sem) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TickType_t ticks = (timeout_ms < 0) ? 
                       portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xSemaphoreTake((SemaphoreHandle_t)sem, ticks) != pdTRUE) {
        return (timeout_ms == 0) ? OSA_ERR_TIMEOUT : OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_sem_post(osa_sem_t sem)
{
    if (!sem) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    BaseType_t result;
    
    /* Use FromISR version if in interrupt context */
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xSemaphoreGiveFromISR((SemaphoreHandle_t)sem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xSemaphoreGive((SemaphoreHandle_t)sem);
    }
    
    if (result != pdTRUE) {
        return OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_sem_get_count(osa_sem_t sem, int *count)
{
    if (!sem || !count) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    *count = (int)uxSemaphoreGetCount((SemaphoreHandle_t)sem);
    return OSA_OK;
}
