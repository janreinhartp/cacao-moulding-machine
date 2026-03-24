/**
 * @file i2c_manager.c
 * @brief Shared I2C bus manager implementation.
 */

#include "i2c_manager.h"
#include "board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "i2c_mgr";

static i2c_master_bus_handle_t s_bus_handle = NULL;
static SemaphoreHandle_t s_i2c_mutex = NULL;

esp_err_t i2c_manager_init(void)
{
    if (s_bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C bus already initialized");
        return ESP_OK;
    }

    s_i2c_mutex = xSemaphoreCreateMutex();
    if (s_i2c_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C mutex");
        return ESP_ERR_NO_MEM;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_PIN,
        .scl_io_num = BOARD_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &s_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C bus initialized (SDA=%d, SCL=%d, freq=%d Hz)",
             BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN, BOARD_I2C_FREQ_HZ);
    return ESP_OK;
}

i2c_master_bus_handle_t i2c_manager_get_bus(void)
{
    return s_bus_handle;
}

bool i2c_manager_lock(uint32_t timeout_ms)
{
    if (s_i2c_mutex == NULL) {
        return false;
    }
    return xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void i2c_manager_unlock(void)
{
    if (s_i2c_mutex != NULL) {
        xSemaphoreGive(s_i2c_mutex);
    }
}
