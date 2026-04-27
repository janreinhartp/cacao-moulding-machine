# Cacao Moulding Machine — ESP32-S3 Firmware

Automated controller for a cacao chocolate moulding machine. Manages a repeatable 9-step moulding cycle driven by an ESP32-S3, a PCF8575 relay expander, a 128×64 OLED display, and three push-buttons.

---

## Hardware

| Component | Detail |
|---|---|
| MCU | ESP32-S3 |
| Relay driver | PCF8575 16-bit I2C I/O expander @ 0x20, active-LOW |
| Display | SSD1306 128×64 OLED @ 0x3C (I2C) |
| Buttons | 3× momentary push-button (PREV / ENTER / NEXT) |
| I2C bus | SDA = GPIO8, SCL = GPIO9, 400 kHz |

### GPIO Pin Map

| Signal | GPIO |
|---|---|
| I2C SDA | 8 |
| I2C SCL | 9 |
| PCF8575 INT | 7 |
| BTN PREV | 4 |
| BTN ENTER | 5 |
| BTN NEXT | 6 |

### PCF8575 Relay Mapping (P0–P7)

| PCF Pin | Relay | Notes |
|---|---|---|
| P00 | Spare 3 | Unused |
| P01 | Spare 2 | Unused |
| P02 | Mould A | Remapped from P06 (hardware fault on P06) |
| P03 | Heater | |
| P04 | Release | Ejects moulded balls |
| P05 | Mould B | Press actuator |
| P06 | Spare 1 | Hardware fault — do not use |
| P07 | Mixer | Cacao chocolate mixer |

All relays are **active-LOW**: pulling a PCF8575 pin LOW turns the relay ON.

---

## Software Stack

- **ESP-IDF v5.5.3** (CMake-based)
- **FreeRTOS** — 3 tasks:
  - `machine_task` — priority 5, sequence engine & state machine
  - `button_task` — priority 4, ISR-based input with debounce
  - `ui_task` — priority 3, display rendering & screen routing
- **u8g2** — display graphics library (git submodule at `components/u8g2/`)
- **NVS** — persistent settings storage

---

## Project Structure

```
main/
  main.c                  App entry point, queue init, task launch
  machine/
    machine_control.c     Machine state machine + task loop
    auto_sequence.c       Non-blocking moulding sequence engine
  ui/
    ui_manager.c          UI task, screen transitions, input routing
    ui_screens.c          u8g2 pixel rendering for all screens
  include/
    app_events.h          Event types and enums (IPC)
    auto_sequence.h       Sequence engine public API
    ui_manager.h          UI screen enum and public API
    ui_screens.h          Screen rendering function declarations

peripheral/
  button_input.c          ISR button handler, debounce, long-press repeat
  i2c_manager.c           Shared I2C bus with mutex
  relay_control.c         PCF8575 relay on/off/toggle/is_on
  oled_display.c          u8g2 SSD1306 driver init and accessor
  nvs_settings.c          Load/save machine settings to NVS
  pcf8575.c               Low-level PCF8575 I2C driver
  include/
    board_config.h        All pin definitions, relay map, system constants
    nvs_settings.h        Settings struct + public API

components/
  u8g2/                   u8g2 graphics library (git submodule)

documentation/
  ARCHITECTURE.md         Detailed architecture notes
```

---

## Machine State Machine

```
SELF_TEST → IDLE → MENU_NAVIGATION
                        ├── SETTINGS
                        ├── RUN_AUTO
                        └── TEST_MACHINE
```

Emergency stop can be triggered from any state by pressing **all 3 buttons simultaneously**.

---

## Moulding Cycle (Auto Run)

Each cycle produces **6 balls**. Steps are timed automatically except steps 8–9 which wait for operator input.

| Step | Name | Action | Timer |
|---|---|---|---|
| 1 | Mould A In | Mould A ON | `mould_a_in_s` |
| 2 | Mixer Fill | Mixer ON | `mixer_fill_s` |
| 3 | Mould A Out | Mixer OFF + Mould A OFF | `mould_a_out_s` |
| 4 | Press 1 | Mould B ON | `press_time_s` |
| 5 | Press Gap | Mould B OFF | `press_gap_s` |
| 6 | Press 2 | Mould B ON | `press_time_s` |
| 7 | Pre-Release | Mould B OFF | `pre_release_s` |
| 8 | Release | Release ON | **Wait: press ENTER** |
| 9 | Confirming | Release OFF | **Wait: press ENTER to repeat** |

After step 9, pressing ENTER restarts from step 1. Cycle count and total ball count are displayed live on the OLED.

---

## Configurable Settings

All values are in **seconds** and are persisted to NVS (namespace `cacao_cfg`, key `settings_v2`).

| Index | Name | Default | Description |
|---|---|---|---|
| 0 | Mixer Fill | 15 s | Mixer run time to fill Mould A |
| 1 | Mould A In | 3 s | Mould A positioning time |
| 2 | Press Time | 5 s | Duration of each Mould B press |
| 3 | Mould A Out | 3 s | Mould A return time |
| 4 | Press Gap | 2 s | Settle time between two presses |
| 5 | Pre-Release | 3 s | Settle time before Release fires |

Settings are edited from the **Settings** screen (PREV/NEXT to adjust, ENTER on "Save All" to persist).

---

## Button Controls

| Button | Short Press | Long Press (held) |
|---|---|---|
| PREV | Move cursor / decrement value | Rapid decrement |
| NEXT | Move cursor / increment value | Rapid increment |
| ENTER | Confirm / select / advance | — |
| **All 3 simultaneously** | **Emergency stop** | — |

Debounce: 100 ms. Long-press activates after 500 ms, slow repeat at 180 ms, fast repeat at 60 ms (after 6 steps).

---

## OLED Screens

| Screen | Description |
|---|---|
| Splash | Startup logo / boot |
| Main Menu | Settings / Run Auto / Test Machine |
| Settings | List of 6 configurable timers |
| Settings Edit | Adjust a single value |
| Auto Run | Current step name, countdown, progress bar, cycle count, ball count |
| Release Wait | Operator removes cacao balls, press ENTER when done |
| Mould Confirm | Shows cycle number and total balls; ENTER to repeat |
| Test Machine | Toggle individual relays manually |
| Emergency Stop | Halts all relays; press ENTER to return to menu |
| Error | Displays error message |

---

## Test Machine Safety Interlock

In the **Test Machine** screen, **Mould A**, **Mould B**, and **Release** are mutually exclusive — only one of the three can be ON at a time. Attempting to turn one ON while another is already ON is silently blocked. Heater and Mixer have no such restriction.

---

## Building and Flashing

**Requirements:**
- ESP-IDF v5.5.3 installed
- Target: `esp32s3`
- Flash port: COM22 (adjust as needed), 460800 baud

```bash
# Build
idf.py build

# Flash
idf.py -p COM22 flash

# Flash + monitor
idf.py -p COM22 flash monitor
```

---

## Known Hardware Notes

- **PCF8575 P06** has a hardware fault and must not be used. Mould A was remapped to P02 as a workaround.
- The I2C bus is shared between the OLED and PCF8575 and is protected by a FreeRTOS mutex.
- NVS key was bumped from `settings` → `settings_v2` to force a fresh defaults load after the settings struct was redesigned.
