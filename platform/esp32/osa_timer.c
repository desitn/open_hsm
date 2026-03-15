/*
 * ESP32 FreeRTOS Timer Implementation
 */

#include "osa_platform.h"
#include "osa_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>

/* Static allocation for timers */
#define OSA_MAX_TIMERS 8

static struct {
    TimerHandle_t handle;
    StaticTimer_t buffer;
    bool used;
    osa_timer_callback_t callback;
    void *arg;
} timer_pool[OSA_MAX_TIMERS];

static int timer_pool_init = 0;

static void timer_pool_init_once(void)
{
    if (!timer_pool_init) {
        memset(timer_pool, 0, sizeof(timer_pool));
        timer_pool_init = 1;
    }
}

static int find_free_timer_slot(void)
{
    for (int i = 0; i < OSA_MAX_TIMERS; i++) {
        if (!timer_pool[i].used) {
            return i;
        }
    }
    return -1;
}

/* FreeRTOS timer callback wrapper */
static void timer_callback_wrapper(TimerHandle_t xTimer)
{
    /* Find the timer slot */
    for (int i = 0; i < OSA_MAX_TIMERS; i++) {
        if (timer_pool[i].handle == xTimer && timer_pool[i].used) {
            if (timer_pool[i].callback) {
                timer_pool[i].callback(timer_pool[i].arg);
            }
            break;
        }
    }
}

osa_timer_t osa_timer_create(const char *name,
                             uint32_t period_ms,
                             bool periodic,
                             osa_timer_callback_t callback,
                             void *arg)
{
    timer_pool_init_once();
    
    if (period_ms == 0 || !callback) {
        return NULL;
    }
    
    int slot = find_free_timer_slot();
    if (slot < 0) {
        return NULL;
    }
    
    /* Convert period to ticks */
    TickType_t period_ticks = pdMS_TO_TICKS(period_ms);
    if (period_ticks == 0) {
        period_ticks = 1;  /* Minimum 1 tick */
    }
    
    TimerHandle_t handle = xTimerCreateStatic(
        name ? name : "osa_timer",
        period_ticks,
        periodic ? pdTRUE : pdFALSE,
        (void *)slot,  /* Timer ID used as slot index */
        timer_callback_wrapper,
        &timer_pool[slot].buffer
    );
    
    if (!handle) {
        return NULL;
    }
    
    timer_pool[slot].handle = handle;
    timer_pool[slot].used = true;
    timer_pool[slot].callback = callback;
    timer_pool[slot].arg = arg;
    
    return (osa_timer_t)handle;
}

int osa_timer_destroy(osa_timer_t timer)
{
    if (!timer) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TimerHandle_t handle = (TimerHandle_t)timer;
    
    /* Stop timer if running */
    xTimerStop(handle, portMAX_DELAY);
    
    /* Find and mark slot as free */
    for (int i = 0; i < OSA_MAX_TIMERS; i++) {
        if (timer_pool[i].handle == handle && timer_pool[i].used) {
            timer_pool[i].used = false;
            timer_pool[i].handle = NULL;
            timer_pool[i].callback = NULL;
            break;
        }
    }
    
    /* Note: FreeRTOS static timers cannot be truly deleted,
     * but we mark the slot as free for reuse */
    return OSA_OK;
}

int osa_timer_start(osa_timer_t timer)
{
    if (!timer) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TimerHandle_t handle = (TimerHandle_t)timer;
    
    BaseType_t result;
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerStartFromISR(handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerStart(handle, portMAX_DELAY);
    }
    
    if (result != pdPASS) {
        return OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_timer_stop(osa_timer_t timer)
{
    if (!timer) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TimerHandle_t handle = (TimerHandle_t)timer;
    
    BaseType_t result;
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xTimerStopFromISR(handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xTimerStop(handle, portMAX_DELAY);
    }
    
    if (result != pdPASS) {
        return OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_timer_restart(osa_timer_t timer, uint32_t period_ms)
{
    if (!timer) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TimerHandle_t handle = (TimerHandle_t)timer;
    
    TickType_t period_ticks = pdMS_TO_TICKS(period_ms);
    if (period_ticks == 0) {
        period_ticks = 1;
    }
    
    if (xTimerChangePeriod(handle, period_ticks, portMAX_DELAY) != pdPASS) {
        return OSA_ERR_GENERIC;
    }
    
    /* Restart the timer */
    if (xTimerReset(handle, portMAX_DELAY) != pdPASS) {
        return OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

int osa_timer_set_period(osa_timer_t timer, uint32_t period_ms)
{
    if (!timer) {
        return OSA_ERR_INVALID_PARAM;
    }
    
    TimerHandle_t handle = (TimerHandle_t)timer;
    
    TickType_t period_ticks = pdMS_TO_TICKS(period_ms);
    if (period_ticks == 0) {
        period_ticks = 1;
    }
    
    if (xTimerChangePeriod(handle, period_ticks, portMAX_DELAY) != pdPASS) {
        return OSA_ERR_GENERIC;
    }
    
    return OSA_OK;
}

bool osa_timer_is_active(osa_timer_t timer)
{
    if (!timer) {
        return false;
    }
    
    return xTimerIsTimerActive((TimerHandle_t)timer) == pdTRUE;
}

uint32_t osa_timer_get_period(osa_timer_t timer)
{
    if (!timer) {
        return 0;
    }
    
    TickType_t period_ticks = xTimerGetPeriod((TimerHandle_t)timer);
    return (uint32_t)(period_ticks * portTICK_PERIOD_MS);
}

uint32_t osa_timer_get_expiry(osa_timer_t timer)
{
    if (!timer) {
        return 0;
    }
    
    /* FreeRTOS doesn't provide direct API for remaining time */
    /* Return 0 as not supported */
    (void)timer;
    return 0;
}
