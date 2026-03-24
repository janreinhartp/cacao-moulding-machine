/**
 * @file button_input.h
 * @brief Debounced GPIO button input handler with event-based notification.
 */
#pragma once

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Callback type for button press events. */
typedef void (*button_callback_t)(int button_id);

/**
 * @brief Initialize the button input subsystem.
 * @param callback Function called on debounced button press (from task context).
 * @return ESP_OK on success.
 */
esp_err_t button_input_init(button_callback_t callback);

#ifdef __cplusplus
}
#endif
