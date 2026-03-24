/**
 * @file pcf8575.h
 * @brief PCF8575 I2C 16-bit I/O expander driver with interrupt support.
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration for PCF8575 driver.
 */
typedef struct {
    uint8_t     i2c_addr;       /**< I2C 7-bit address */
    gpio_num_t  int_pin;        /**< Interrupt GPIO pin, or GPIO_NUM_NC if unused */
} pcf8575_config_t;

/**
 * @brief Initialize the PCF8575 driver.
 * @param config Pointer to configuration.
 * @return ESP_OK on success.
 */
esp_err_t pcf8575_init(const pcf8575_config_t *config);

/**
 * @brief Write a 16-bit value to all PCF8575 outputs.
 * @param value 16-bit value (each bit = one pin).
 * @return ESP_OK on success.
 */
esp_err_t pcf8575_write(uint16_t value);

/**
 * @brief Read the current 16-bit state from PCF8575.
 * @param[out] value Pointer to store the read value.
 * @return ESP_OK on success.
 */
esp_err_t pcf8575_read(uint16_t *value);

/**
 * @brief Set a single pin HIGH or LOW.
 * @param pin Pin number (0–15).
 * @param level true = HIGH, false = LOW.
 * @return ESP_OK on success.
 */
esp_err_t pcf8575_set_pin(uint8_t pin, bool level);

/**
 * @brief Get the current cached output state.
 * @return Current 16-bit output state.
 */
uint16_t pcf8575_get_output_state(void);

#ifdef __cplusplus
}
#endif
