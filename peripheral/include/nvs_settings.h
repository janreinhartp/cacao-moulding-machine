/**
 * @file nvs_settings.h
 * @brief Non-volatile storage manager for persistent machine settings.
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Settings structure stored in NVS. */
typedef struct {
    uint16_t mixer_time_s;
    uint16_t mould_a_time_s;
    uint16_t mould_b_time_s;
    uint16_t release_time_s;
    uint16_t heater_time_s;
} machine_settings_t;

/**
 * @brief Initialize NVS and load settings (or apply defaults).
 * @return ESP_OK on success.
 */
esp_err_t nvs_settings_init(void);

/**
 * @brief Get a pointer to the current in-memory settings.
 * @return Pointer to settings (never NULL after init).
 */
const machine_settings_t *nvs_settings_get(void);

/**
 * @brief Update a single setting in memory only (no NVS write).
 * @param index Setting index (0=mixer, 1=mould_a, 2=mould_b, 3=release, 4=heater).
 * @param value New value in seconds.
 * @return ESP_OK on success.
 */
esp_err_t nvs_settings_update(uint8_t index, uint16_t value);

/**
 * @brief Update a single setting by index and persist to NVS immediately.
 * @param index Setting index (0=mixer, 1=mould_a, 2=mould_b, 3=release, 4=heater).
 * @param value New value in seconds.
 * @return ESP_OK on success.
 */
esp_err_t nvs_settings_set(uint8_t index, uint16_t value);

/**
 * @brief Persist the current in-memory settings to NVS.
 * @return ESP_OK on success.
 */
esp_err_t nvs_settings_save_all(void);

/**
 * @brief Reset all settings to factory defaults and persist.
 * @return ESP_OK on success.
 */
esp_err_t nvs_settings_reset_defaults(void);

/** Number of configurable timer settings. */
#define SETTINGS_COUNT 5

/** Names for settings display. */
#define SETTINGS_NAMES { \
    "Mixer Time",   \
    "Mould A Time", \
    "Mould B Time", \
    "Release Time", \
    "Heater Time"   \
}

#ifdef __cplusplus
}
#endif
