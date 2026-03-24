/**
 * @file ui_screens.c
 * @brief Screen rendering implementations for the OLED UI.
 */

#include "ui_screens.h"
#include "oled_display.h"
#include "nvs_settings.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

static const char *s_relay_names[] = RELAY_NAMES;
static const char *s_setting_names[] = SETTINGS_NAMES;

void ui_screen_splash(void)
{
    oled_clear();
    oled_draw_title("CACAO MACHINE");
    oled_draw_string(20, 3, "Initializing...", false);
    oled_draw_string(10, 5, "v1.0  ESP32-S3", false);
    oled_flush();
}

void ui_screen_main_menu(uint8_t cursor_pos)
{
    static const char *items[] = {
        "Settings",
        "Run Auto",
        "Test Machine",
    };
    const int item_count = 3;

    oled_clear();
    oled_draw_title("MAIN MENU");

    for (int i = 0; i < item_count; i++) {
        char line[22];
        snprintf(line, sizeof(line), " %s", items[i]);
        bool selected = (i == (int)cursor_pos);
        if (selected) {
            line[0] = '>';
        }
        oled_draw_string(4, (uint8_t)(i + 2), line, selected);
    }

    oled_draw_string(4, 7, "Prev  Enter  Next", false);
    oled_flush();
}

void ui_screen_settings(uint8_t cursor_pos, const uint16_t *values)
{
    oled_clear();
    oled_draw_title("SETTINGS");

    /* Show up to 4 settings at a time with scrolling */
    int start = 0;
    if (cursor_pos >= 4) {
        start = cursor_pos - 3;
    }

    for (int i = 0; i < 4 && (start + i) < SETTINGS_COUNT; i++) {
        int idx = start + i;
        char line[22];
        snprintf(line, sizeof(line), " %-10s %3us", s_setting_names[idx], values[idx]);
        bool selected = (idx == (int)cursor_pos);
        if (selected) {
            line[0] = '>';
        }
        oled_draw_string(0, (uint8_t)(i + 2), line, selected);
    }

    oled_draw_string(4, 7, "Prev  Edit   Next", false);
    oled_flush();
}

void ui_screen_settings_edit(uint8_t index, uint16_t value)
{
    oled_clear();
    oled_draw_title("EDIT SETTING");

    if (index < SETTINGS_COUNT) {
        oled_draw_string(4, 2, s_setting_names[index], false);
    }

    char val_str[16];
    snprintf(val_str, sizeof(val_str), ">> %u s <<", value);
    oled_draw_string(20, 4, val_str, true);

    oled_draw_string(4, 7, " -    Save    + ", false);
    oled_flush();
}

void ui_screen_settings_saved(void)
{
    oled_clear();
    oled_draw_title("SETTINGS");
    oled_draw_string(20, 3, "Settings Saved!", false);
    oled_draw_string(30, 5, "OK", true);
    oled_flush();
}

void ui_screen_run_auto(const char *step_name, uint32_t remaining_s, uint32_t total_s)
{
    oled_clear();
    oled_draw_title("AUTO RUN");

    char line[22];
    snprintf(line, sizeof(line), "Step: %s", step_name ? step_name : "---");
    oled_draw_string(4, 2, line, false);

    uint8_t mins = (uint8_t)(remaining_s / 60);
    uint8_t secs = (uint8_t)(remaining_s % 60);
    snprintf(line, sizeof(line), "Time: %02u:%02u", mins, secs);
    oled_draw_string(4, 4, line, false);

    /* Progress bar */
    uint8_t progress = 0;
    if (total_s > 0) {
        uint32_t elapsed = total_s - remaining_s;
        progress = (uint8_t)(elapsed * 100 / total_s);
    }
    oled_draw_progress_bar(4, 6, 120, progress);

    oled_draw_string(0, 7, "ALL BTN = E-STOP", false);
    oled_flush();
}

void ui_screen_run_complete(void)
{
    oled_clear();
    oled_draw_title("AUTO RUN");
    oled_draw_string(15, 3, "Cycle Complete!", false);
    oled_draw_string(20, 5, "Press Enter", false);
    oled_flush();
}

void ui_screen_test_machine(uint8_t cursor_pos, uint8_t relay_states)
{
    oled_clear();
    oled_draw_title("TEST MACHINE");

    /* Show up to 4 relays at a time with scrolling */
    int start = 0;
    if (cursor_pos >= 4) {
        start = cursor_pos - 3;
    }

    for (int i = 0; i < 4 && (start + i) < RELAY_COUNT; i++) {
        int idx = start + i;
        char line[22];
        bool is_on = (relay_states & (1 << idx)) != 0;
        snprintf(line, sizeof(line), " %-10s [%s]",
                 s_relay_names[idx], is_on ? "ON " : "OFF");
        bool selected = (idx == (int)cursor_pos);
        if (selected) {
            line[0] = '>';
        }
        oled_draw_string(0, (uint8_t)(i + 2), line, selected);
    }

    oled_draw_string(0, 7, "Prev Toggle  Next", false);
    oled_flush();
}

void ui_screen_emergency_stop(void)
{
    oled_clear();
    oled_draw_string(10, 1, "!! EMERGENCY !!", true);
    oled_draw_string(30, 3, "STOP", true);
    oled_draw_string(5, 5, "All relays OFF", false);
    oled_draw_string(5, 7, "Press Enter", false);
    oled_flush();
}

void ui_screen_self_test(const char *status_msg)
{
    oled_clear();
    oled_draw_title("SELF TEST");
    oled_draw_string(4, 3, status_msg ? status_msg : "Testing...", false);
    oled_flush();
}

void ui_screen_error(const char *error_msg)
{
    oled_clear();
    oled_draw_string(15, 1, "!! ERROR !!", true);
    if (error_msg) {
        oled_draw_string(4, 3, error_msg, false);
    }
    oled_draw_string(5, 6, "Press Enter to", false);
    oled_draw_string(5, 7, "return to menu", false);
    oled_flush();
}
