/**
 * @file ui_screens.c
 * @brief Screen rendering using the u8g2 graphics library.
 */

#include "ui_screens.h"
#include "oled_display.h"
#include "nvs_settings.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

/* ── Layout constants (pixels, SSD1306 128x64) ────────────────────────────── */
#define DISPLAY_W    128
#define DISPLAY_H    64

#define FONT_BODY    u8g2_font_6x10_tf
#define CHAR_W       6
#define CHAR_H       10
#define CHAR_ASCENT  7     /* approx. pixels above baseline for this font */

/* Vertical baselines */
#define Y_TITLE      8
#define Y_TITLE_BOX  11    /* height of title background box */
#define Y_SEP        11    /* separator line y position */
#define Y_ROW1       22
#define Y_ROW2       33
#define Y_ROW3       44
#define Y_ROW4       55
#define Y_HINT       63

static const uint8_t s_content_y[] = { Y_ROW1, Y_ROW2, Y_ROW3, Y_ROW4 };

static const char *s_relay_names[]   = RELAY_NAMES;
static const char *s_setting_names[] = SETTINGS_NAMES;

/* ── Shared helpers ──────────────────────────────────────────────────────── */

static void draw_title_bar(u8g2_t *u8g2, const char *title)
{
    /* White filled bar at the top */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, 0, DISPLAY_W, Y_TITLE_BOX);

    /* Centred title text in black */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_SetFontMode(u8g2, 1);
    uint8_t tw = (uint8_t)u8g2_GetStrWidth(u8g2, title);
    uint8_t tx = (tw < DISPLAY_W) ? (DISPLAY_W - tw) / 2 : 0;
    u8g2_DrawStr(u8g2, tx, Y_TITLE, title);

    /* Restore white drawing colour */
    u8g2_SetDrawColor(u8g2, 1);
}

static void draw_hint(u8g2_t *u8g2, const char *hint)
{
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);
    uint8_t tw = (uint8_t)u8g2_GetStrWidth(u8g2, hint);
    uint8_t tx = (tw < DISPLAY_W) ? (DISPLAY_W - tw) / 2 : 0;
    u8g2_DrawStr(u8g2, tx, Y_HINT, hint);
}

static void draw_menu_row(u8g2_t *u8g2, uint8_t row_idx, const char *label,
                          bool selected)
{
    if (row_idx >= 4) return;
    uint8_t y = s_content_y[row_idx];

    if (selected) {
        /* Highlight: white box, black text */
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawBox(u8g2, 0, (uint8_t)(y - CHAR_ASCENT), DISPLAY_W, CHAR_H);
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_SetFontMode(u8g2, 1);
        u8g2_DrawStr(u8g2, 4, y, label);
        u8g2_SetDrawColor(u8g2, 1);
    } else {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_SetFontMode(u8g2, 1);
        u8g2_DrawStr(u8g2, 4, y, label);
    }
}

/* ── Screen implementations ─────────────────────────────────────────────── */

void ui_screen_splash(void)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "CACAO MACHINE");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    const char *sub = "Initializing...";
    uint8_t sw = (uint8_t)u8g2_GetStrWidth(u8g2, sub);
    u8g2_DrawStr(u8g2, (DISPLAY_W - sw) / 2, Y_ROW2, sub);

    const char *ver = "v1.0  ESP32-S3";
    uint8_t vw = (uint8_t)u8g2_GetStrWidth(u8g2, ver);
    u8g2_DrawStr(u8g2, (DISPLAY_W - vw) / 2, Y_ROW4, ver);

    u8g2_SendBuffer(u8g2);
}

void ui_screen_main_menu(uint8_t cursor_pos)
{
    static const char *items[] = { "Settings", "Run Auto", "Test Machine" };

    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "MAIN MENU");

    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < 3; i++) {
        draw_menu_row(u8g2, (uint8_t)i, items[i], i == (int)cursor_pos);
    }

    draw_hint(u8g2, "PREV  OK  NEXT");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_settings(uint8_t cursor_pos, const uint16_t *values)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "SETTINGS");

    /* Total items: SETTINGS_COUNT settings + 1 "Save All" entry */
    int total_items = SETTINGS_COUNT + 1;
    int start = 0;
    if (cursor_pos >= 4) start = (int)cursor_pos - 3;

    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < 4 && (start + i) < total_items; i++) {
        int idx = start + i;
        if (idx == SETTINGS_COUNT) {
            draw_menu_row(u8g2, (uint8_t)i, "[ Save All ]", idx == (int)cursor_pos);
        } else {
            char line[22];
            snprintf(line, sizeof(line), "%-10s %3us", s_setting_names[idx], values[idx]);
            draw_menu_row(u8g2, (uint8_t)i, line, idx == (int)cursor_pos);
        }
    }

    draw_hint(u8g2, "PREV  EDIT  NEXT");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_settings_edit(uint8_t index, uint16_t value)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "EDIT SETTING");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    if (index < SETTINGS_COUNT) {
        uint8_t nw = (uint8_t)u8g2_GetStrWidth(u8g2, s_setting_names[index]);
        u8g2_DrawStr(u8g2, (DISPLAY_W - nw) / 2, Y_ROW2, s_setting_names[index]);
    }

    char val_str[16];
    snprintf(val_str, sizeof(val_str), "< %u s >", value);

    /* Highlighted value block */
    uint8_t vw = (uint8_t)u8g2_GetStrWidth(u8g2, val_str);
    uint8_t vx = (DISPLAY_W - vw) / 2;
    u8g2_DrawBox(u8g2, (uint8_t)(vx - 2), (uint8_t)(Y_ROW3 - CHAR_ASCENT),
                 (uint8_t)(vw + 4), CHAR_H);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawStr(u8g2, vx, Y_ROW3, val_str);
    u8g2_SetDrawColor(u8g2, 1);

    draw_hint(u8g2, "  -   SAVE   +  ");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_settings_saved(void)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "SETTINGS");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    const char *msg = "Settings Saved!";
    uint8_t mw = (uint8_t)u8g2_GetStrWidth(u8g2, msg);
    u8g2_DrawStr(u8g2, (DISPLAY_W - mw) / 2, Y_ROW2, msg);

    const char *ok = "[ OK ]";
    uint8_t ow = (uint8_t)u8g2_GetStrWidth(u8g2, ok);
    uint8_t ox = (DISPLAY_W - ow) / 2;
    u8g2_DrawBox(u8g2, (uint8_t)(ox - 2), (uint8_t)(Y_ROW4 - CHAR_ASCENT),
                 (uint8_t)(ow + 4), CHAR_H);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawStr(u8g2, ox, Y_ROW4, ok);

    u8g2_SendBuffer(u8g2);
}

void ui_screen_run_auto(const char *step_name, uint32_t remaining_s, uint32_t total_s, uint16_t cycle_count)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "AUTO RUN");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    /* Step name */
    char step_line[22];
    snprintf(step_line, sizeof(step_line), "Step: %s", step_name ? step_name : "---");
    u8g2_DrawStr(u8g2, 2, Y_ROW1, step_line);

    /* Countdown timer */
    uint8_t mins = (uint8_t)(remaining_s / 60);
    uint8_t secs = (uint8_t)(remaining_s % 60);
    char time_str[14];
    snprintf(time_str, sizeof(time_str), "Time: %02u:%02u", mins, secs);
    u8g2_DrawStr(u8g2, 2, Y_ROW2, time_str);

    /* Progress bar */
    uint8_t bar_x = 4, bar_w = 120, bar_h = 7;
    uint8_t bar_y = (uint8_t)(Y_ROW3 - CHAR_ASCENT + 1);
    u8g2_DrawFrame(u8g2, bar_x, bar_y, bar_w, bar_h);
    if (total_s > 0) {
        uint32_t elapsed = total_s - remaining_s;
        uint8_t fill_w = (uint8_t)((elapsed * (bar_w - 2)) / total_s);
        if (fill_w > 0) {
            u8g2_DrawBox(u8g2, (uint8_t)(bar_x + 1), (uint8_t)(bar_y + 1),
                         fill_w, (uint8_t)(bar_h - 2));
        }
    }

    /* Cycle count (left) and total balls (right) on the same row */
    uint16_t balls = (uint16_t)(cycle_count * BALLS_PER_CYCLE);
    char cyc_str[12];
    snprintf(cyc_str, sizeof(cyc_str), "Cyc: %u", cycle_count);
    char ball_str[14];
    snprintf(ball_str, sizeof(ball_str), "Balls: %u", balls);
    u8g2_DrawStr(u8g2, 2, Y_ROW4, cyc_str);
    uint8_t bx = (uint8_t)(DISPLAY_W - u8g2_GetStrWidth(u8g2, ball_str));
    u8g2_DrawStr(u8g2, bx, Y_ROW4, ball_str);

    draw_hint(u8g2, "ALL BTNS = E-STOP");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_run_complete(void)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "AUTO RUN");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    const char *msg = "Cycle Complete!";
    uint8_t mw = (uint8_t)u8g2_GetStrWidth(u8g2, msg);
    u8g2_DrawStr(u8g2, (DISPLAY_W - mw) / 2, Y_ROW2, msg);

    draw_hint(u8g2, "Press ENTER");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_release_wait(void)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "RELEASE");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    const char *l1 = "Release running";
    uint8_t w1 = (uint8_t)u8g2_GetStrWidth(u8g2, l1);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w1) / 2, Y_ROW2, l1);

    const char *l2 = "Remove cacao";
    uint8_t w2 = (uint8_t)u8g2_GetStrWidth(u8g2, l2);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w2) / 2, Y_ROW3, l2);

    draw_hint(u8g2, "ENTER when done");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_mould_confirm(uint16_t cycle_count)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "MOULDING");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    char l1[22];
    snprintf(l1, sizeof(l1), "Cycle #%u done!", cycle_count);
    uint8_t w1 = (uint8_t)u8g2_GetStrWidth(u8g2, l1);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w1) / 2, Y_ROW2, l1);

    uint16_t balls = (uint16_t)(cycle_count * BALLS_PER_CYCLE);
    char l2[20];
    snprintf(l2, sizeof(l2), "Total balls: %u", balls);
    uint8_t w2 = (uint8_t)u8g2_GetStrWidth(u8g2, l2);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w2) / 2, Y_ROW3, l2);

    draw_hint(u8g2, "ENTER=Repeat");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_test_machine(uint8_t cursor_pos, uint8_t relay_states)
{
    static const uint8_t s_test_relay_list[] = RELAY_TEST_LIST;

    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "TEST MACHINE");

    int total = RELAY_TEST_COUNT + 1;  /* functional relays + Exit */
    int start = 0;
    if (cursor_pos >= 4) start = cursor_pos - 3;

    u8g2_SetDrawColor(u8g2, 1);
    for (int i = 0; i < 4 && (start + i) < total; i++) {
        int idx = start + i;
        char line[22];
        if (idx == RELAY_TEST_COUNT) {
            snprintf(line, sizeof(line), "[ Exit ]");
        } else {
            int pin = s_test_relay_list[idx];
            bool is_on = (relay_states & (1 << pin)) != 0;
            snprintf(line, sizeof(line), "%-9s [%s]", s_relay_names[pin],
                     is_on ? "ON " : "OFF");
        }
        draw_menu_row(u8g2, (uint8_t)i, line, idx == (int)cursor_pos);
    }

    draw_hint(u8g2, "ENTER=Toggle/Exit");
    u8g2_SendBuffer(u8g2);
}

void ui_screen_emergency_stop(void)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    /* Full-screen white background */
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, 0, DISPLAY_W, DISPLAY_H);

    /* Black text on white */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_SetFontMode(u8g2, 1);

    const char *t1 = "!! EMERGENCY !!";
    uint8_t w1 = (uint8_t)u8g2_GetStrWidth(u8g2, t1);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w1) / 2, 18, t1);

    const char *t2 = "STOP";
    uint8_t w2 = (uint8_t)u8g2_GetStrWidth(u8g2, t2);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w2) / 2, 30, t2);

    const char *t3 = "All relays OFF";
    uint8_t w3 = (uint8_t)u8g2_GetStrWidth(u8g2, t3);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w3) / 2, 45, t3);

    const char *t4 = "Press ENTER";
    uint8_t w4 = (uint8_t)u8g2_GetStrWidth(u8g2, t4);
    u8g2_DrawStr(u8g2, (DISPLAY_W - w4) / 2, 58, t4);

    u8g2_SendBuffer(u8g2);
}

void ui_screen_self_test(const char *status_msg)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "SELF TEST");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    const char *msg = status_msg ? status_msg : "Testing...";
    uint8_t mw = (uint8_t)u8g2_GetStrWidth(u8g2, msg);
    uint8_t mx = (mw < DISPLAY_W) ? (DISPLAY_W - mw) / 2 : 0;
    u8g2_DrawStr(u8g2, mx, Y_ROW2, msg);

    u8g2_SendBuffer(u8g2);
}

void ui_screen_error(const char *error_msg)
{
    u8g2_t *u8g2 = oled_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, FONT_BODY);

    draw_title_bar(u8g2, "!! ERROR !!");

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    if (error_msg && error_msg[0] != '\0') {
        /* Split long messages to two lines */
        char buf[32];
        strncpy(buf, error_msg, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        if (strlen(buf) <= 20) {
            uint8_t ew = (uint8_t)u8g2_GetStrWidth(u8g2, buf);
            u8g2_DrawStr(u8g2, (DISPLAY_W - ew) / 2, Y_ROW2, buf);
        } else {
            /* Wrap at word boundary */
            buf[20] = '\0';
            u8g2_DrawStr(u8g2, 2, Y_ROW2, buf);
            strncpy(buf, error_msg + 20, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            u8g2_DrawStr(u8g2, 2, Y_ROW3, buf);
        }
    }

    draw_hint(u8g2, "ENTER = main menu");
    u8g2_SendBuffer(u8g2);
}