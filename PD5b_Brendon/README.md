# ZEIT4230 Prototype Design 5b — Brendon Bone

**Subsystem:** Battery Monitoring and USB-C (Group BJT², Sound Modulating Toy)
**Submission:** application code with unit tests
**Layout:** team host-testing standard (`HOST_TEMPLATE_VERSION.txt` = `PD5_HOST_TEMPLATE_V1`)

## Purpose

This folder contains the remaining software functionality for my subsystem, refactored from the group demonstration firmware plus the new status-pin logic, organised as five hardware-independent modules with a fully automated Unity/CTest test suite. There are no STM32 HAL calls anywhere, no user interaction, and no network access during the build: it compiles and runs its complete test suite on any PC with CMake ≥ 3.23, Ninja and a C compiler.

| Module | Origin | Purpose |
|--------|--------|---------|
| `Application/bq34z100.c/.h` | refactor of my PD3 driver | Complete TI BQ34Z100-G1 fuel-gauge command set: telemetry reads, Control() subcommands, data-flash read/write with checksum commit |
| `Application/battery_monitor.c/.h` | refactor of `brendon_battery_usb.c` | 1 Hz battery snapshot cache; low/critical battery alarms with comparator-style hysteresis (Schmitt-trigger latching: assert <30% / <10%, release ≥32% / ≥12%) so SOC noise around a threshold cannot chatter the alarms — PD2 reqs 2.5, 2.19, 2.20 (the assert thresholds are unchanged; hysteresis only delays the release); speaker-lockout latch for the critical-battery mute (sets with critical <10%, releases at ≥32%, never released by a failed reading); the gauge's instant current `Current()` for `battery_time` (falls back to AverageCurrent if that read fails, flagged so `battery_time` leaves it out of its average) and its own time to empty / time to full, now shown only on the debug screen (65535 = no estimate); these three extra reads happen only after the five core reads succeed and never invalidate the snapshot; a timed SOC override for the debug screen's battery fault injection (only the SOC is replaced, so the real alarm and lockout code runs) |
| `Application/battery_time.c/.h` | **new** (best-ui; 1 Nov: 2 min, steps) | Time to empty / time to full for the header and the Battery screen: RemainingCapacity and FullChargeCapacity over a 2-minute average of the instant current (10 s until 1 Nov: music made the time jump by hours), restarted when the charger is plugged in or out (the two readings after the change are left out, and so is a reading where AverageCurrent stood in; an invalid reading keeps the average, 120 in a row restart it), so the time follows a plug or unplug within seconds instead of the gauge's minutes; the times come from a held current that only follows the average once it has moved 5 % (at least 10 mA); time to empty also while draining on the charger; draining / filling start at ±10 mA and stop at 0; rounded minutes, capped at 99:59; shown in steps of 1 / 5 / 15 min that move one step once the new time is one and a half steps away, or straight there from two and a half (a change of the current itself, such as the music stopping, still takes the 2 minutes) |
| `Application/charger_status.c/.h` | **new** | Decodes the three status pins added after the demo (PD3 §3.1.5): PA0 ← BQ25170 STAT (incl. 1 Hz blink = fault, detected from edge timing), PB8 ← BQ25170 PG, PB4 ← BQ34Z100 ALERT (latched), PA9 VBUS — PD2 req 2.11 |
| `Application/msc_store.c/.h` | refactor of `usbd_storage_if.c` USER CODE | USB-MSC ↔ SD backing store: thread-context bring-up (rate-limited, never disturbs a busy card), block read/write with bounded idle waits, capacity fallback for enumeration — PD2 reqs 2.14–2.16, 2.21 |

A sixth module, `console_router` (the serial test-menu dispatch), was
removed with the rest of the UART on 1 Nov 2026, together with its test
suite and mocks.

Deliberate improvements made during the refactor (each is unit-tested):
the post-I/O SD spin-waits are now **bounded** (`MSC_TIMEOUT` instead of a
potential infinite loop in the USB ISR), the bring-up rate limiter works
correctly for an attempt at tick 0, and battery readings are **plausibility
checked** (SOC ≤ 100%, 2.0–5.0 V window) before the low/critical alarms can
act on them.

## Project structure

```
PD5b_Brendon/
├── Application/              the five modules — my real assessed code (no HAL includes)
├── Interfaces/               injected hardware interface headers
│   ├── bq_bus.h                I²C bus for the gauge driver and battery monitor
│   ├── charger_pins.h          GPIO readers for the charger/gauge status pins
│   └── msc_sd_ops.h            SD card operations for the USB-MSC backing store
├── Tests/
│   ├── test_<module>.c        one Unity suite per module, each with its own main()
│   ├── Doubles/               test doubles (mocks) for the injected interfaces
│   └── Unity/                 vendored Unity test framework (MIT licence)
├── CMakeLists.txt             team-standard host build (auto-globs the dirs above)
├── CMakePresets.json          team-standard host-macos / host-windows presets
├── HOST_TEMPLATE_VERSION.txt  team layout marker (PD5_HOST_TEMPLATE_V1)
└── README.md                  this file
```

## Building and testing on macOS

Prerequisites: Xcode command-line tools (`/usr/bin/clang`), CMake ≥ 3.23 and
Ninja (e.g. `brew install cmake ninja`). From this folder:

```
cmake --preset host-macos
cmake --build --preset host-macos
ctest --preset host-macos
```

## Building and testing on Windows

Prerequisites: MSYS2 installed at `C:\msys64` with the UCRT64 toolchain
(`pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake
mingw-w64-ucrt-x86_64-ninja`) and `C:\msys64\ucrt64\bin` on the PATH. From
this folder, in plain PowerShell:

```
cmake --preset host-windows
cmake --build --preset host-windows
ctest --preset host-windows
```

## Hardware independence

Every module receives its hardware access as a small struct of function
pointers at init (dependency injection); the structs are defined under
`Interfaces/`:

- `BQ_Bus` (I²C: `mem_read`, `mem_write`, `is_ready`, `delay_ms`) for the
  gauge driver and battery monitor;
- `Charger_Pins` (four GPIO level readers) for the status-pin decoder;
- `MSC_SdOps` (card state/init/read/write/capacity) for the backing store.

In the unit tests these are test doubles (`Tests/Doubles/`). Hardware-specific
code is **replaced by mock code**, as the brief requires: the scripted
bus/pin/SD mocks stand in for the HAL. None of the five modules calls another
group member's code.

On the real board, `Core/Src/brendon_ports.c` wires the bus, pin and SD
structs to the HAL once. A simplified example:

```c
/* --- board-side adapter (integration, Semester 2 — not part of PD5b) --- */
static int hal_mem_read(void *ctx, uint8_t reg, uint8_t *d, uint16_t n) {
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)ctx, 0x55U << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, d, n, 100U) == HAL_OK ? 0 : -1;
}
static const BQ_Bus board_bus = { hal_mem_read, hal_mem_write,
                                  hal_is_ready, hal_delay, &hi2c1 };
```

so integration is a thin adapter file — none of the tested logic changes.

Multi-tasking and real-time operation are **out of scope** for this submission (addressed after the RTOS content later in semester); all modules are cooperative and re-entrancy-free by design.

## Expected result

5 CTest suites, 193 Unity cases, 100% pass:

```
100% tests passed, 0 tests failed out of 5
```

Each test executable can also be run directly (e.g.
`./build/host-windows/test_bq34z100`) and prints Unity's per-case results.

| Suite                 | Cases | What it covers |
|-----------------------|-------|----------------|
| `test_bq34z100`       | 65    | Every driver function: command codes, little-endian assembly, signed registers, Control() and data-flash sequences (order, delays, checksum), all failure paths |
| `test_battery_monitor`| 51    | 1 Hz cadence incl. tick wrap, partial-read invalidation + recovery, 30%/10% alarm assert boundaries, 32%/12% hysteresis release sweeps + dead-band anti-chatter + latch-across-invalid-poll scenarios, speaker lockout (enter <10%, hold to 31%, release at 32%, re-arm, unchanged by invalid polls, boot latch), time to empty / to full (values land in the snapshot, a failed TTE or TTF read = 65535 and still valid, no time reads after a core failure, invalid snapshot = 65535), instant current (read right after the core reads and before the times, a failed read falls back to AverageCurrent, is flagged and stays valid, 0 in an invalid snapshot), debug SOC override (only the SOC replaced, 25% → low, 8% → critical + lockout, 60% → released, immediate poll on set / clear / expiry, expiry after N s incl. tick wrap and the 65535 s maximum (~18 h), clear with 0 s, the latches go on from the injected SOC when it ends (after 8% the lockout holds until 32%), the plausibility check sees the injected SOC, never hides a gauge failure, refused without a gauge or above 100%), plausibility rejection, NULL safety |
| `test_battery_time`   | 35    | No time before 3 readings, the instant current (not AverageCurrent) is averaged, the 120-reading (2-minute) window and its rounding, every time rule (to empty, also while draining on the charger, to full, full while charging, charge complete, no capacity figure), the ±10 mA deadband on both sides and a time lasting until 0 mA, the held current, minute rounding, the 99:59 cap, extremes without overflow, an invalid reading keeps the average (120 in a row restart it), the board scenarios (unplugged after charging at +900 mA: TO FULL gone at once, TO EMPTY settled on the fifth publish and never 99:59; plugging in; a charge finishing), settling after a charger change, a failed instant read (AverageCurrent stands in and is never averaged: after an unplug TO EMPTY is never 99:59, a full window stays as it was), the shown times (steps of 1 / 5 / 15 min, one step at a time up and down once one and a half steps away, straight there from two and a half and never two moves for one time, the band edges, a steady countdown under an hour, 0 and unknown at once, a repeatable song of -135..-220 mA: the average changes over 50 times in 10 min, the shown time at most twice; a net -30 or +50 mA on the charger with a +/-50 mA wobble: at most 30 changes in 20 min, or 10 when the wobble holds for 2-8 s), NULL safety |
| `test_charger_status` | 21    | PG/STAT decode, 1 Hz fault-blink detection (nominal/jitter/too-fast/too-slow/recovery), ALERT latch, pin failures, full charge-cycle scenario |
| `test_msc_store`      | 21    | SD bring-up state machine (busy-protection, 500 ms rate limit, wide-bus fallback), bounded post-I/O waits, capacity fallback, parameter guards |

## Continuous integration - Experiment using generative AI

The GitHub Actions workflow **PD5b Brendon host tests**(`.github/workflows/pd5b-brendon-tests.yml` at the repository root) configures,builds and runs this entire test suite with CTest on an Ubuntu runner on everypush or pull request that touches `PD5b_Brendon/`. It is path-scoped so itnever triggers on other group members' work, and it uses a plain CMakeconfigure (not the team presets, which pin macOS/Windows compiler paths) —the CMakeLists is toolchain-agnostic, so the Linux runner's default GCC isused. A red run means a commit broke a test; the log shows the failing suitevia `--output-on-failure`.

## Acknowledgements

- **Unity test framework** (ThrowTheSwitch, MIT licence — see `Tests/Unity/LICENSE.txt`): vendored unmodified and used as the test runner/assertion library for all suites.
- **Texas Instruments BQ34Z100-R2/G1 documentation** (TRM SLUUA09, datasheet SLUSF37, EVM user's guide SLUU904B): source of the command codes, the block-data/data-flash access sequence and checksum rule implemented in `bq34z100.c` (carried over from my PD3 driver).
- **Texas Instruments BQ25170 datasheet**: STAT/PG open-drain behaviour and the 1 Hz fault-blink indication decoded by `charger_status.c`.
- **STMicroelectronics USB device middleware documentation**: the STORAGE_*_FS callback semantics that `msc_store.c` re-implements hardware-independently (from my PD3/PD4 work in `usbd_storage_if.c`).
- **Generative AI (Claude, Anthropic):** used, with my supervision, to refactor my existing demonstration code into these hardware-independent modules, to write the unit-test suites and mocks, and to set up the CMake build and this README; a further Claude Code session was used, again under my supervision, to restructure the project into the team's shared host-testing layout (this folder structure, the shared CMake presets, and the updated documentation). The subsystem design, the original demo firmware this is refactored from, and the behavioural decisions (thresholds, pin semantics, state-machine rules) are my own; I have reviewed the code. The course brief encourages tools such as CMock and generative AI for test generation and requires this acknowledgement.
