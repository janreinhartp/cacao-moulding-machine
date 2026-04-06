/**
 * @file oled_display.h
 * @brief SSD1306 128x64 OLED display driver backed by u8g2.
 */
#pragma once

#include "esp_err.h"
#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the OLED display via u8g2.
 *
 * Must be called after i2c_manager_init().
 * @return ESP_OK on success.
 */
esp_err_t oled_display_init(void);

/**
 * @brief Return a pointer to the u8g2 instance for direct rendering.
 */
u8g2_t *oled_get_u8g2(void);

#ifdef __cplusplus
}
#endif
