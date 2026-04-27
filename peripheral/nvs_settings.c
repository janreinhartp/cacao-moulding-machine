/**
 * @file nvs_settings.c
 * @brief NVS persistent settings manager implementation.
 */

#include "nvs_settings.h"
#include "board_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "nvs_set";

#define NVS_NAMESPACE    "cacao_cfg"
#define NVS_KEY_BLOB     "settings_v2"

static machine_settings_t s_settings;
static bool s_initialized = false;

static void apply_defaults(void)
{
    s_settings.mixer_fill_s  = DEFAULT_MIXER_FILL_S;
    s_settings.mould_a_in_s  = DEFAULT_MOULD_A_IN_S;
    s_settings.press_time_s  = DEFAULT_PRESS_TIME_S;
    s_settings.mould_a_out_s = DEFAULT_MOULD_A_OUT_S;
    s_settings.press_gap_s   = DEFAULT_PRESS_GAP_S;
    s_settings.pre_release_s = DEFAULT_PRE_RELEASE_S;
}

static esp_err_t save_to_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(handle, NVS_KEY_BLOB, &s_settings, sizeof(s_settings));
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Settings saved to NVS");
    } else {
        ESP_LOGE(TAG, "NVS save failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_settings_init(void)
{
    /* Initialize NVS flash */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition issue, erasing...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) return ret;
        ret = nvs_flash_init();
        if (ret != ESP_OK) return ret;
    }

    /* Try loading from NVS */
    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret == ESP_OK) {
        size_t required_size = sizeof(s_settings);
        ret = nvs_get_blob(handle, NVS_KEY_BLOB, &s_settings, &required_size);
        nvs_close(handle);

        if (ret == ESP_OK && required_size == sizeof(s_settings)) {
            ESP_LOGI(TAG, "Settings loaded from NVS");
            s_initialized = true;
            return ESP_OK;
        }
    }

    /* No valid settings found: apply defaults and save */
    ESP_LOGI(TAG, "Applying default settings");
    apply_defaults();
    ret = save_to_nvs();
    s_initialized = true;
    return ret;
}

const machine_settings_t *nvs_settings_get(void)
{
    return &s_settings;
}

static esp_err_t update_field(uint8_t index, uint16_t value)
{
    if (!s_initialized || index >= SETTINGS_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t *fields[] = {
        &s_settings.mixer_fill_s,
        &s_settings.mould_a_in_s,
        &s_settings.press_time_s,
        &s_settings.mould_a_out_s,
        &s_settings.press_gap_s,
        &s_settings.pre_release_s,
    };

    *fields[index] = value;
    ESP_LOGI(TAG, "Setting[%d] = %u s (in-memory)", index, value);
    return ESP_OK;
}

esp_err_t nvs_settings_update(uint8_t index, uint16_t value)
{
    return update_field(index, value);
}

esp_err_t nvs_settings_set(uint8_t index, uint16_t value)
{
    esp_err_t ret = update_field(index, value);
    if (ret != ESP_OK) return ret;
    return save_to_nvs();
}

esp_err_t nvs_settings_save_all(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG, "Saving all settings to NVS");
    return save_to_nvs();
}

esp_err_t nvs_settings_reset_defaults(void)
{
    apply_defaults();
    ESP_LOGI(TAG, "Settings reset to defaults");
    return save_to_nvs();
}
