/**
 * @file relay_control.h
 * @brief Relay control abstraction layer (LOW-level trigger logic).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize relay control subsystem.
 *        Turns all relays OFF initially (all pins HIGH for low-level trigger).
 * @return ESP_OK on success.
 */
esp_err_t relay_control_init(void);

/**
 * @brief Turn a relay ON (sets pin LOW for active-low trigger).
 * @param relay_index Relay index (0–7).
 * @return ESP_OK on success.
 */
esp_err_t relay_on(uint8_t relay_index);

/**
 * @brief Turn a relay OFF (sets pin HIGH for active-low trigger).
 * @param relay_index Relay index (0–7).
 * @return ESP_OK on success.
 */
esp_err_t relay_off(uint8_t relay_index);

/**
 * @brief Toggle a relay state.
 * @param relay_index Relay index (0–7).
 * @return ESP_OK on success.
 */
esp_err_t relay_toggle(uint8_t relay_index);

/**
 * @brief Turn all relays OFF (safe state).
 * @return ESP_OK on success.
 */
esp_err_t relay_all_off(void);

/**
 * @brief Check if a specific relay is currently ON.
 * @param relay_index Relay index (0–7).
 * @return true if the relay is ON.
 */
bool relay_is_on(uint8_t relay_index);

#ifdef __cplusplus
}
#endif
