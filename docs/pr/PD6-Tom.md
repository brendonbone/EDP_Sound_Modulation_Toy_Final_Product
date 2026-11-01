# PD6-Tom: Speaker & output (effects) under FreeRTOS

> A record of a pull request in the original project repository (base
> branch `rtos-integration`), kept as it was written.

**Changed since this PR** (the final firmware): the serial console,
LogTask and the solo builds were removed on 1 Nov 2026. So:
- The `[TOM]` log lines below are gone; the DWT profiling had already gone
  with the effect files (29-30 Oct 2026). `Tom_GetDeadlineMissed()` and
  `Tom_GetUnderruns()` now feed `App_RTOS_GetStats()`: debug page 1 shows
  `tomMissed`, underruns and Tom's stack.
- An I2S3 DMA that will not start gives the fault `Speaker DMA failed` /
  `No sound`.

Brings the host-tested `PD5b_Tom/Application/AudioEffects`
DSP into the firmware behind **TomTask**.

## What the branch does

- Compiles `audio_effects.c`, `pitch_detector.c`, `pitch_shifter.c` into the
  firmware (root `CMakeLists.txt`, `Application/AudioEffects` include dir).
  `Tests/`, `Tools/` and `ResearchAutotune/td_psola_autotune.c` (offline,
  whole-buffer, uses `calloc`) are **not** built.
- `AudioEffectsState s_fx` (26 060 B) is a static in `.bss`, never on the
  stack. RAM after the branch: 119 392 B used (91 %), 11 680 B free — it fits,
  so no reverb/echo size profile was added (`#if defined(STM32F411xE)` hook
  described in the docs if it is ever needed).
- `TOM_CMD_SET_EFFECT` maps `UiEffect_t` → `AudioEffectMode`
  (`UI_EFFECT_NONE` = bypass, `1..6` = AUTOTUNE, PITCH_UP, PITCH_DOWN, ROBOT,
  ECHO, REVERB — the table is documented in `app_rtos.h` and implemented in
  `tom_dac_output.c`). `TOM_CMD_SET_VOLUME/MUTE` → Q15 int16 scaling.
- Per 64-sample 32 kHz playback block: 2:3 upsample → 96 samples at 48 kHz →
  `AudioEffects_ProcessBlock(64)` + `(32)` → gain → L/R interleave, so the
  effects always run at their 48 kHz design rate.
- DWT cycle profiling: every `ProcessBlock` call is timed, normalised to 64
  samples, tracked per mode (max / EMA / calls / over-budget) and logged every
  10 s as `[TOM] fx <mode> calls=… max=… avg=… cycles/64 (budget 57600) over=…`.
  A mode over budget is warned once and stays selectable.
- Fault beep from `faultBeepSem` (200 ms 1 kHz, replaces the source);
  `Tom_GetDeadlineMissed()` / `Tom_GetUnderruns()` feed `App_RTOS_DumpStats`.
- Tom's GitHub workflow moved from `PD5b_Tom/.github/` (ignored by GitHub) to
  `.github/workflows/pd5b-tom-tests.yml`, path-scoped to `PD5b_Tom/**`,
  running his `host-linux` presets with `working-directory: PD5b_Tom`.

## Task / queue / flag summary

| Task | Priority / stack | Owns | Blocks on | Produces |
|---|---|---|---|---|
| `TomTask` | AboveNormal / 1024 w | I2S3 TX DMA (circular, never stopped), `AudioEffectsState` | `TOM_DAC_HALF/FULL_FLAG`, `TOM_CMD_FLAG`; `audioBlockQueue` (0); `faultBeepSem` (0) | filled DMA halves; `osMemoryPoolFree` of every consumed block |

Deadline: 4 ms per half = 288 000 cycles at the 72 MHz SYSCLK for 192 effect samples
(four `ProcessBlock` calls). Effect budget 57 600 cycles per 64-sample call
(60 % of the per-sample share); the rest is adapter + interleave + RTOS.

## Board-test checklist (not verifiable without hardware)

- [ ] Boot: `[TOM] task up (effects 26060 B static, budget 57600 cycles/64)`.
- [ ] Record then play with no effect: pass-through as on the base branch, `d` shows `tomMissed=0`, `underruns≈0`.
- [ ] Send `TOM_CMD_SET_EFFECT` 1..6 from Tony's Effects screen (PD6-Tony) or from a debugger: `[TOM] Effect: Autotune (UI 1)` …; audible change; **record the `[TOM] fx …` cycle numbers per mode into `docs/RTOS_ARCHITECTURE.md` §9**.
- [ ] Any `WARNING: … > budget` line: note which mode; if `tomMissed` also increments while it is selected, the mode is not real-time on this part (keep it selectable, document, do **not** raise the task priority — slide 35).
- [ ] Effect switch mid-playback is click-free (`AudioEffects_Reset` on switch).
- [ ] Pull the SD card mid-recording: fault beep audible for ~200 ms over silence.
- [ ] Volume ±5 from the encoder and mute toggle apply within one half (4 ms).

## CubeMX steps still needed (this member)

None. (If Jack moves capture to 48 kHz — see `docs/pr/PD6-Jack.md` — the
upsampler becomes 1:1 and a half then carries three 64-sample blocks; no
CubeMX change on Tom's side.)

## Remaining blocking calls owned by Tom

None. The DMA is started once with `HAL_I2S_Transmit_DMA`; all rendering is
in the task between flags.

## Host tests

```
cd PD5b_Tom
cmake --preset host-windows      # or host-linux / host-macos
cmake --build --preset host-windows
ctest --preset host-windows --output-on-failure
```
Result on this branch: 3/3 passed (`audio_effects_behaviour`,
`td_psola_autotune_research`, `audio_effects_orchestration`).
CI: `.github/workflows/pd5b-tom-tests.yml` (moved to the root).

Firmware: `cmake --preset Debug && cmake --build --preset Debug` → clean,
text 166 876 / data 536 / bss 117 320.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
