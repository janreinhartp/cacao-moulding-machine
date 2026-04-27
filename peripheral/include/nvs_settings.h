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
    uint16_t mixer_fill_s;    /* Mixer run time to fill Mould A */
    uint16_t mould_a_in_s;    /* Mould A positioning time */
    uint16_t press_time_s;    /* Mould B press duration (per press) */
    uint16_t mould_a_out_s;   /* Mould A return time */
    uint16_t press_gap_s;     /* Settle between two presses */
    uint16_t pre_release_s;   /* Settle before release fires */
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
 * @param index Setting index (0=mixer_fill, 1=mould_a_in, 2=press_time, 3=mould_a_out, 4=press_gap, 5=pre_release).
 * @param value New value in seconds.
 * @return ESP_OK on success.
 */
esp_err_t nvs_settings_update(uint8_t index, uint16_t value);

/**
 * @brief Update a single setting by index and persist to NVS immediately.
 * @param index Setting index (0=mixer_fill, 1=mould_a_in, 2=press_time, 3=mould_a_out, 4=press_gap, 5=pre_release).
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
#define SETTINGS_COUNT 6

/** Names for settings display (max 10 chars for OLED). */
#define SETTINGS_NAMES { \
    "Mixer Fill",  \
    "Mould A In",  \
    "Press Time",  \
    "MouldA Out",  \
    "Press Gap",   \
    "PreRelease"   \
}

#ifdef __cplusplus
}
#endif
