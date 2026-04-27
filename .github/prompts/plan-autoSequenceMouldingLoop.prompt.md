## Plan: New Auto-Sequence with Moulding Loop

**What & why**: Replace the linear 5-step sequence with a two-phase design: a timed pre-mix phase, then a repeating moulding sub-cycle that pauses for operator confirmation between repetitions. Mixer + Heater run throughout.

---

### New Sequence Flow

```
START → [PREMIX: Mixer+Heater ON] → mixer_time_s countdown
                ↓
       ┌─ MOULDING CYCLE ─────────────────────────────────┐
       │  Mould A ON → mould_a_time_s                     │
       │  Mould A OFF → wait settle_time_s                │
       │  Mould B ON → mould_b_time_s                     │
       │  Mould B OFF → wait settle_time_s                │
       │  Release ON → release_time_s                     │
       │  Release OFF → CONFIRM SCREEN                    │
       │       ↓ ENTER                                    │
       └──────────────────────────────────────────────────┘
   (Mixer + Heater ON throughout all of the above)
   3-button = Emergency stop any time
```

---

### Phase 1 — `app_events.h`
Replace step enum. Remove `AUTO_STEP_MIXER`, `AUTO_STEP_HEATER`, `AUTO_STEP_COMPLETE`. Add:
- `AUTO_STEP_PREMIX`, `AUTO_STEP_MOULD_A_SETTLE`, `AUTO_STEP_MOULD_B_SETTLE`, `AUTO_STEP_MOULD_CONFIRM`

Add two new event types: `EVT_MOULD_CONFIRM_NEEDED` (machine→UI) and `EVT_MOULD_CONFIRM` (UI→machine).

### Phase 2 — Settings: new "Settle Time" (6th setting)
- `board_config.h` — add `DEFAULT_SETTLE_TIME_S 2`
- `nvs_settings.h` — add `settle_time_s` field to `machine_settings_t`, `SETTINGS_COUNT = 6`, update `SETTINGS_NAMES`
- `nvs_settings.c` — add field to `apply_defaults()` and `update_field()` pointer array

### Phase 3 — `auto_sequence.c` rewrite
Replace the generic step table with explicit per-step entry functions. Each one activates/deactivates the right relay(s) and sets a timer duration. Key rules:
- `enter_premix()`: `relay_on(MIXER)` + `relay_on(HEATER)`, timer = `mixer_time_s`
- `enter_mould_a_settle()`: `relay_off(MOULD_A)`, timer = `settle_time_s`
- `enter_mould_b_settle()`: `relay_off(MOULD_B)`, timer = `settle_time_s`
- `enter_mould_confirm()`: `relay_off(RELEASE)`, sets step to `MOULD_CONFIRM`, `s_running` stays `true` but no timer (tick does nothing)
- `auto_sequence_moulding_repeat()`: external call → `enter_mould_a()` (Mixer+Heater remain ON)
- `auto_sequence_stop()`: `relay_all_off()` (including Mixer+Heater)
- New `auto_sequence.h` declarations for `moulding_repeat()` and `is_waiting_confirm()`

### Phase 4 — `machine_control.c`
- Add `static bool s_confirm_notified`
- Periodic RUN_AUTO section: when step == `MOULD_CONFIRM` and not yet notified → send `EVT_MOULD_CONFIRM_NEEDED` to UI queue, set flag
- Handle `EVT_MOULD_CONFIRM` event → call `auto_sequence_moulding_repeat()`, clear flag
- On RUN_AUTO exit action: reset `s_confirm_notified`

### Phase 5 — UI layer
**`ui_manager.h`**: add `UI_SCREEN_MOULD_CONFIRM` enum value

**`ui_manager.c`**:
- `EVT_MOULD_CONFIRM_NEEDED` in event loop → `s_current_screen = UI_SCREEN_MOULD_CONFIRM`
- New `handle_mould_confirm_input()`: ENTER → send `EVT_MOULD_CONFIRM` to machine queue + set screen to `UI_SCREEN_RUN_AUTO`; all other buttons → `check_emergency_stop()`
- Wire new screen into `process_button_event()` and `render_current_screen()`

**`ui_screens.h` + `ui_screens.c`**: add `ui_screen_mould_confirm()` — title "MOULDING", body "Cycle done! Got the balls?", hint "ENTER=Repeat / 3-btn=STOP"

---

### Relevant Files
- `main/include/app_events.h` — step enum + new event types
- `peripheral/include/board_config.h` — `DEFAULT_SETTLE_TIME_S`
- `peripheral/include/nvs_settings.h` — struct + `SETTINGS_COUNT` + names
- `peripheral/nvs_settings.c` — `apply_defaults()` + `update_field()`
- `main/machine/auto_sequence.h` — new function declarations
- `main/machine/auto_sequence.c` — full redesign
- `main/machine/machine_control.c` — confirm handling
- `main/include/ui_manager.h` — new screen enum value
- `main/ui/ui_manager.c` — event handling + new screen
- `main/include/ui_screens.h` — new function decl
- `main/ui/ui_screens.c` — new screen render

---

### Verification
1. Flash → "Run Auto" → confirm Mixer + Heater relays both ON during PREMIX step
2. After `mixer_time_s` → Mould A activates while Mixer + Heater stay ON
3. Trace full sub-cycle relay states (A settle OFF, B ON, B settle OFF, Release ON/OFF)
4. Confirm screen appears after Release deactivates with correct message
5. ENTER repeats — Mould A re-activates, Mixer+Heater still ON
6. 3-button emergency stop from confirm screen → all relays OFF
7. Settings screen shows 6 entries with "Settle Time" last; persists after reboot
