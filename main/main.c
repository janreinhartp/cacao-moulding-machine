/**
 * @file main.c
 * @brief Cacao Moulding Machine - Application Entry Point
 *
 * Initializes all subsystems in correct order and starts FreeRTOS tasks.
 * System architecture:
 *   - Input Task:   Handles debounced button events, sends to UI queue
 *   - UI Task:      Manages OLED display, menu navigation, screen rendering
 *   - Machine Task: State machine, auto-sequence execution, relay control
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "board_config.h"
#include "app_events.h"

/* Peripheral drivers */
#include "i2c_manager.h"
#include "pcf8575.h"
#include "relay_control.h"
#include "button_input.h"
#include "oled_display.h"
#include "nvs_settings.h"

/* Application modules */
#include "ui_manager.h"
#include "machine_control.h"

static const char *TAG = "app_main";

/* ─── Global Event Queues ─── */
QueueHandle_t g_ui_event_queue = NULL;
QueueHandle_t g_machine_event_queue = NULL;

esp_err_t app_events_init(void)
{
    g_ui_event_queue = xQueueCreate(16, sizeof(app_event_t));
    g_machine_event_queue = xQueueCreate(16, sizeof(app_event_t));

    if (g_ui_event_queue == NULL || g_machine_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queues");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Event queues created");
    return ESP_OK;
}

/**
 * @brief Button press callback – routes events to both UI and machine queues.
 */
static void on_button_press(int button_id)
{
    /* button_id == 3 (NUM_BUTTONS) is the E-stop sentinel from GPIO-level check */
    if (button_id == (int)BTN_ID_COUNT) {
        app_event_t evt = { .type = EVT_EMERGENCY_STOP };
        xQueueSend(g_machine_event_queue, &evt, pdMS_TO_TICKS(10));
        xQueueSend(g_ui_event_queue,      &evt, pdMS_TO_TICKS(10));
        return;
    }

    app_event_t event = {
        .type = EVT_BUTTON_PRESS,
        .data.button = (button_id_t)button_id,
    };

    /* Send to UI queue for navigation handling */
    xQueueSend(g_ui_event_queue, &event, pdMS_TO_TICKS(10));

    ESP_LOGD(TAG, "Button event -> UI queue (btn=%d)", button_id);
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Cacao Moulding Machine v1.0");
    ESP_LOGI(TAG, "  MCU: ESP32-S3");
    ESP_LOGI(TAG, "========================================");

    esp_err_t ret;

    /* 1. Initialize event queues (must be first) */
    ret = app_events_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Event queue init failed! Halting.");
        return;
    }

    /* 2. Initialize NVS settings */
    ret = nvs_settings_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS settings init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 3. Install GPIO ISR service (once, before any driver that uses GPIO interrupts) */
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 4. Initialize I2C bus (shared by PCF8575 and OLED) */
    ret = i2c_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 4. Initialize PCF8575 I/O expander */
    pcf8575_config_t pcf_config = {
        .i2c_addr = PCF8575_I2C_ADDR,
        .int_pin = PCF8575_INT_PIN,
    };
    ret = pcf8575_init(&pcf_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "PCF8575 init failed: %s (continuing without I/O expander)", esp_err_to_name(ret));
    }

    /* 5. Initialize relay control (depends on PCF8575) */
    ret = relay_control_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Relay control init failed: %s (continuing)", esp_err_to_name(ret));
    }

    /* 6. Initialize OLED display */
    ret = oled_display_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "OLED init failed: %s (continuing without display)", esp_err_to_name(ret));
    }

    /* 7. Initialize button input handler */
    ret = button_input_init(on_button_press);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 8. Start Machine Control Task */
    ret = machine_control_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Machine control init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 9. Start UI Manager Task */
    ret = ui_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UI manager init failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  System initialization complete");
    ESP_LOGI(TAG, "  All tasks running");
    ESP_LOGI(TAG, "========================================");

    /* Log loaded settings */
    const machine_settings_t *settings = nvs_settings_get();
    ESP_LOGI(TAG, "Settings: MixFill=%us, MouldAIn=%us, PressTime=%us, MouldAOut=%us, PressGap=%us, PreRelease=%us",
             settings->mixer_fill_s, settings->mould_a_in_s,
             settings->press_time_s, settings->mould_a_out_s,
             settings->press_gap_s, settings->pre_release_s);
}

