/**
 * @file relay_control.c
 * @brief Relay control abstraction for LOW-level trigger relays via PCF8575.
 *
 * LOW-level trigger logic:
 *   - Relay ON  = PCF8575 pin LOW  (clear bit)
 *   - Relay OFF = PCF8575 pin HIGH (set bit)
 */

#include "relay_control.h"
#include "pcf8575.h"
#include "board_config.h"
#include "esp_log.h"

static const char *TAG = "relay_ctrl";

/* Track relay states independently for fast queries */
static uint8_t s_relay_state = 0x00; /* Bit per relay: 1=ON, 0=OFF */

esp_err_t relay_control_init(void)
{
    s_relay_state = 0x00;
    esp_err_t ret = relay_all_off();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Relay control initialized - all relays OFF");
    }
    return ret;
}

esp_err_t relay_on(uint8_t relay_index)
{
    if (relay_index >= RELAY_COUNT) {
        ESP_LOGE(TAG, "Invalid relay index: %d", relay_index);
        return ESP_ERR_INVALID_ARG;
    }

    /* LOW-level trigger: set pin LOW to activate relay */
    esp_err_t ret = pcf8575_set_pin(relay_index, false);
    if (ret == ESP_OK) {
        s_relay_state |= (1 << relay_index);
        ESP_LOGI(TAG, "Relay %d ON", relay_index);
    }
    return ret;
}

esp_err_t relay_off(uint8_t relay_index)
{
    if (relay_index >= RELAY_COUNT) {
        ESP_LOGE(TAG, "Invalid relay index: %d", relay_index);
        return ESP_ERR_INVALID_ARG;
    }

    /* LOW-level trigger: set pin HIGH to deactivate relay */
    esp_err_t ret = pcf8575_set_pin(relay_index, true);
    if (ret == ESP_OK) {
        s_relay_state &= ~(1 << relay_index);
        ESP_LOGI(TAG, "Relay %d OFF", relay_index);
    }
    return ret;
}

esp_err_t relay_toggle(uint8_t relay_index)
{
    if (relay_index >= RELAY_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    if (relay_is_on(relay_index)) {
        return relay_off(relay_index);
    } else {
        return relay_on(relay_index);
    }
}

esp_err_t relay_all_off(void)
{
    /* Set all relay pins HIGH (P0–P7) while preserving upper byte */
    uint16_t current = pcf8575_get_output_state();
    uint16_t new_state = current | 0x00FF; /* Set lower 8 bits HIGH = all relays OFF */
    esp_err_t ret = pcf8575_write(new_state);
    if (ret == ESP_OK) {
        s_relay_state = 0x00;
        ESP_LOGI(TAG, "All relays OFF");
    }
    return ret;
}

bool relay_is_on(uint8_t relay_index)
{
    if (relay_index >= RELAY_COUNT) {
        return false;
    }
    return (s_relay_state & (1 << relay_index)) != 0;
}
