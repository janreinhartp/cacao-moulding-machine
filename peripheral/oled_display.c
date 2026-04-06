/**
 * @file oled_display.c
 * @brief SSD1306 128x64 OLED display driver using the u8g2 library over I2C.
 */

#include "oled_display.h"
#include "i2c_manager.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

static const char *TAG = "oled";

static u8g2_t s_u8g2;
static i2c_master_dev_handle_t s_oled_dev = NULL;

/* Accumulated I2C write buffer for a single u8g2 transfer */
#define I2C_BUF_MAX 256
static uint8_t s_i2c_buf[I2C_BUF_MAX];
static uint16_t s_i2c_buf_len = 0;

/* ---------------------------------------------------------------------------
 * u8x8 byte-level I2C callback
 * --------------------------------------------------------------------------*/
static uint8_t u8x8_byte_esp_idf_i2c(u8x8_t *u8x8, uint8_t msg,
                                      uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_BYTE_INIT:
            break;

        case U8X8_MSG_BYTE_SET_DC:
            /* Not used by I2C transports */
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            s_i2c_buf_len = 0;
            break;

        case U8X8_MSG_BYTE_SEND: {
            const uint8_t *src = (const uint8_t *)arg_ptr;
            for (uint8_t i = 0; i < arg_int; i++) {
                if (s_i2c_buf_len < I2C_BUF_MAX) {
                    s_i2c_buf[s_i2c_buf_len++] = src[i];
                }
            }
            break;
        }

        case U8X8_MSG_BYTE_END_TRANSFER:
            if (s_oled_dev != NULL && s_i2c_buf_len > 0) {
                if (i2c_manager_lock(100)) {
                    i2c_master_transmit(s_oled_dev, s_i2c_buf, s_i2c_buf_len, 100);
                    i2c_manager_unlock();
                }
            }
            break;

        default:
            return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * u8x8 GPIO / delay callback
 * --------------------------------------------------------------------------*/
static uint8_t u8x8_gpio_delay_esp_idf(u8x8_t *u8x8, uint8_t msg,
                                        uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int ? arg_int : 1));
            break;
        case U8X8_MSG_DELAY_10MICRO:
            esp_rom_delay_us(10);
            break;
        case U8X8_MSG_DELAY_100NANO:
            esp_rom_delay_us(1);
            break;
        case U8X8_MSG_GPIO_RESET:
            /* No physical reset pin wired */
            break;
        default:
            return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/
esp_err_t oled_display_init(void)
{
    i2c_master_bus_handle_t bus = i2c_manager_get_bus();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length  = I2C_ADDR_BIT_LEN_7,
        .device_address   = OLED_I2C_ADDR,
        .scl_speed_hz     = BOARD_I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &s_oled_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add OLED I2C device: %s", esp_err_to_name(ret));
        return ret;
    }

    /* u8g2 setup: SSD1306 128x64, full-frame-buffer mode, hardware I2C */
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &s_u8g2,
        U8G2_R0,
        u8x8_byte_esp_idf_i2c,
        u8x8_gpio_delay_esp_idf
    );

    /* Inform u8g2 of the 7-bit I2C address (stored internally as addr << 1) */
    u8x8_SetI2CAddress(&s_u8g2.u8x8, OLED_I2C_ADDR << 1);

    u8g2_InitDisplay(&s_u8g2);
    u8g2_SetPowerSave(&s_u8g2, 0); /* Wake display */
    u8g2_ClearBuffer(&s_u8g2);
    u8g2_SendBuffer(&s_u8g2);

    ESP_LOGI(TAG, "OLED display initialized via u8g2 (%dx%d)", OLED_WIDTH, OLED_HEIGHT);
    return ESP_OK;
}

u8g2_t *oled_get_u8g2(void)
{
    return &s_u8g2;
}