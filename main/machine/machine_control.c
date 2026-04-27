/**
 * @file machine_control.c
 * @brief Machine control state machine and task implementation.
 *
 * Central state machine managing all machine operational states.
 * Communicates with UI task via event queues.
 */

#include "machine_control.h"
#include "auto_sequence.h"
#include "relay_control.h"
#include "pcf8575.h"
#include "nvs_settings.h"
#include "board_config.h"
#include "app_events.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "machine";

static machine_state_t s_state = MACHINE_STATE_SELF_TEST;
static TaskHandle_t s_machine_task_handle = NULL;
static bool s_confirm_notified = false;
static bool s_release_notified = false;

static const char *state_name(machine_state_t state)
{
    switch (state) {
        case MACHINE_STATE_IDLE:            return "IDLE";
        case MACHINE_STATE_MENU_NAVIGATION: return "MENU_NAV";
        case MACHINE_STATE_SETTINGS:        return "SETTINGS";
        case MACHINE_STATE_RUN_AUTO:        return "RUN_AUTO";
        case MACHINE_STATE_TEST_MACHINE:    return "TEST";
        case MACHINE_STATE_ERROR:           return "ERROR";
        case MACHINE_STATE_EMERGENCY_STOP:  return "E-STOP";
        case MACHINE_STATE_SELF_TEST:       return "SELF_TEST";
        default:                            return "UNKNOWN";
    }
}

machine_state_t machine_get_state(void)
{
    return s_state;
}

void machine_request_state(machine_state_t new_state)
{
    app_event_t event = {
        .type = EVT_MACHINE_STATE_CHANGE,
        .data.state = new_state,
    };
    xQueueSend(g_machine_event_queue, &event, pdMS_TO_TICKS(10));
}

void machine_emergency_stop(void)
{
    ESP_LOGE(TAG, "!!! EMERGENCY STOP !!!");
    relay_all_off();
    auto_sequence_stop();
    s_state = MACHINE_STATE_EMERGENCY_STOP;
}

bool machine_self_test(void)
{
    ESP_LOGI(TAG, "Running self-test...");

    /* Test 1: Verify PCF8575 communication */
    uint16_t read_val;
    esp_err_t ret = pcf8575_read(&read_val);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Self-test FAIL: PCF8575 communication error");
        return false;
    }
    ESP_LOGI(TAG, "  PCF8575 comms: OK (read 0x%04X)", read_val);

    /* Test 2: Verify all relays can be set to OFF state */
    ret = relay_all_off();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Self-test FAIL: Relay reset error");
        return false;
    }
    ESP_LOGI(TAG, "  Relay reset: OK");

    /* Test 3: Verify NVS settings are loaded */
    const machine_settings_t *settings = nvs_settings_get();
    if (settings->mixer_fill_s == 0) {
        ESP_LOGW(TAG, "Self-test WARN: Mixer fill time is 0");
    }
    ESP_LOGI(TAG, "  NVS settings: OK");

    ESP_LOGI(TAG, "Self-test PASSED");
    return true;
}

static void transition_state(machine_state_t new_state)
{
    if (new_state == s_state) return;

    ESP_LOGI(TAG, "State: %s -> %s", state_name(s_state), state_name(new_state));

    /* Exit actions for current state */
    switch (s_state) {
        case MACHINE_STATE_RUN_AUTO:
            auto_sequence_stop();
            relay_all_off();
            s_confirm_notified = false;
            s_release_notified = false;
            break;
        case MACHINE_STATE_TEST_MACHINE:
            relay_all_off();
            break;
        default:
            break;
    }

    /* Entry actions for new state */
    switch (new_state) {
        case MACHINE_STATE_RUN_AUTO:
            auto_sequence_start();
            break;
        case MACHINE_STATE_ERROR:
        case MACHINE_STATE_EMERGENCY_STOP:
            relay_all_off();
            break;
        default:
            break;
    }

    s_state = new_state;
}

static void machine_task(void *pvParam)
{
    app_event_t event;

    /* Perform self-test during startup */
    bool test_ok = machine_self_test();
    if (test_ok) {
        /* Notify UI of self-test result */
        app_event_t st_event = {
            .type = EVT_SELF_TEST_RESULT,
            .data.self_test_passed = true,
        };
        xQueueSend(g_ui_event_queue, &st_event, pdMS_TO_TICKS(10));
        transition_state(MACHINE_STATE_IDLE);
    } else {
        app_event_t err_event = {
            .type = EVT_ERROR,
            .data.error = { .code = -1 },
        };
        strncpy(err_event.data.error.message, "Self-test failed", sizeof(err_event.data.error.message) - 1);
        xQueueSend(g_ui_event_queue, &err_event, pdMS_TO_TICKS(10));
        transition_state(MACHINE_STATE_ERROR);
    }

    while (1) {
        /* Process incoming events with a short timeout for periodic work */
        BaseType_t got_event = xQueueReceive(g_machine_event_queue, &event, pdMS_TO_TICKS(100));

        if (got_event == pdTRUE) {
            switch (event.type) {
                case EVT_MACHINE_STATE_CHANGE:
                    transition_state(event.data.state);
                    break;

                case EVT_EMERGENCY_STOP:
                    machine_emergency_stop();
                    break;

                case EVT_MOULD_CONFIRM:
                    if (s_state == MACHINE_STATE_RUN_AUTO) {
                        auto_sequence_moulding_repeat();
                        s_confirm_notified = false;
                        s_release_notified = false;
                    }
                    break;
                case EVT_RELEASE_DONE:
                    if (s_state == MACHINE_STATE_RUN_AUTO) {
                        auto_sequence_release_done();
                        s_release_notified = false;
                    }
                    break;
                default:
                    break;
            }
        }

        /* Periodic processing based on current state */
        switch (s_state) {
            case MACHINE_STATE_RUN_AUTO: {
                bool still_running = auto_sequence_tick();
                /* Notify UI once when Release is active and waiting for operator */
                if (auto_sequence_is_release_waiting() && !s_release_notified) {
                    app_event_t release_evt = { .type = EVT_RELEASE_DONE_NEEDED };
                    xQueueSend(g_ui_event_queue, &release_evt, pdMS_TO_TICKS(10));
                    s_release_notified = true;
                }
                /* Notify UI once when waiting for operator confirmation */
                if (auto_sequence_is_waiting_confirm() && !s_confirm_notified) {
                    app_event_t confirm_evt = { .type = EVT_MOULD_CONFIRM_NEEDED };
                    xQueueSend(g_ui_event_queue, &confirm_evt, pdMS_TO_TICKS(10));
                    s_confirm_notified = true;
                }
                if (!still_running) {
                    ESP_LOGI(TAG, "Auto sequence finished");
                }
                break;
            }

            case MACHINE_STATE_EMERGENCY_STOP:
                /* Continuously ensure relays are off */
                relay_all_off();
                break;

            default:
                break;
        }
    }
}

esp_err_t machine_control_init(void)
{
    BaseType_t ret = xTaskCreate(
        machine_task,
        "machine_task",
        TASK_MACHINE_STACK_SIZE,
        NULL,
        TASK_MACHINE_PRIORITY,
        &s_machine_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create machine task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Machine control initialized");
    return ESP_OK;
}
