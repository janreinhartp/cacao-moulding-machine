/**
 * @file i2c_manager.h
 * @brief Shared I2C bus manager with mutex protection.
 */
#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the shared I2C bus.
 * @return ESP_OK on success.
 */
esp_err_t i2c_manager_init(void);

/**
 * @brief Get the I2C master bus handle.
 * @return Bus handle, or NULL if not initialized.
 */
i2c_master_bus_handle_t i2c_manager_get_bus(void);

/**
 * @brief Acquire the I2C bus mutex.
 * @param timeout_ms Timeout in milliseconds.
 * @return true if acquired.
 */
bool i2c_manager_lock(uint32_t timeout_ms);

/**
 * @brief Release the I2C bus mutex.
 */
void i2c_manager_unlock(void);

#ifdef __cplusplus
}
#endif
