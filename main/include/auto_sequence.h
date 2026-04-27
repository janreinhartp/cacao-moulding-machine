/**
 * @file auto_sequence.h
 * @brief Automated production sequence engine.
 *
 * Moulding cycle (repeats on operator confirm):
 *   1. Mould A position  → mould_a_in_s
 *   2. Mixer fill        → mixer_fill_s
 *   3. Mould A return    → mould_a_out_s
 *   4. Mould B press x2  → press_time_s each, press_gap_s between
 *   5. Pre-release settle → pre_release_s
 *   6. Release ON        → wait for operator ENTER
 *   7. Release OFF       → wait for operator ENTER to repeat
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
 *        Turns all relays OFF.
 */
void auto_sequence_stop(void);

/**
 * @brief Periodic tick – call from the machine control task (~100 ms).
 *        Advances step timing. Returns false only when fully stopped.
 * @return true if sequence is still active (including waiting for confirm).
 */
bool auto_sequence_tick(void);

/**
 * @brief Called by machine task when operator confirms and wants to repeat.
 *        Restarts the full moulding cycle from step 1.
 */
void auto_sequence_moulding_repeat(void);

/**
 * @brief Returns true when Release relay is ON and waiting for operator ENTER.
 */
bool auto_sequence_is_release_waiting(void);

/**
 * @brief Called when operator confirms cacao has been removed from Release.
 *        Turns Release OFF and advances to MOULD_CONFIRM.
 */
void auto_sequence_release_done(void);

/**
 * @brief Returns true when the sequence is paused waiting for operator confirm.
 */
bool auto_sequence_is_waiting_confirm(void);

/**
 * @brief Get the current auto sequence step.
 */
auto_step_t auto_sequence_get_step(void);

/**
 * @brief Get remaining time for current step in seconds (0 if waiting/idle).
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
