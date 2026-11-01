# PD6-Brendon: Battery monitoring & USB-C under FreeRTOS

> A record of a pull request in the original project repository (base
> branch `rtos-integration`), kept as it was written.

**Changed since this PR** (the final firmware): the serial console,
LogTask and the solo builds were removed on 1 Nov 2026. So:
- `console_router` and `test_console_router` are gone (5 host test
  suites now). BrendonTask paces its loop with `osDelay(100 ms)` instead of
  the `consoleRxQueue` wait and sends no `StorageReq_t`;
  `Brendon_Init(&hi2c1)` takes no UART.
- The checklist's log lines and menu keys are now checks on the TFT:
  debug page 3 (gauge found, I2C1 mode, charger, PG, VBUS, USB, MSC),
  page 5 (`Battery LOW 25%`: the beep and `low yes`), page 1 (drops,
  BrendonTask's stack, red below 200 B) and the footer (`SD USB`).
  `docs/RTOS_ARCHITECTURE.md` section 6 has the current board checks.

Brings the host-tested `PD5b_Brendon/Application`
logic into the firmware behind **BrendonTask** and deletes the duplicated
`Core/` copies.

## What the branch does

- Compiles `battery_monitor`, `bq34z100` (PD5b), `charger_status`,
  `console_router`, `msc_store` into the firmware (root `CMakeLists.txt`;
  `Tests/` and `Tools/` are not built).
- `Core/Src/brendon_ports.c` implements `Interfaces/{bq_bus,charger_pins,msc_sd_ops}.h`
  with HAL calls **from inside BrendonTask only** (the MSC block ops run in the
  OTG_FS ISR by USB-class design and make no `os*` calls).
- `Core/Src/brendon_battery_usb.c` is RTOS glue: `BatMon_Tick` drives the 1 Hz
  `BatteryMsg_t` publish and the hysteresis alarms drive `faultBeepSem`;
  `Charger_Tick` at 10 Hz; `Router_HandleChar` with a handler table whose
  handlers **put queue messages** (`JackCmd_t`, `TomCmd_t`, `StorageReq_t`) or
  log — never call another subsystem; USB watch → `JACK_CMD_ENTER_MSC/EXIT_MSC`
  with a reply queue; `MSC_Poll` pumped only while the card is handed over.
- `USB_DEVICE/App/usbd_storage_if.c` routes the SCSI callbacks through
  `msc_store` (bounded post-I/O wait instead of the old infinite spin) behind
  the `g_msc_media_ready` hand-over flag.
- Duplicates resolved: `Core/Src/bq34z100.c` + `Core/Inc/bq34z100.h` deleted;
  `Battery_Snapshot_t` comes from `battery_monitor.h` only.

## Task / queue / flag summary

| Task | Priority / stack | Owns | Blocks on | Produces |
|---|---|---|---|---|
| `BrendonTask` | BelowNormal / 768 w | I2C1 (gauge), charger GPIOs, USB device state, SD bring-up while in MSC mode | `consoleRxQueue` (≤ 100 ms); `BRENDON_I2C_DONE/ERR_FLAG` (100 ms) per transfer | `batteryQueue` (newest wins), `jackCmdQueue`, `tomCmdQueue` + `TOM_CMD_FLAG`, `storageReqQueue` + private reply queue, `faultBeepSem` |

Flags: `BRENDON_I2C_DONE_FLAG 0x1`, `BRENDON_I2C_ERR_FLAG 0x2` (set by
`HAL_I2C_MemRxCplt/MemTxCplt/ErrorCallback` in `main.c`).

## Board-test checklist (not verifiable without hardware)

- [ ] Boot prints `[BRENDON] BQ34Z100 detected on I2C1` and `[BRENDON] task up (I2C1 blocking fallback)` (until the CubeMX step below is done).
- [ ] Menu `1` shows a plausible snapshot and `I2C1 transfers : blocking (...)`; after the CubeMX change it must say `interrupt + flag`.
- [ ] Tony's status line updates once a second from `batteryQueue`.
- [ ] Discharge (or fake SOC via a bench supply) below 30 % → `[BRENDON] LOW battery` once and a 200 ms beep; rises above 32 % before it can re-trigger (hysteresis).
- [ ] Menu `3` decodes PG/STAT/ALERT/VBUS; on the Nucleo PB8/PB4 float and PA9 is VBUS, so expect `NO INPUT` unless jumpered.
- [ ] Menu `6` stops on any key without hanging the console.
- [ ] USB-C to a PC: `[BRENDON] USB host attached` → `[JACK] USB-MSC mode` → `[BRENDON] MSC mode active`; drive appears; copy a file off; unplug → card re-mounted; `d` shows no dropped messages.
- [ ] `d` every 10 s: `BrendonTask stack free` stays > 200 B during menu `5` (deep call chain).

## CubeMX steps still needed (this member)

1. **I2C1 interrupts** (for the `_IT` + flag gauge path): System Core → NVIC →
   enable `I2C1 event interrupt` **and** `I2C1 error interrupt`, preemption
   priority **6**, tick "Uses FreeRTOS functions". Regenerate; `brendon_ports.c`
   detects the enabled IRQs at runtime and switches automatically.
2. ~~**Charger status pins**~~ done in the .ioc: PA0 `CHG_STAT`, PB8 `CHG_PG`,
   PB4 `GAUGE_ALERT` as GPIO_Input with internal pull-up. Optional later:
   EXTI on PA0/PB8/PB4 (falling+rising, priority ≥ 5) — `charger_status`
   already supports being fed from edges.
3. ~~**VBUS sensing**~~ done in the .ioc: `Activate_VBUS` unticked, PA9 is
   GPIO_Input `VBUS_SENSE`, and `MX_USB_DEVICE_Init` has "Do Not Generate
   Function Call" ticked — BrendonTask starts/stops the USB stack from
   VBUS_SENSE (AN4879 §2.6.1). Hardware: VBUS → 33 kΩ → PA9 → 82 kΩ → GND.

## Remaining blocking calls owned by Brendon

| Where | Call | Why |
|---|---|---|
| `brendon_ports.c` `bq_mem_read/write` fallback | `HAL_I2C_Mem_Read/Write(…, 100 ms)` | until CubeMX step 1 |
| `brendon_ports.c` `bq_is_ready` | `HAL_I2C_IsDeviceReady(3, 50 ms)` | bus-level probe, pre-kernel and menu retry only |
| `brendon_ports.c` `sd_read/write_blocks` | `HAL_SD_ReadBlocks/WriteBlocks(…, 1000)` in the OTG_FS ISR | USB MSC class design; wait bounded by `MSC_WAIT_MAX_POLLS` |
| `brendon_battery_usb.c` `Brendon_Init` | `HAL_Delay(50)` | pre-kernel |

## Host tests

```
cmake -S PD5b_Brendon -B PD5b_Brendon/build/host -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build PD5b_Brendon/build/host
ctest --test-dir PD5b_Brendon/build/host --output-on-failure
```
Result on this branch: 5/5 passed (`test_battery_monitor`, `test_bq34z100`,
`test_charger_status`, `test_console_router`, `test_msc_store`).
CI: `.github/workflows/pd5b-brendon-tests.yml` (path-scoped to `PD5b_Brendon/**`).

Firmware: `cmake --preset Debug && cmake --build --preset Debug` → clean,
text 161 744 / data 536 / bss 90 408.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
