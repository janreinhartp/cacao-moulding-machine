/**
 * @file auto_sequence.c
 * @brief Automated production sequence engine for cacao moulding.
 *
 * Sequence: Mixer → Mould A → Mould B → Release → Heater → Complete
 * Uses non-blocking timing with state transitions.
 */

#include "auto_sequence.h"
#include "relay_control.h"
#include "nvs_settings.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "auto_seq";

static auto_step_t s_step = AUTO_STEP_IDLE;
static int64_t s_step_start_us = 0;
static uint32_t s_step_duration_s = 0;
static bool s_running = false;

/* Step configuration: maps step → relay index and settings index */
typedef struct {
    auto_step_t step;
    uint8_t     relay_index;
    uint8_t     setting_index; /* Index into machine_settings_t fields */
    const char  *name;
} step_config_t;

static const step_config_t s_steps[] = {
    { AUTO_STEP_MIXER,   RELAY_MIXER,   0, "Mixing"    },
    { AUTO_STEP_MOULD_A, RELAY_MOULD_A, 1, "Mould A"   },
    { AUTO_STEP_MOULD_B, RELAY_MOULD_B, 2, "Mould B"   },
    { AUTO_STEP_RELEASE, RELAY_RELEASE, 3, "Release"   },
    { AUTO_STEP_HEATER,  RELAY_HEATER,  4, "Heating"   },
};
#define STEP_COUNT (sizeof(s_steps) / sizeof(s_steps[0]))

static void enter_step(int step_index)
{
    if (step_index >= (int)STEP_COUNT) {
        /* Sequence complete */
        relay_all_off();
        s_step = AUTO_STEP_COMPLETE;
        s_running = false;
        ESP_LOGI(TAG, "Auto sequence COMPLETE");
        return;
    }

    const step_config_t *cfg = &s_steps[step_index];
    const machine_settings_t *settings = nvs_settings_get();
    const uint16_t *vals = &settings->mixer_time_s;

    /* Turn off previous relay (if any) */
    if (step_index > 0) {
        relay_off(s_steps[step_index - 1].relay_index);
    }

    s_step = cfg->step;
    s_step_duration_s = vals[cfg->setting_index];
    s_step_start_us = esp_timer_get_time();

    /* Activate this step's relay */
    relay_on(cfg->relay_index);

    ESP_LOGI(TAG, "Step: %s (%lu s)", cfg->name, (unsigned long)s_step_duration_s);
}

esp_err_t auto_sequence_start(void)
{
    relay_all_off();
    s_running = true;

    ESP_LOGI(TAG, "Auto sequence STARTED");
    enter_step(0);
    return ESP_OK;
}

void auto_sequence_stop(void)
{
    relay_all_off();
    s_step = AUTO_STEP_IDLE;
    s_step_duration_s = 0;
    s_running = false;
    ESP_LOGW(TAG, "Auto sequence STOPPED");
}

bool auto_sequence_tick(void)
{
    if (!s_running || s_step == AUTO_STEP_COMPLETE || s_step == AUTO_STEP_IDLE) {
        return false;
    }

    int64_t elapsed_us = esp_timer_get_time() - s_step_start_us;
    int64_t duration_us = (int64_t)s_step_duration_s * 1000000LL;

    if (elapsed_us >= duration_us) {
        /* Current step timed out → advance to next */
        int current_idx = -1;
        for (int i = 0; i < (int)STEP_COUNT; i++) {
            if (s_steps[i].step == s_step) {
                current_idx = i;
                break;
            }
        }
        enter_step(current_idx + 1);
    }

    return s_running;
}

auto_step_t auto_sequence_get_step(void)
{
    return s_step;
}

uint32_t auto_sequence_get_remaining_s(void)
{
    if (!s_running || s_step == AUTO_STEP_IDLE || s_step == AUTO_STEP_COMPLETE) {
        return 0;
    }

    int64_t elapsed_us = esp_timer_get_time() - s_step_start_us;
    int64_t duration_us = (int64_t)s_step_duration_s * 1000000LL;
    int64_t remaining_us = duration_us - elapsed_us;

    if (remaining_us < 0) remaining_us = 0;
    return (uint32_t)(remaining_us / 1000000LL);
}

uint32_t auto_sequence_get_total_s(void)
{
    return s_step_duration_s;
}

const char *auto_sequence_get_step_name(void)
{
    for (int i = 0; i < (int)STEP_COUNT; i++) {
        if (s_steps[i].step == s_step) {
            return s_steps[i].name;
        }
    }
    if (s_step == AUTO_STEP_COMPLETE) return "Complete";
    return "Idle";
}
