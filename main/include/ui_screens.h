/**
 * @file ui_screens.h
 * @brief Individual screen rendering functions for the UI system.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Render the splash/startup screen. */
void ui_screen_splash(void);

/** Render the main menu. cursor_pos = selected item (0-based). */
void ui_screen_main_menu(uint8_t cursor_pos);

/**
 * Render the settings list screen.
 * @param cursor_pos Currently highlighted setting.
 * @param values Array of current timer values.
 */
void ui_screen_settings(uint8_t cursor_pos, const uint16_t *values);

/**
 * Render the settings edit screen for a single value.
 * @param index Setting index.
 * @param value Current value (being edited).
 */
void ui_screen_settings_edit(uint8_t index, uint16_t value);

/** Render the save confirmation screen. */
void ui_screen_settings_saved(void);

/**
 * Render the auto-run screen.
 * @param step_name Name of current step (e.g., "Mixing").
 * @param remaining_s Time remaining.
 * @param total_s Total duration for this step.
 * @param cycle_count Number of completed cycles in this session.
 */
void ui_screen_run_auto(const char *step_name, uint32_t remaining_s, uint32_t total_s, uint16_t cycle_count);

/** Render auto-run complete screen. */
void ui_screen_run_complete(void);

/** Render the release wait screen (release ON, press ENTER when cacao removed). */
void ui_screen_release_wait(void);

/**
 * Render the mould confirmation screen (operator retrieves balls, press ENTER to repeat).
 * @param cycle_count Number of completed cycles in this session.
 */
void ui_screen_mould_confirm(uint16_t cycle_count);

/**
 * Render the test machine screen.
 * @param cursor_pos Currently highlighted relay.
 * @param relay_states Bitmask of relay ON states.
 */
void ui_screen_test_machine(uint8_t cursor_pos, uint8_t relay_states);

/** Render the emergency stop screen. */
void ui_screen_emergency_stop(void);

/** Render self-test screen. */
void ui_screen_self_test(const char *status_msg);

/** Render error screen. */
void ui_screen_error(const char *error_msg);

#ifdef __cplusplus
}
#endif
