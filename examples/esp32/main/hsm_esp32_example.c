/*
 * ESP32 HSM + Worker Pool Example
 * 
 * This example demonstrates the HSM framework running on ESP32
 * with FreeRTOS. It includes:
 * - Hierarchical State Machine
 * - Worker Pool for async tasks
 * - LED blinking state machine
 * - WiFi connection state machine (placeholder)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "osa_platform.h"
#include "osa_hsm.h"
#include "osa_worker.h"

#define TAG "HSM_ESP32"

/* LED GPIO (built-in LED on most ESP32 dev boards) */
#define LED_GPIO_PIN    GPIO_NUM_2

/* HSM Signals */
enum esp32_hsm_signals {
    HSM_SIG_BUTTON_PRESS = HSM_USER_SIG,
    HSM_SIG_WIFI_CONNECTED,
    HSM_SIG_WIFI_DISCONNECTED,
    HSM_SIG_WORK_COMPLETE,
    HSM_SIG_TOGGLE_LED,
};

/* LED State Machine States */
struct led_hsm {
    struct osa_hsm_active active;
    bool led_state;
};

static struct led_hsm g_led_hsm;
static struct osa_worker_pool g_worker_pool;

/* Forward declarations */
int led_off_state(void* entry, const struct hsm_event* event);
int led_on_state(void* entry, const struct hsm_event* event);
int led_blinking_state(void* entry, const struct hsm_event* event);

/* State definitions */
struct hsm_state led_off = {
    .handler = led_off_state,
    .parent = NULL,
    .name = "LED_OFF"
};

struct hsm_state led_on = {
    .handler = led_on_state,
    .parent = NULL,
    .name = "LED_ON"
};

struct hsm_state led_blinking = {
    .handler = led_blinking_state,
    .parent = NULL,
    .name = "LED_BLINKING"
};

/* LED OFF State Handler */
int led_off_state(void* entry, const struct hsm_event* event)
{
    STATE_ENTRY(entry);
    struct led_hsm* hsm = (struct led_hsm*)entry;

    switch (event->signal) {
        case HSM_SIG_ENTRY:
            ESP_LOGI(TAG, "Entering state: %s", STATE_NAME());
            gpio_set_level(LED_GPIO_PIN, 0);
            hsm->led_state = false;
            break;

        case HSM_SIG_EXIT:
            ESP_LOGI(TAG, "Exiting state: %s", STATE_NAME());
            break;

        case HSM_SIG_BUTTON_PRESS:
            ESP_LOGI(TAG, "Button pressed, turning LED ON");
            STATE_TRANSIT(&led_on);
            break;

        case HSM_SIG_TOGGLE_LED:
            ESP_LOGI(TAG, "Toggle LED to ON");
            STATE_TRANSIT(&led_on);
            break;

        default:
            STATE_SUPER(event);
            break;
    }
    return 0;
}

/* LED ON State Handler */
int led_on_state(void* entry, const struct hsm_event* event)
{
    STATE_ENTRY(entry);
    struct led_hsm* hsm = (struct led_hsm*)entry;

    switch (event->signal) {
        case HSM_SIG_ENTRY:
            ESP_LOGI(TAG, "Entering state: %s", STATE_NAME());
            gpio_set_level(LED_GPIO_PIN, 1);
            hsm->led_state = true;
            break;

        case HSM_SIG_EXIT:
            ESP_LOGI(TAG, "Exiting state: %s", STATE_NAME());
            break;

        case HSM_SIG_BUTTON_PRESS:
            ESP_LOGI(TAG, "Button pressed, starting blink mode");
            STATE_TRANSIT(&led_blinking);
            break;

        case HSM_SIG_TOGGLE_LED:
            ESP_LOGI(TAG, "Toggle LED to OFF");
            STATE_TRANSIT(&led_off);
            break;

        default:
            STATE_SUPER(event);
            break;
    }
    return 0;
}

/* LED Blinking State Handler */
int led_blinking_state(void* entry, const struct hsm_event* event)
{
    STATE_ENTRY(entry);
    struct led_hsm* hsm = (struct led_hsm*)entry;

    switch (event->signal) {
        case HSM_SIG_ENTRY:
            ESP_LOGI(TAG, "Entering state: %s", STATE_NAME());
            /* Start periodic timer for blinking */
            osa_hsm_active_period(&hsm->active, 500);  /* 500ms period */
            break;

        case HSM_SIG_EXIT:
            ESP_LOGI(TAG, "Exiting state: %s", STATE_NAME());
            osa_hsm_active_period(&hsm->active, 0);  /* Stop periodic */
            break;

        case HSM_SIG_PERIOD:
            /* Toggle LED */
            hsm->led_state = !hsm->led_state;
            gpio_set_level(LED_GPIO_PIN, hsm->led_state);
            ESP_LOGD(TAG, "LED toggled: %s", hsm->led_state ? "ON" : "OFF");
            break;

        case HSM_SIG_BUTTON_PRESS:
            ESP_LOGI(TAG, "Button pressed, turning LED OFF");
            STATE_TRANSIT(&led_off);
            break;

        default:
            STATE_SUPER(event);
            break;
    }
    return 0;
}

/* Simulated async work function */
int simulate_network_work(void *ctx)
{
    int work_id = (int)ctx;
    ESP_LOGI(TAG, "Starting async work %d...", work_id);
    
    /* Simulate network delay */
    osa_thread_sleep(2000);
    
    ESP_LOGI(TAG, "Async work %d completed!", work_id);
    return work_id * 10;  /* Return some result */
}

/* Work completion callback */
void on_work_complete(struct osa_worker_job *job, int result, void *ctx)
{
    ESP_LOGI(TAG, "Job %d completed with result %d (ctx: %p)", 
             job->id, result, ctx);
    
    /* Send event to HSM */
    struct hsm_event event = {
        .signal = HSM_SIG_WORK_COMPLETE,
        .param = result
    };
    osa_hsm_active_event_post(&g_led_hsm.active, &event, 0);
}

/* Button monitoring task */
static void button_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Button monitor task started");
    
    int press_count = 0;
    
    while (1) {
        /* Simulate button press every 5 seconds for demo */
        osa_thread_sleep(5000);
        
        press_count++;
        ESP_LOGI(TAG, "Simulated button press #%d", press_count);
        
        struct hsm_event event = {
            .signal = HSM_SIG_BUTTON_PRESS,
            .param = press_count
        };
        osa_hsm_active_event_post(&g_led_hsm.active, &event, 0);
        
        /* Every 3rd press, submit async work */
        if (press_count % 3 == 0) {
            ESP_LOGI(TAG, "Submitting async work to worker pool");
            
            struct osa_worker_job *job = osa_worker_job_alloc();
            if (job) {
                osa_worker_job_init(job, simulate_network_work, 
                                   (void*)press_count, 
                                   on_work_complete, NULL);
                osa_worker_pool_submit(&g_worker_pool, job);
            }
        }
    }
}

/* Initialize GPIO */
static void init_gpio(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    gpio_set_level(LED_GPIO_PIN, 0);
    ESP_LOGI(TAG, "GPIO initialized, LED on pin %d", LED_GPIO_PIN);
}

/* Main application */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "HSM ESP32 Example Starting...");
    ESP_LOGI(TAG, "=================================");
    
    /* Print memory stats */
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    
    /* Initialize GPIO */
    init_gpio();
    
    /* Initialize Worker Pool */
    ESP_LOGI(TAG, "Initializing Worker Pool...");
    int err = osa_worker_pool_init(&g_worker_pool, "esp32_pool", 2, 8);
    if (osa_is_err(err)) {
        ESP_LOGE(TAG, "Failed to init worker pool: %s", osa_error_string(err));
        return;
    }
    
    err = osa_worker_pool_start(&g_worker_pool);
    if (osa_is_err(err)) {
        ESP_LOGE(TAG, "Failed to start worker pool: %s", osa_error_string(err));
        return;
    }
    ESP_LOGI(TAG, "Worker Pool started with 2 workers");
    
    /* Initialize HSM */
    ESP_LOGI(TAG, "Initializing HSM...");
    err = osa_hsm_active_init(&g_led_hsm.active, &led_off);
    if (osa_is_err(err)) {
        ESP_LOGE(TAG, "Failed to init HSM: %s", osa_error_string(err));
        return;
    }
    
    g_led_hsm.active.name = "LED_HSM";
    g_led_hsm.led_state = false;
    
    /* Start HSM with smaller stack for ESP32 */
    err = osa_hsm_active_start(&g_led_hsm.active, 3072, 5, 10);
    if (osa_is_err(err)) {
        ESP_LOGE(TAG, "Failed to start HSM: %s", osa_error_string(err));
        return;
    }
    ESP_LOGI(TAG, "HSM started successfully");
    
    /* Create button monitoring task */
    struct osa_thread_param btn_param = {
        .name = "button_task",
        .stack_size = 2048,
        .priority = 3
    };
    osa_thread_t btn_thread = osa_thread_create(&btn_param, button_task, NULL);
    if (!btn_thread) {
        ESP_LOGE(TAG, "Failed to create button task");
        return;
    }
    
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "System initialized successfully!");
    ESP_LOGI(TAG, "LED will change state every 5 seconds");
    ESP_LOGI(TAG, "Every 3rd press triggers async work");
    ESP_LOGI(TAG, "=================================");
    
    /* Main task can now exit, FreeRTOS scheduler takes over */
    /* HSM and Worker Pool continue running in their tasks */
}
