# Cacao Moulding Machine - Firmware Architecture

## Project Structure

```
CACAO-MOULDING-MACHINE/
├── CMakeLists.txt                    # Top-level project CMake
├── main/
│   ├── CMakeLists.txt                # Main component CMake
│   ├── main.c                        # Application entry point
│   ├── include/
│   │   ├── board_config.h            # Pin definitions & system config
│   │   ├── app_events.h              # Event types & queue declarations
│   │   ├── ui_manager.h              # UI task interface
│   │   ├── ui_screens.h              # Screen rendering functions
│   │   ├── machine_control.h         # Machine state machine interface
│   │   └── auto_sequence.h           # Auto production sequence interface
│   ├── ui/
│   │   ├── ui_manager.c              # UI task, input routing, screen mgmt
│   │   └── ui_screens.c              # Individual screen renderers
│   └── machine/
│       ├── machine_control.c         # State machine, task, self-test
│       └── auto_sequence.c           # Sequential automation engine
├── peripheral/
│   ├── CMakeLists.txt                # Peripheral component CMake
│   ├── include/
│   │   ├── i2c_manager.h             # Shared I2C bus interface
│   │   ├── pcf8575.h                 # PCF8575 I/O expander interface
│   │   ├── relay_control.h           # Relay abstraction interface
│   │   ├── button_input.h            # Button handler interface
│   │   ├── oled_display.h            # OLED display driver interface
│   │   └── nvs_settings.h            # NVS settings interface
│   ├── i2c_manager.c                 # I2C bus init + mutex protection
│   ├── pcf8575.c                     # PCF8575 read/write/interrupt
│   ├── relay_control.c               # LOW-level trigger relay logic
│   ├── button_input.c                # Debounced ISR + task processing
│   ├── oled_display.c                # SSD1306 framebuffer rendering
│   └── nvs_settings.c                # NVS blob persistence
└── documentation/
    └── ARCHITECTURE.md               # This file
```

---

## System Architecture

### FreeRTOS Tasks

| Task           | Priority | Stack  | Role                                    |
|----------------|----------|--------|-----------------------------------------|
| `btn_task`     | 4        | 2048   | Debounces ISR events, calls callback    |
| `ui_task`      | 3        | 4096   | OLED rendering, menu navigation         |
| `machine_task` | 5        | 4096   | State machine, auto-sequence, relays    |

### Inter-Task Communication

```
  [Button ISR]
       │  (ISR-safe queue)
       ▼
  [btn_task]  ──callback──►  [on_button_press()]
                                    │
                    ┌───────────────┼───────────────┐
                    ▼                               ▼
          g_ui_event_queue              g_machine_event_queue
                    │                               │
                    ▼                               ▼
              [ui_task]                      [machine_task]
           Screen rendering               State transitions
           Input processing               Auto-sequence tick
           Menu navigation                Relay control
```

---

## State Machine Diagram

```
                    ┌──────────────┐
        Power On ──►│  SELF_TEST   │
                    └──────┬───────┘
                     pass  │  fail
              ┌────────────┤────────────┐
              ▼                         ▼
        ┌───────────┐            ┌───────────┐
        │   IDLE    │            │   ERROR   │◄─── fault detected
        └─────┬─────┘            └─────┬─────┘
              │                        │ Enter btn
              ▼                        ▼
        ┌───────────────┐        ┌───────────┐
        │ MENU_NAVIGATION│◄──────│   IDLE    │
        └───┬───┬───┬───┘        └───────────┘
            │   │   │
     ┌──────┘   │   └──────┐
     ▼          ▼          ▼
┌──────────┐ ┌──────────┐ ┌──────────────┐
│ SETTINGS │ │ RUN_AUTO │ │ TEST_MACHINE │
│          │ │          │ │              │
│ Edit     │ │ Mixer    │ │ Toggle       │
│ values   │ │ Mould A  │ │ individual   │
│ NVS save │ │ Mould B  │ │ relays       │
│          │ │ Release  │ │              │
│          │ │ Heater   │ │ Exit: all OFF│
└────┬─────┘ │ Complete │ └──────┬───────┘
     │       └────┬─────┘        │
     │            │              │
     └──────┬─────┴──────┬───────┘
            ▼            │
      MENU_NAVIGATION    │
                         │
     ════════════════════╪════════════════
     ALL BUTTONS PRESSED │ (any RUN state)
                         ▼
                ┌──────────────────┐
                │ EMERGENCY_STOP   │
                │ All relays OFF   │
                │ Press Enter      │
                │ → return to IDLE │
                └──────────────────┘
```

---

## Module Descriptions

### Peripheral Layer

#### `i2c_manager`
- Initializes the I2C master bus (ESP-IDF v5.x new driver API)
- Provides mutex-protected access for shared bus (OLED + PCF8575)
- Lock/unlock functions for thread-safe I2C transactions

#### `pcf8575`
- 16-bit I/O expander driver
- Cached output state for efficient single-pin operations
- Interrupt support (negative edge on GPIO7)
- All I2C operations go through i2c_manager mutex

#### `relay_control`
- **Critical**: LOW-level trigger logic
  - Relay ON  = PCF8575 pin LOW  (bit cleared)
  - Relay OFF = PCF8575 pin HIGH (bit set)
- Abstract ON/OFF/toggle/all-off operations
- Tracks relay states in a local bitmask for fast queries

#### `button_input`
- ISR-driven with FreeRTOS queue for deferred processing
- Software debouncing (50ms default) using `esp_timer_get_time()`
- Callback function invoked from task context (safe for any operation)

#### `oled_display`
- SSD1306 128x64 driver with internal framebuffer (1024 bytes)
- 5x7 pixel font supporting ASCII 32–126
- Functions: clear, draw_string, draw_title, draw_hline, draw_progress_bar, flush
- All rendering happens in framebuffer → single I2C flush

#### `nvs_settings`
- Stores `machine_settings_t` as a blob in NVS namespace `"cacao_cfg"`
- Loads on boot, applies defaults if no valid settings found
- Individual field update + save, or full reset to defaults

### Application Layer

#### `machine_control`
- Central state machine (see diagram above)
- Self-test on startup: PCF8575 comms, relay reset, NVS validation
- Clean state transitions with entry/exit actions
- Emergency stop: immediate relay shutdown from any state

#### `auto_sequence`
- Non-blocking timer-based sequential automation
- Steps: Mixer → Mould A → Mould B → Release → Heater → Complete
- Each step activates its relay for the configured duration (from NVS)
- `auto_sequence_tick()` called periodically from machine task

#### `ui_manager`
- Runs the UI FreeRTOS task
- Processes button events from the queue
- State-dependent input routing (each screen has its own handler)
- Renders current screen at controlled refresh rate (100ms)
- Emergency stop detection: all 3 buttons within 500ms window

#### `ui_screens`
- Pure rendering functions (no state management)
- Screens: Splash, Main Menu, Settings List, Settings Edit, Run Auto, Test Machine, Emergency Stop, Self-Test, Error

---

## Sample Log Output

```
I (325) app_main: ========================================
I (325) app_main:   Cacao Moulding Machine v1.0
I (325) app_main:   MCU: ESP32-S3
I (325) app_main: ========================================
I (335) app_main: Event queues created
I (345) nvs_set: Settings loaded from NVS
I (355) i2c_mgr: I2C bus initialized (SDA=8, SCL=9, freq=400000 Hz)
I (365) pcf8575: PCF8575 initialized at addr 0x20
I (365) pcf8575: PCF8575 interrupt configured on GPIO7
I (375) relay_ctrl: All relays OFF
I (375) relay_ctrl: Relay control initialized - all relays OFF
I (385) oled: OLED display initialized (128x64)
I (395) button: Button input initialized (3 buttons)
I (405) machine: Machine control initialized
I (405) machine: Running self-test...
I (415) machine:   PCF8575 comms: OK (read 0xFFFF)
I (415) machine:   Relay reset: OK
I (425) machine:   NVS settings: OK
I (425) machine: Self-test PASSED
I (435) machine: State: SELF_TEST -> IDLE
I (435) ui_mgr: UI manager initialized
I (445) app_main: ========================================
I (445) app_main:   System initialization complete
I (455) app_main:   All tasks running
I (455) app_main: ========================================
I (465) app_main: Settings: Mixer=30s, MouldA=10s, MouldB=10s, Release=5s, Heater=20s
I (2435) ui_mgr: Screen -> 1
I (5500) button: Button 2 pressed
I (8200) button: Button 1 pressed
I (8200) ui_mgr: Screen -> 4
I (8200) machine: State: MENU_NAV -> RUN_AUTO
I (8210) auto_seq: Auto sequence STARTED
I (8210) auto_seq: Step: Mixing (30 s)
I (8210) relay_ctrl: Relay 0 ON
I (38210) auto_seq: Step: Mould A (10 s)
I (38210) relay_ctrl: Relay 0 OFF
I (38210) relay_ctrl: Relay 1 ON
```

---

## Hardware Wiring Reference

| Signal        | ESP32-S3 Pin | Notes                    |
|---------------|-------------|--------------------------|
| I2C SDA       | GPIO 8      | Shared: OLED + PCF8575   |
| I2C SCL       | GPIO 9      | Shared: OLED + PCF8575   |
| PCF8575 INT   | GPIO 7      | Active-low interrupt     |
| Button PREV   | GPIO 4      | Active-low, internal pullup |
| Button ENTER  | GPIO 5      | Active-low, internal pullup |
| Button NEXT   | GPIO 6      | Active-low, internal pullup |

### PCF8575 Relay Mapping (P0–P7, LOW = Relay ON)

| Pin | Relay        | Function          |
|-----|-------------|-------------------|
| P00 | Mixer       | Mixing motor      |
| P01 | Mould A     | Up/Down actuator  |
| P02 | Mould B     | Forward/Reverse   |
| P03 | Release     | Mould release     |
| P04 | Heater      | Heating element   |
| P05 | Spare 1     | Reserved          |
| P06 | Spare 2     | Reserved          |
| P07 | Spare 3     | Reserved          |

---

## Build & Flash

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COM# flash monitor
```
