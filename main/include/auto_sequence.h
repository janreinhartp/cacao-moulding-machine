/**
 * @file auto_sequence.h
 * @brief Automated production sequence engine.
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "app_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the automatic production sequence.
 * @return ESP_OK on success.
 */
esp_err_t auto_sequence_start(void);

/**
 * @brief Stop and reset the auto sequence (e.g., on emergency stop).
 */
void auto_sequence_stop(void);

/**
 * @brief Periodic tick – call from the machine control task.
 *        Handles step timing and transitions.
 * @return true if sequence is still running, false if complete or stopped.
 */
bool auto_sequence_tick(void);

/**
 * @brief Get the current auto sequence step.
 */
auto_step_t auto_sequence_get_step(void);

/**
 * @brief Get remaining time for current step in seconds.
 */
uint32_t auto_sequence_get_remaining_s(void);

/**
 * @brief Get total time for current step in seconds.
 */
uint32_t auto_sequence_get_total_s(void);

/**
 * @brief Get a human-readable name for the current step.
 */
const char *auto_sequence_get_step_name(void);

#ifdef __cplusplus
}
#endif
