/**
 * @file auto_sequence.c
 * @brief Automated production sequence engine for cacao moulding.
 *
 * Moulding cycle (repeats on operator confirm):
 *   1. Mould A ON   -> mould_a_in_s  (position under mixer)
 *   2. Mixer ON     -> mixer_fill_s  (fill the mould)
 *   3. Mixer + Mould A OFF -> mould_a_out_s  (return to neutral)
 *   4. Mould B ON   -> press_time_s  (first press)
 *   5. Mould B OFF  -> press_gap_s   (settle between presses)
 *   6. Mould B ON   -> press_time_s  (second press)
 *   7. Mould B OFF  -> pre_release_s (settle before release)
 *   8. Release ON   -> wait for operator ENTER
 *   9. Release OFF  -> wait for operator ENTER to repeat
 */

#include "auto_sequence.h"
#include "relay_control.h"
#include "nvs_settings.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "auto_seq";

static auto_step_t s_step           = AUTO_STEP_IDLE;
static int64_t     s_step_start_us  = 0;
static uint32_t    s_step_duration_s = 0;
static bool        s_running        = false;

/* ── Internal helpers ─────────────────────────────────────────────────── */

static void start_timer(uint32_t duration_s)
{
    s_step_duration_s = duration_s;
    s_step_start_us   = esp_timer_get_time();
}

static bool timer_elapsed(void)
{
    int64_t elapsed = esp_timer_get_time() - s_step_start_us;
    return elapsed >= (int64_t)s_step_duration_s * 1000000LL;
}

/* ── Step entry functions ─────────────────────────────────────────────── */

static void enter_mould_a_pos(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_on(RELAY_MOULD_A);
    s_step = AUTO_STEP_MOULD_A_POS;
    start_timer(cfg->mould_a_in_s);
    ESP_LOGI(TAG, "MOULD_A_POS: ON for %u s", cfg->mould_a_in_s);
}

static void enter_mixer_fill(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_on(RELAY_MIXER);
    s_step = AUTO_STEP_MIXER_FILL;
    start_timer(cfg->mixer_fill_s);
    ESP_LOGI(TAG, "MIXER_FILL: ON for %u s", cfg->mixer_fill_s);
}

static void enter_mould_a_return(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_off(RELAY_MIXER);
    relay_off(RELAY_MOULD_A);
    s_step = AUTO_STEP_MOULD_A_RETURN;
    start_timer(cfg->mould_a_out_s);
    ESP_LOGI(TAG, "MOULD_A_RETURN: %u s", cfg->mould_a_out_s);
}

static void enter_press_1(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_on(RELAY_MOULD_B);
    s_step = AUTO_STEP_PRESS_1;
    start_timer(cfg->press_time_s);
    ESP_LOGI(TAG, "PRESS_1: Mould B ON for %u s", cfg->press_time_s);
}

static void enter_press_settle(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_off(RELAY_MOULD_B);
    s_step = AUTO_STEP_PRESS_SETTLE;
    start_timer(cfg->press_gap_s);
    ESP_LOGI(TAG, "PRESS_SETTLE: %u s", cfg->press_gap_s);
}

static void enter_press_2(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_on(RELAY_MOULD_B);
    s_step = AUTO_STEP_PRESS_2;
    start_timer(cfg->press_time_s);
    ESP_LOGI(TAG, "PRESS_2: Mould B ON for %u s", cfg->press_time_s);
}

static void enter_pre_release(void)
{
    const machine_settings_t *cfg = nvs_settings_get();
    relay_off(RELAY_MOULD_B);
    s_step = AUTO_STEP_PRE_RELEASE;
    start_timer(cfg->pre_release_s);
    ESP_LOGI(TAG, "PRE_RELEASE: settling %u s", cfg->pre_release_s);
}

static void enter_release(void)
{
    relay_on(RELAY_RELEASE);
    s_step = AUTO_STEP_RELEASE;
    s_step_duration_s = 0;
    ESP_LOGI(TAG, "RELEASE: ON, waiting for operator ENTER");
}

static void enter_mould_confirm(void)
{
    relay_off(RELAY_RELEASE);
    s_step = AUTO_STEP_MOULD_CONFIRM;
    s_step_duration_s = 0;
    ESP_LOGI(TAG, "MOULD_CONFIRM: waiting for operator");
}

/* ── Public API ───────────────────────────────────────────────────────── */

esp_err_t auto_sequence_start(void)
{
    relay_all_off();
    s_running = true;
    ESP_LOGI(TAG, "Auto sequence STARTED");
    enter_mould_a_pos();
    return ESP_OK;
}

void auto_sequence_stop(void)
{
    relay_all_off();
    s_step            = AUTO_STEP_IDLE;
    s_step_duration_s = 0;
    s_running         = false;
    ESP_LOGW(TAG, "Auto sequence STOPPED");
}

void auto_sequence_moulding_repeat(void)
{
    relay_all_off();
    ESP_LOGI(TAG, "Moulding REPEAT - restarting from step 1");
    enter_mould_a_pos();
}

bool auto_sequence_is_release_waiting(void)
{
    return s_running && s_step == AUTO_STEP_RELEASE;
}

void auto_sequence_release_done(void)
{
    if (s_running && s_step == AUTO_STEP_RELEASE) {
        ESP_LOGI(TAG, "Release DONE by operator");
        enter_mould_confirm();
    }
}

bool auto_sequence_is_waiting_confirm(void)
{
    return s_running && s_step == AUTO_STEP_MOULD_CONFIRM;
}

bool auto_sequence_tick(void)
{
    if (!s_running) {
        return false;
    }

    /* Waiting states: RELEASE and MOULD_CONFIRM stay alive until external event */
    if (s_step == AUTO_STEP_RELEASE || s_step == AUTO_STEP_MOULD_CONFIRM) {
        return true;
    }

    if (!timer_elapsed()) {
        return true;
    }

    /* Advance to next step */
    switch (s_step) {
        case AUTO_STEP_MOULD_A_POS:    enter_mixer_fill();     break;
        case AUTO_STEP_MIXER_FILL:     enter_mould_a_return(); break;
        case AUTO_STEP_MOULD_A_RETURN: enter_press_1();        break;
        case AUTO_STEP_PRESS_1:        enter_press_settle();   break;
        case AUTO_STEP_PRESS_SETTLE:   enter_press_2();        break;
        case AUTO_STEP_PRESS_2:        enter_pre_release();    break;
        case AUTO_STEP_PRE_RELEASE:    enter_release();        break;
        default:                                               break;
    }

    return s_running;
}

auto_step_t auto_sequence_get_step(void)
{
    return s_step;
}

uint32_t auto_sequence_get_remaining_s(void)
{
    if (!s_running || s_step == AUTO_STEP_IDLE ||
        s_step == AUTO_STEP_RELEASE || s_step == AUTO_STEP_MOULD_CONFIRM) {
        return 0;
    }

    int64_t elapsed_us   = esp_timer_get_time() - s_step_start_us;
    int64_t duration_us  = (int64_t)s_step_duration_s * 1000000LL;
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
    switch (s_step) {
        case AUTO_STEP_MOULD_A_POS:    return "Mould A In";
        case AUTO_STEP_MIXER_FILL:     return "Mixer Fill";
        case AUTO_STEP_MOULD_A_RETURN: return "Mould A Out";
        case AUTO_STEP_PRESS_1:        return "Press 1";
        case AUTO_STEP_PRESS_SETTLE:   return "Press Gap";
        case AUTO_STEP_PRESS_2:        return "Press 2";
        case AUTO_STEP_PRE_RELEASE:    return "Pre-Release";
        case AUTO_STEP_RELEASE:        return "Release";
        case AUTO_STEP_MOULD_CONFIRM:  return "Confirming";
        default:                       return "Idle";
    }
}
