/**
 * @file app_events.h
 * @brief Application-wide event definitions for inter-task communication.
 */
#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Event Types ─── */
typedef enum {
    EVT_BUTTON_PRESS,
    EVT_MACHINE_STATE_CHANGE,
    EVT_RELAY_ACTION,
    EVT_TIMER_EXPIRED,
    EVT_ERROR,
    EVT_EMERGENCY_STOP,
    EVT_SELF_TEST_RESULT,
} app_event_type_t;

/* ─── Button Identifiers ─── */
typedef enum {
    BTN_ID_PREV = 0,
    BTN_ID_ENTER,
    BTN_ID_NEXT,
    BTN_ID_COUNT,
} button_id_t;

/* ─── Machine States ─── */
typedef enum {
    MACHINE_STATE_IDLE = 0,
    MACHINE_STATE_MENU_NAVIGATION,
    MACHINE_STATE_SETTINGS,
    MACHINE_STATE_RUN_AUTO,
    MACHINE_STATE_TEST_MACHINE,
    MACHINE_STATE_ERROR,
    MACHINE_STATE_EMERGENCY_STOP,
    MACHINE_STATE_SELF_TEST,
} machine_state_t;

/* ─── Auto Sequence Steps ─── */
typedef enum {
    AUTO_STEP_IDLE = 0,
    AUTO_STEP_MIXER,
    AUTO_STEP_MOULD_A,
    AUTO_STEP_MOULD_B,
    AUTO_STEP_RELEASE,
    AUTO_STEP_HEATER,
    AUTO_STEP_COMPLETE,
} auto_step_t;

/* ─── Event Payload ─── */
typedef struct {
    app_event_type_t type;
    union {
        button_id_t     button;
        machine_state_t state;
        struct {
            uint8_t relay_index;
            bool    relay_on;
        } relay;
        struct {
            auto_step_t step;
            uint32_t    remaining_s;
        } timer;
        struct {
            int     code;
            char    message[32];
        } error;
        bool self_test_passed;
    } data;
} app_event_t;

/* ─── Global Event Queues ─── */
extern QueueHandle_t g_ui_event_queue;
extern QueueHandle_t g_machine_event_queue;

/**
 * @brief Initialize global event queues.
 * @return ESP_OK on success.
 */
esp_err_t app_events_init(void);

#ifdef __cplusplus
}
#endif
