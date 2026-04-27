/**
 * @file board_config.h
 * @brief Hardware pin definitions and system configuration for the Cacao Moulding Machine.
 */
#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── I2C Bus Configuration ─── */
#define BOARD_I2C_PORT          I2C_NUM_0
#define BOARD_I2C_SDA_PIN       GPIO_NUM_8
#define BOARD_I2C_SCL_PIN       GPIO_NUM_9
#define BOARD_I2C_FREQ_HZ       400000

/* ─── PCF8575 Configuration ─── */
#define PCF8575_I2C_ADDR        0x20
#define PCF8575_INT_PIN         GPIO_NUM_7

/* ─── Button GPIO Pins ─── */
#define BTN_PREV_PIN            GPIO_NUM_4
#define BTN_ENTER_PIN           GPIO_NUM_5
#define BTN_NEXT_PIN            GPIO_NUM_6

/* ─── OLED Display Configuration ─── */
#define OLED_I2C_ADDR           0x3C
#define OLED_WIDTH              128
#define OLED_HEIGHT             64

/* ─── PCF8575 Relay Pin Mapping (P0–P7) ─── */
/* Active relays are on the high-side pins P03–P07 */
#define RELAY_SPARE_3           0   /* P00 */
#define RELAY_SPARE_2           1   /* P01 */
#define RELAY_MOULD_A           2   /* P02 - remapped from P06 (hardware fault) */
#define RELAY_HEATER            3   /* P03 */
#define RELAY_RELEASE           4   /* P04 */
#define RELAY_MOULD_B           5   /* P05 */
#define RELAY_SPARE_1           6   /* P06 - hardware fault, was Mould A */
#define RELAY_MIXER             7   /* P07 */
#define RELAY_COUNT             8
/* Ordered list of relay indices shown in the test machine screen (no spares) */
#define RELAY_TEST_LIST         { RELAY_MOULD_A, RELAY_HEATER, RELAY_RELEASE, RELAY_MOULD_B, RELAY_MIXER }
#define RELAY_TEST_COUNT        5
#define BALLS_PER_CYCLE         6   /* moulding balls produced per completed cycle */

/* ─── Relay names for UI display (index 0–7 by PCF8575 pin) ─── */
#define RELAY_NAMES { \
    "Spare 3",    \
    "Spare 2",    \
    "Mould A",    \
    "Heater",     \
    "Release",    \
    "Mould B",    \
    "Spare 1",    \
    "Mixer"       \
}

/* ─── Default Timer Durations (seconds) ─── */
#define DEFAULT_MIXER_FILL_S        15  /* Mixer run time to fill Mould A */
#define DEFAULT_MOULD_A_IN_S        3   /* Mould A moves into position under mixer */
#define DEFAULT_PRESS_TIME_S        5   /* Duration of each Mould B press */
#define DEFAULT_MOULD_A_OUT_S       3   /* Mould A returns to neutral */
#define DEFAULT_PRESS_GAP_S         2   /* Settle between two presses */
#define DEFAULT_PRE_RELEASE_S       3   /* Settle after pressing before release fires */

/* ─── FreeRTOS Task Configuration ─── */
#define TASK_UI_STACK_SIZE          4096
#define TASK_UI_PRIORITY            3
#define TASK_MACHINE_STACK_SIZE     4096
#define TASK_MACHINE_PRIORITY       5
#define TASK_INPUT_STACK_SIZE       4096
#define TASK_INPUT_PRIORITY         4

/* ─── System Timing ─── */
#define BUTTON_DEBOUNCE_MS          100
#define UI_REFRESH_INTERVAL_MS      100
#define WATCHDOG_TIMEOUT_S          30

#ifdef __cplusplus
}
#endif
