# PD6-Tony: Touchscreen UI (orchestrator) under FreeRTOS

> A record of a pull request in the original project repository (base
> branch `rtos-integration`), kept as it was written.

Brings the host-tested `PD5b_Tony/Application` UI
state machine into the firmware behind **TonyTask**.

**Changed since this PR** (the final firmware). The record below is
kept as it was; these parts of it no longer match the code:
- Knob: TIM3 counts it in encoder mode (`scan_inputs()` in `tony_ui.c`).
  `encoder_sm` is still compiled, but only the host tests use it.
- Effects: the effect and the selected file go to Jack
  (`JACK_CMD_SET_EFFECT`). Effects are made ahead into effect files
  (FxRenderTask) and Jack plays them. Tom ignores `TOM_CMD_SET_EFFECT`.
- Battery: the screens show the gauge's `soc_pct`, or `--%` when the
  reading is not valid. `battery_mv_to_pct()` is no longer called.
- Serial console: the serial console, LogTask and the solo builds were
  removed on 1 Nov 2026. The checklist's log lines and `d` are now checks
  on the TFT: the Home screen appears; every tap acts on the button under
  the finger; the footer shows `REC m:ss`, `FX <effect>` and `MUTE`; the
  Files list shows the names; the Volume screen shows the level; debug
  page 1 shows TonyTask's stack (red below 200 B).

## What the branch does

- Compiles `ui_state_machine`, `display_logic`, `encoder_sm`,
  `touch_calibration`, `touch_region` into the firmware (root
  `CMakeLists.txt`, `Application/` + `Interfaces/` include dirs; `Tests/` not
  built).
- `Core/Src/tony_ui.c` is now glue + renderer: **read inputs → `ui_event_t`**
  (encoder via `encoder_sm_update`, touch via `touch_is_pressed` /
  `touch_calibrate` / `touch_get_event`, PB0/PB1/PB2/PA8 debounced →
  `EVENT_GO_HOME` / `EVENT_BTN_PLAYPAUSE` / `EVENT_BTN_RECORD` /
  `EVENT_MUTE_TOGGLE`) → **`ui_process_event(&s_ui, ev)`** → **diff** of the
  resulting `ui_state_t` against the previous one → commands (Appendix C):
  playback transitions → `JACK_CMD_RECORD_START/STOP`, `PLAY_START(file_index)`,
  `PLAY_PAUSE/RESUME/STOP`, replay = stop + start; `active_effect` →
  `TOM_CMD_SET_EFFECT`; `volume` / `muted` → `TOM_CMD_SET_VOLUME` /
  `TOM_CMD_SET_MUTE`; entering the FILE screen → `STORAGE_REQ_LIST_FILES` on
  TonyTask's own reply queue; `EVENT_FILE_SELECT` → `PLAY_START(file_index)`
  and the Play screen → **`ui_get_display_info()` → render**.
- Renderer draws each screen's rows at the `MENU_ROW` positions that
  `touch_region` hit-tests (so taps land on what is drawn), three info lines
  and a status bar (battery %, MUTE, SD status text, active effect).
- Battery screen uses `BatteryMsg_t` (SOC when valid, otherwise
  `battery_mv_to_pct(mV)`); SD/file screen uses `StorageStatusMsg_t`
  (`sd_status`, `file_count`) and the file list reply. Jack's status is the
  authority on transport state: after 1 s without a UI command, an idle Jack
  resets the UI to `PLAYBACK_IDLE` (finished / refused / card pulled).
- Effect labels: `display_logic.c` gains `ui_effect_name()` with Tom's names
  (Autotune, Pitch Up, Pitch Down, Robot, Echo, Reverb); the Play/Effects
  lines use it; `Tests/test_ui.c` updated (`"Active: Autotune"`).
- The old `demo_mode_t` test screens are **deleted** (the state machine is the
  UI; the driver demos were standalone deliverables and are kept in the
  original project repository).
  Tony may restore them behind a hidden menu item if wanted.
- TFT / touch conversion: `Core/Src/tony_ports.c` is the **one** function each
  for SPI (`TonyPorts_SpiTransmit`, now called by all seven transmit sites in
  `ILI9341_STM32_Driver.c`) and ADC (`TonyPorts_AdcRead`). Each uses
  `HAL_SPI_Transmit_IT` + `TONY_SPI_DONE_FLAG` / `HAL_ADC_Start_IT` +
  `TONY_ADC_DONE_FLAG` when `NVIC_GetEnableIRQ` says the IRQ is on, and the
  blocking HAL call otherwise. The callbacks (`HAL_SPI_TxCpltCallback`,
  `HAL_ADC_ConvCpltCallback`) are in `main.c`. With this `.ioc` both paths
  are still blocking — see CubeMX steps.

## Task / queue / flag summary

| Task | Priority / stack | Owns | Blocks on | Produces |
|---|---|---|---|---|
| `TonyTask` | Low / 768 w | SPI1 (TFT), ADC1 (touch), input GPIOs | `osDelay(10)`; `batteryQueue` / `storageStatusQueue` / private `StorageReply_t` queue (0); `TONY_SPI_DONE_FLAG` / `TONY_ADC_DONE_FLAG` (once IRQs are enabled) | `jackCmdQueue`, `tomCmdQueue` + `TOM_CMD_FLAG`, `storageReqQueue` |

Flags: `TONY_SPI_DONE_FLAG 0x1`, `TONY_ADC_DONE_FLAG 0x2`.

## Board-test checklist (not verifiable without hardware)

- [ ] Boot: splash, then `[TONY] task up (SPI1 blocking, ADC1 blocking)` and the HOME screen with six rows; status bar shows `BAT`, `SD:` text from Jack.
- [ ] Encoder rotate on HOME moves the highlight; push enters the highlighted screen; PB0 returns home from any screen.
- [ ] Touch: tap each HOME row → matching screen (`[TONY] tap x,y` logged; if X is mirrored on the PCB, remove the inversion in `touch_tap`). Verify the calibration constants in `ui_types.h` against this panel.
- [ ] PB2 from HOME → Record screen, `[JACK] Recording …`; PB2 again → stop; the status bar `SD:` text follows Jack (`Recording` → `Ready`).
- [ ] PB1 → Play screen + `PLAY_START`; PB1 pause / resume; tap Stop / Replay.
- [ ] FILE screen: list requested (`[TONY] file list: N file(s)`), encoder scrolls, `> RECnn.WAV` shown, Select plays it.
- [ ] EFFECTS screen: rows show Tom's names, encoder scrolls, tap/push toggles; `[TOM] Effect: …` follows; status bar `FX:` updates.
- [ ] VOLUME screen: encoder ±1 (`[TOM] Volume …`), push = mute (`[TOM] Muted`) on every screen.
- [ ] BATTERY screen shows `Battery: N%` / `Voltage: N mV` from `batteryQueue`.
- [ ] Playback that ends by itself returns the UI to Stopped within ~1 s (reconciliation).
- [ ] `d` shows `TonyTask stack free` > 200 B while drawing the EFFECTS screen (largest render).
- [ ] After the CubeMX step: boot line says `SPI1 IT+flag, ADC1 IT+flag` and the screens still draw correctly.

## CubeMX steps still needed (this member)

1. **SPI1 global interrupt**: System Core → NVIC → enable `SPI1 global
   interrupt`, preemption priority **6**, "Uses FreeRTOS functions". Then
   `TonyPorts_SpiTransmit` switches to `HAL_SPI_Transmit_IT` + flag at runtime.
   (Next step, optional: DMA → add a stream for `SPI1_TX`, then use
   `HAL_SPI_Transmit_DMA` in the same function.)
2. **ADC global interrupt**: NVIC → enable `ADC1 global interrupt`, priority
   **6**, "Uses FreeRTOS functions". Then `TonyPorts_AdcRead` switches to
   `HAL_ADC_Start_IT` + flag at runtime.
3. No pin changes.

## Remaining blocking calls owned by Tony

| Where | Call | Why |
|---|---|---|
| `tony_ports.c` `TonyPorts_SpiTransmit` fallback | `HAL_SPI_Transmit(…, 50 ms)` | until CubeMX step 1 (sole SPI1 owner) |
| `tony_ports.c` `TonyPorts_AdcRead` fallback | `HAL_ADC_PollForConversion(…, 10)` | until CubeMX step 2 (sole ADC1 owner) |
| `ILI9341_STM32_Driver.c` `ILI9341_Init/Reset` | `HAL_Delay` | pre-kernel only (called from `Tony_Init`) |

`osDelay(2)` panel-settle waits in the touch sampling are RTOS delays, not
blocking calls.

## Host tests

```
cmake -S PD5b_Tony -B PD5b_Tony/build/host -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build PD5b_Tony/build/host
ctest --test-dir PD5b_Tony/build/host --output-on-failure
```
Result on this branch: 1/1 passed (`test_ui`, 100+ cases).
CI: new `.github/workflows/pd5b-tony-tests.yml` (path-scoped to `PD5b_Tony/**`).

Firmware: `cmake --preset Debug && cmake --build --preset Debug` → clean,
text 159 724 / data 348 / bss 90 824.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
