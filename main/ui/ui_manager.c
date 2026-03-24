/**
 * @file ui_manager.c
 * @brief UI task managing screen transitions, input handling, and display refresh.
 */

#include "ui_manager.h"
#include "ui_screens.h"
#include "machine_control.h"
#include "auto_sequence.h"
#include "oled_display.h"
#include "relay_control.h"
#include "nvs_settings.h"
#include "board_config.h"
#include "app_events.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ui_mgr";

static ui_screen_t s_current_screen = UI_SCREEN_SPLASH;
static uint8_t s_cursor_pos = 0;
static uint16_t s_edit_value = 0;
static uint8_t s_edit_index = 0;
static char s_auto_step_name[16] = "";
static uint32_t s_auto_remaining = 0;
static uint32_t s_auto_total = 0;
static char s_error_msg[32] = "";

/* Track all-buttons-pressed state for emergency stop detection */
static bool s_btn_prev_pressed = false;
static bool s_btn_enter_pressed = false;
static bool s_btn_next_pressed = false;

void ui_set_screen(ui_screen_t screen)
{
    s_current_screen = screen;
    s_cursor_pos = 0;
    ESP_LOGI(TAG, "Screen -> %d", screen);
}

ui_screen_t ui_get_screen(void)
{
    return s_current_screen;
}

void ui_update_auto_run(const char *step_name, uint32_t remaining_s, uint32_t total_s)
{
    if (step_name) {
        strncpy(s_auto_step_name, step_name, sizeof(s_auto_step_name) - 1);
        s_auto_step_name[sizeof(s_auto_step_name) - 1] = '\0';
    }
    s_auto_remaining = remaining_s;
    s_auto_total = total_s;
}

/* Check if all three buttons were pressed simultaneously for e-stop */
static void check_emergency_stop(button_id_t btn)
{
    switch (btn) {
        case BTN_ID_PREV:  s_btn_prev_pressed = true;  break;
        case BTN_ID_ENTER: s_btn_enter_pressed = true;  break;
        case BTN_ID_NEXT:  s_btn_next_pressed = true;   break;
        default: break;
    }

    if (s_btn_prev_pressed && s_btn_enter_pressed && s_btn_next_pressed) {
        ESP_LOGW(TAG, "EMERGENCY STOP triggered by all buttons");
        machine_emergency_stop();
        s_current_screen = UI_SCREEN_EMERGENCY_STOP;
        s_btn_prev_pressed = false;
        s_btn_enter_pressed = false;
        s_btn_next_pressed = false;
    }
}

/* Clear emergency button tracking on timeout (called if not all pressed together) */
static void reset_emergency_tracking(void)
{
    s_btn_prev_pressed = false;
    s_btn_enter_pressed = false;
    s_btn_next_pressed = false;
}

static void handle_main_menu_input(button_id_t btn)
{
    switch (btn) {
        case BTN_ID_PREV:
            if (s_cursor_pos > 0) s_cursor_pos--;
            break;
        case BTN_ID_NEXT:
            if (s_cursor_pos < 2) s_cursor_pos++;
            break;
        case BTN_ID_ENTER:
            switch (s_cursor_pos) {
                case 0:
                    s_current_screen = UI_SCREEN_SETTINGS;
                    s_cursor_pos = 0;
                    machine_request_state(MACHINE_STATE_SETTINGS);
                    break;
                case 1:
                    s_current_screen = UI_SCREEN_RUN_AUTO;
                    machine_request_state(MACHINE_STATE_RUN_AUTO);
                    break;
                case 2:
                    s_current_screen = UI_SCREEN_TEST_MACHINE;
                    s_cursor_pos = 0;
                    machine_request_state(MACHINE_STATE_TEST_MACHINE);
                    break;
            }
            break;
        default:
            break;
    }
}

static void handle_settings_input(button_id_t btn)
{
    switch (btn) {
        case BTN_ID_PREV:
            if (s_cursor_pos > 0) s_cursor_pos--;
            break;
        case BTN_ID_NEXT:
            if (s_cursor_pos < SETTINGS_COUNT - 1) s_cursor_pos++;
            break;
        case BTN_ID_ENTER: {
            /* Enter edit mode for the selected setting */
            const machine_settings_t *settings = nvs_settings_get();
            const uint16_t *vals = &settings->mixer_time_s;
            s_edit_index = s_cursor_pos;
            s_edit_value = vals[s_edit_index];
            s_current_screen = UI_SCREEN_SETTINGS_EDIT;
            break;
        }
        default:
            break;
    }
}

static void handle_settings_edit_input(button_id_t btn)
{
    switch (btn) {
        case BTN_ID_PREV:
            if (s_edit_value > 1) s_edit_value--;
            break;
        case BTN_ID_NEXT:
            if (s_edit_value < 999) s_edit_value++;
            break;
        case BTN_ID_ENTER:
            /* Save and return to settings list */
            nvs_settings_set(s_edit_index, s_edit_value);
            s_current_screen = UI_SCREEN_SETTINGS;
            break;
        default:
            break;
    }
}

static void handle_run_auto_input(button_id_t btn)
{
    /* During auto run, check for emergency stop (all buttons) */
    check_emergency_stop(btn);
}

static void handle_test_machine_input(button_id_t btn)
{
    switch (btn) {
        case BTN_ID_PREV:
            if (s_cursor_pos > 0) {
                s_cursor_pos--;
            } else {
                /* Exit test mode: turn all relays off, return to menu */
                relay_all_off();
                s_current_screen = UI_SCREEN_MAIN_MENU;
                s_cursor_pos = 2;
                machine_request_state(MACHINE_STATE_MENU_NAVIGATION);
            }
            break;
        case BTN_ID_NEXT:
            if (s_cursor_pos < RELAY_COUNT - 1) s_cursor_pos++;
            break;
        case BTN_ID_ENTER:
            relay_toggle(s_cursor_pos);
            break;
        default:
            break;
    }
}

static void handle_emergency_stop_input(button_id_t btn)
{
    if (btn == BTN_ID_ENTER) {
        s_current_screen = UI_SCREEN_MAIN_MENU;
        s_cursor_pos = 0;
        machine_request_state(MACHINE_STATE_IDLE);
    }
}

static void handle_error_input(button_id_t btn)
{
    if (btn == BTN_ID_ENTER) {
        s_current_screen = UI_SCREEN_MAIN_MENU;
        s_cursor_pos = 0;
        machine_request_state(MACHINE_STATE_IDLE);
    }
}

static void process_button_event(button_id_t btn)
{
    /* Reset emergency tracking after a short period (we only track simultaneous) */
    static int64_t last_btn_time = 0;
    int64_t now = esp_timer_get_time();
    if ((now - last_btn_time) > 500000) { /* 500ms window */
        reset_emergency_tracking();
    }
    last_btn_time = now;

    switch (s_current_screen) {
        case UI_SCREEN_MAIN_MENU:       handle_main_menu_input(btn);        break;
        case UI_SCREEN_SETTINGS:        handle_settings_input(btn);         break;
        case UI_SCREEN_SETTINGS_EDIT:   handle_settings_edit_input(btn);    break;
        case UI_SCREEN_RUN_AUTO:        handle_run_auto_input(btn);         break;
        case UI_SCREEN_TEST_MACHINE:    handle_test_machine_input(btn);     break;
        case UI_SCREEN_EMERGENCY_STOP:  handle_emergency_stop_input(btn);   break;
        case UI_SCREEN_ERROR:           handle_error_input(btn);            break;
        default: break;
    }
}

static void render_current_screen(void)
{
    const machine_settings_t *settings = nvs_settings_get();

    switch (s_current_screen) {
        case UI_SCREEN_SPLASH:
            ui_screen_splash();
            break;

        case UI_SCREEN_MAIN_MENU:
            ui_screen_main_menu(s_cursor_pos);
            break;

        case UI_SCREEN_SETTINGS:
            ui_screen_settings(s_cursor_pos, &settings->mixer_time_s);
            break;

        case UI_SCREEN_SETTINGS_EDIT:
            ui_screen_settings_edit(s_edit_index, s_edit_value);
            break;

        case UI_SCREEN_RUN_AUTO: {
            auto_step_t step = auto_sequence_get_step();
            if (step == AUTO_STEP_COMPLETE) {
                ui_screen_run_complete();
            } else {
                ui_screen_run_auto(
                    auto_sequence_get_step_name(),
                    auto_sequence_get_remaining_s(),
                    auto_sequence_get_total_s()
                );
            }
            break;
        }

        case UI_SCREEN_TEST_MACHINE: {
            uint8_t relay_states = 0;
            for (int i = 0; i < RELAY_COUNT; i++) {
                if (relay_is_on((uint8_t)i)) relay_states |= (1 << i);
            }
            ui_screen_test_machine(s_cursor_pos, relay_states);
            break;
        }

        case UI_SCREEN_EMERGENCY_STOP:
            ui_screen_emergency_stop();
            break;

        case UI_SCREEN_SELF_TEST:
            ui_screen_self_test("Running self-test...");
            break;

        case UI_SCREEN_ERROR:
            ui_screen_error(s_error_msg);
            break;
    }
}

static void ui_task(void *pvParam)
{
    app_event_t event;
    TickType_t last_render = 0;

    /* Show splash screen for 2 seconds (non-blocking via tick check) */
    ui_screen_splash();
    TickType_t splash_start = xTaskGetTickCount();

    while (1) {
        /* Handle splash → main menu transition */
        if (s_current_screen == UI_SCREEN_SPLASH) {
            if ((xTaskGetTickCount() - splash_start) >= pdMS_TO_TICKS(2000)) {
                s_current_screen = UI_SCREEN_MAIN_MENU;
                s_cursor_pos = 0;
                machine_request_state(MACHINE_STATE_MENU_NAVIGATION);
            }
        }

        /* Process incoming UI events (non-blocking, short timeout) */
        if (xQueueReceive(g_ui_event_queue, &event, pdMS_TO_TICKS(UI_REFRESH_INTERVAL_MS)) == pdTRUE) {
            switch (event.type) {
                case EVT_BUTTON_PRESS:
                    process_button_event(event.data.button);
                    break;
                case EVT_MACHINE_STATE_CHANGE:
                    if (event.data.state == MACHINE_STATE_ERROR) {
                        s_current_screen = UI_SCREEN_ERROR;
                    }
                    break;
                case EVT_ERROR:
                    strncpy(s_error_msg, event.data.error.message, sizeof(s_error_msg) - 1);
                    s_error_msg[sizeof(s_error_msg) - 1] = '\0';
                    s_current_screen = UI_SCREEN_ERROR;
                    break;
                default:
                    break;
            }
        }

        /* Refresh display at a controlled rate */
        TickType_t now = xTaskGetTickCount();
        if ((now - last_render) >= pdMS_TO_TICKS(UI_REFRESH_INTERVAL_MS)) {
            render_current_screen();
            last_render = now;
        }
    }
}

esp_err_t ui_manager_init(void)
{
    BaseType_t ret = xTaskCreate(ui_task, "ui_task", TASK_UI_STACK_SIZE, NULL, TASK_UI_PRIORITY, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "UI manager initialized");
    return ESP_OK;
}
