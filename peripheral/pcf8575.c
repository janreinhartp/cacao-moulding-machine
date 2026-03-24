/**
 * @file pcf8575.c
 * @brief PCF8575 I2C 16-bit I/O expander driver implementation.
 */

#include "pcf8575.h"
#include "i2c_manager.h"
#include "board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pcf8575";

static i2c_master_dev_handle_t s_dev_handle = NULL;
static uint16_t s_output_state = 0xFFFF;  /* All pins HIGH (relays OFF for active-low) */

static void IRAM_ATTR pcf8575_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TaskHandle_t task = (TaskHandle_t)arg;
    if (task != NULL) {
        vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

esp_err_t pcf8575_init(const pcf8575_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t bus = i2c_manager_get_bus();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->i2c_addr,
        .scl_speed_hz = BOARD_I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_config, &s_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add PCF8575 device: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Write initial state: all outputs HIGH (relays OFF) */
    s_output_state = 0xFFFF;
    ret = pcf8575_write(s_output_state);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed initial write: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure interrupt pin if specified */
    if (config->int_pin != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << config->int_pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };
        ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure INT pin: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "PCF8575 interrupt configured on GPIO%d", config->int_pin);
    }

    ESP_LOGI(TAG, "PCF8575 initialized at addr 0x%02X", config->i2c_addr);
    return ESP_OK;
}

esp_err_t pcf8575_write(uint16_t value)
{
    if (s_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[2] = {
        (uint8_t)(value & 0xFF),        /* Low byte (P0–P7) */
        (uint8_t)((value >> 8) & 0xFF), /* High byte (P10–P17) */
    };

    if (!i2c_manager_lock(100)) {
        ESP_LOGE(TAG, "Failed to acquire I2C lock");
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = i2c_master_transmit(s_dev_handle, data, sizeof(data), 100);

    i2c_manager_unlock();

    if (ret == ESP_OK) {
        s_output_state = value;
    } else {
        ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t pcf8575_read(uint16_t *value)
{
    if (s_dev_handle == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {0};

    if (!i2c_manager_lock(100)) {
        ESP_LOGE(TAG, "Failed to acquire I2C lock");
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = i2c_master_receive(s_dev_handle, data, sizeof(data), 100);

    i2c_manager_unlock();

    if (ret == ESP_OK) {
        *value = (uint16_t)(data[0] | (data[1] << 8));
    } else {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t pcf8575_set_pin(uint8_t pin, bool level)
{
    if (pin > 15) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t new_state = s_output_state;
    if (level) {
        new_state |= (1 << pin);
    } else {
        new_state &= ~(1 << pin);
    }

    return pcf8575_write(new_state);
}

uint16_t pcf8575_get_output_state(void)
{
    return s_output_state;
}
