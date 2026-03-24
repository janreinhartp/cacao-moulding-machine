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
#define RELAY_MIXER             0   /* P00 */
#define RELAY_MOULD_A           1   /* P01 - Up/Down */
#define RELAY_MOULD_B           2   /* P02 - Forward/Reverse */
#define RELAY_RELEASE           3   /* P03 */
#define RELAY_HEATER            4   /* P04 */
#define RELAY_SPARE_1           5   /* P05 */
#define RELAY_SPARE_2           6   /* P06 */
#define RELAY_SPARE_3           7   /* P07 */
#define RELAY_COUNT             8

/* ─── Relay names for UI display ─── */
#define RELAY_NAMES { \
    "Mixer",      \
    "Mould A",    \
    "Mould B",    \
    "Release",    \
    "Heater",     \
    "Spare 1",    \
    "Spare 2",    \
    "Spare 3"     \
}

/* ─── Default Timer Durations (seconds) ─── */
#define DEFAULT_MIXER_TIME_S        30
#define DEFAULT_MOULD_A_TIME_S      10
#define DEFAULT_MOULD_B_TIME_S      10
#define DEFAULT_RELEASE_TIME_S      5
#define DEFAULT_HEATER_TIME_S       20

/* ─── FreeRTOS Task Configuration ─── */
#define TASK_UI_STACK_SIZE          4096
#define TASK_UI_PRIORITY            3
#define TASK_MACHINE_STACK_SIZE     4096
#define TASK_MACHINE_PRIORITY       5
#define TASK_INPUT_STACK_SIZE       2048
#define TASK_INPUT_PRIORITY         4

/* ─── System Timing ─── */
#define BUTTON_DEBOUNCE_MS          200
#define UI_REFRESH_INTERVAL_MS      100
#define WATCHDOG_TIMEOUT_S          30

#ifdef __cplusplus
}
#endif
