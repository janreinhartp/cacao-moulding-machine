/**
 * @file button_input.c
 * @brief Debounced GPIO button input handler using FreeRTOS timer.
 */

#include "button_input.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "button";

typedef struct {
    gpio_num_t      gpio;
    int             id;
    int64_t         last_press_us;
} button_state_t;

static button_state_t s_buttons[] = {
    { .gpio = BTN_PREV_PIN,  .id = 0, .last_press_us = 0 },
    { .gpio = BTN_ENTER_PIN, .id = 1, .last_press_us = 0 },
    { .gpio = BTN_NEXT_PIN,  .id = 2, .last_press_us = 0 },
};

static button_callback_t s_callback = NULL;
static QueueHandle_t s_btn_queue = NULL;

#define NUM_BUTTONS (sizeof(s_buttons) / sizeof(s_buttons[0]))
#define DEBOUNCE_US (BUTTON_DEBOUNCE_MS * 1000)

/* Long-press auto-repeat (PREV / NEXT only, id != 1) */
#define LONG_PRESS_ENTER_ID     1       /* ENTER button index — no repeat */
#define LONG_PRESS_DELAY_MS     500     /* hold time before repeat starts */
#define LONG_PRESS_SLOW_MS      180     /* repeat interval: slow phase */
#define LONG_PRESS_FAST_MS      60      /* repeat interval: fast phase */
#define LONG_PRESS_ACCEL_STEPS  6       /* repeats before switching to fast */

static void IRAM_ATTR button_isr_handler(void *arg)
{
    int btn_id = (int)(intptr_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(s_btn_queue, &btn_id, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static const char *btn_names[] = { "PREV", "ENTER", "NEXT" };

static void button_task(void *pvParam)
{
    int btn_id;

    while (1) {
        if (xQueueReceive(s_btn_queue, &btn_id, portMAX_DELAY) == pdTRUE) {
            if (btn_id < 0 || btn_id >= (int)NUM_BUTTONS) {
                continue;
            }

            /* Debounce: wait for settle then verify pin is still LOW */
            vTaskDelay(pdMS_TO_TICKS(50));
            int level = gpio_get_level(s_buttons[btn_id].gpio);

            if (level == 0) {
                int64_t now = esp_timer_get_time();
                if ((now - s_buttons[btn_id].last_press_us) >= DEBOUNCE_US) {
                    s_buttons[btn_id].last_press_us = now;

                    ESP_LOGW(TAG, "[BTN] %s (GPIO%d) PRESSED",
                             btn_names[btn_id], s_buttons[btn_id].gpio);

                    if (s_callback != NULL) {
                        s_callback(btn_id);
                    }

                    /* Long-press repeat for PREV / NEXT only */
                    if (btn_id != LONG_PRESS_ENTER_ID) {
                        vTaskDelay(pdMS_TO_TICKS(LONG_PRESS_DELAY_MS));
                        int repeat_count = 0;
                        while (gpio_get_level(s_buttons[btn_id].gpio) == 0) {
                            if (s_callback != NULL) {
                                s_callback(btn_id);
                            }
                            repeat_count++;
                            uint32_t interval = (repeat_count < LONG_PRESS_ACCEL_STEPS)
                                                ? LONG_PRESS_SLOW_MS
                                                : LONG_PRESS_FAST_MS;
                            vTaskDelay(pdMS_TO_TICKS(interval));
                        }
                    }
                }
            }

            /* Drain any queued duplicates for this button */
            int queued_id;
            while (xQueueReceive(s_btn_queue, &queued_id, 0) == pdTRUE) {
                /* discard */
            }

            /* Additional debounce hold-off before accepting the next press */
            vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
        }
    }
}

esp_err_t button_input_init(button_callback_t callback)
{
    s_callback = callback;

    s_btn_queue = xQueueCreate(16, sizeof(int));
    if (s_btn_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret;

    for (int i = 0; i < (int)NUM_BUTTONS; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << s_buttons[i].gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };
        ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO%d: %s", s_buttons[i].gpio, esp_err_to_name(ret));
            return ret;
        }

        ret = gpio_isr_handler_add(s_buttons[i].gpio, button_isr_handler, (void *)(intptr_t)s_buttons[i].id);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add ISR handler for GPIO%d: %s", s_buttons[i].gpio, esp_err_to_name(ret));
            return ret;
        }
    }

    BaseType_t xRet = xTaskCreate(button_task, "btn_task", TASK_INPUT_STACK_SIZE, NULL, TASK_INPUT_PRIORITY, NULL);
    if (xRet != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Button input initialized (%d buttons)", (int)NUM_BUTTONS);
    return ESP_OK;
}
