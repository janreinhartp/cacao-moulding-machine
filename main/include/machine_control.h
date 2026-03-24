/**
 * @file machine_control.h
 * @brief Machine control state machine and task.
 */
#pragma once

#include "esp_err.h"
#include "app_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the machine control system and start the control task.
 * @return ESP_OK on success.
 */
esp_err_t machine_control_init(void);

/**
 * @brief Get the current machine state.
 */
machine_state_t machine_get_state(void);

/**
 * @brief Request a state transition.
 * @param new_state Target state.
 */
void machine_request_state(machine_state_t new_state);

/**
 * @brief Trigger emergency stop: all relays off, enter safe state.
 */
void machine_emergency_stop(void);

/**
 * @brief Perform startup self-test.
 * @return true if all tests passed.
 */
bool machine_self_test(void);

#ifdef __cplusplus
}
#endif
