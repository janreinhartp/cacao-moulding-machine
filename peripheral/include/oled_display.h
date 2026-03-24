/**
 * @file oled_display.h
 * @brief SSD1306 OLED display driver for 128x64 I2C display.
 *        Provides basic text rendering for menu-driven UI.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the OLED display.
 * @return ESP_OK on success.
 */
esp_err_t oled_display_init(void);

/**
 * @brief Clear the entire display buffer.
 */
void oled_clear(void);

/**
 * @brief Flush the internal buffer to the display hardware.
 * @return ESP_OK on success.
 */
esp_err_t oled_flush(void);

/**
 * @brief Draw a string at the specified pixel position.
 * @param x Column position (0–127).
 * @param y Row position in pages (0–7, each page = 8 pixels).
 * @param str Null-terminated string.
 * @param inverted If true, render inverted (white-on-black highlight).
 */
void oled_draw_string(uint8_t x, uint8_t y, const char *str, bool inverted);

/**
 * @brief Draw a horizontal line.
 * @param y Row page (0–7).
 */
void oled_draw_hline(uint8_t y);

/**
 * @brief Show a centered title bar at the top.
 * @param title Text to display.
 */
void oled_draw_title(const char *title);

/**
 * @brief Draw a progress bar.
 * @param x X position.
 * @param y Page row.
 * @param width Width in pixels.
 * @param progress Progress 0–100.
 */
void oled_draw_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t progress);

#ifdef __cplusplus
}
#endif
