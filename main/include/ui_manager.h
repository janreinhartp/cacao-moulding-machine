/**
 * @file ui_manager.h
 * @brief UI task and screen management for OLED menu system.
 */
#pragma once

#include "esp_err.h"
#include "app_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/** UI screen identifiers. */
typedef enum {
    UI_SCREEN_SPLASH = 0,
    UI_SCREEN_MAIN_MENU,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_SETTINGS_EDIT,
    UI_SCREEN_SETTINGS_SAVED,
    UI_SCREEN_RUN_AUTO,
    UI_SCREEN_TEST_MACHINE,
    UI_SCREEN_EMERGENCY_STOP,
    UI_SCREEN_SELF_TEST,
    UI_SCREEN_ERROR,
} ui_screen_t;

/**
 * @brief Initialize the UI system and start the UI task.
 * @return ESP_OK on success.
 */
esp_err_t ui_manager_init(void);

/**
 * @brief Request a screen transition.
 * @param screen Target screen.
 */
void ui_set_screen(ui_screen_t screen);

/**
 * @brief Get the currently active screen.
 */
ui_screen_t ui_get_screen(void);

/**
 * @brief Update the auto-run display with current step info.
 * @param step_name Name of current process step.
 * @param remaining_s Remaining seconds.
 * @param total_s Total seconds for this step.
 */
void ui_update_auto_run(const char *step_name, uint32_t remaining_s, uint32_t total_s);

#ifdef __cplusplus
}
#endif
