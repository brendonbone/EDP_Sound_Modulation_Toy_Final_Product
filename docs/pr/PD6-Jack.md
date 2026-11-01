# PD6-Jack: Audio input & memory under FreeRTOS

> A record of a pull request in the original project repository (base
> branch `rtos-integration`), kept as it was written.

**Changed since this PR** (the final firmware): the serial console,
LogTask and the solo builds were removed on 1 Nov 2026. So:
- The checklist's log lines and keys are now checks on the TFT: debug
  page 2 (mounted, bus width, space, WAV count, last error), the footer
  (`REC m:ss`, `PLAYING <name>`, `NO SD`, `SD USB`), page 1 (`pcm`,
  underruns) and the fault screen (`File rejected`, `SD card removed`,
  `Card too slow`). Single slow writes are no longer shown.
- `_USE_EXPAND` off is now a build error (`#error` in `jack_ports.c`),
  not a log line.
- A microphone DMA that will not start gives the fault `Mic failed to
  start` / `Cannot record` and the recording ends at once.

Brings the host-tested `PD5b_Jack/Application`
logic into the firmware behind **JackStorageTask** (JackAudioTask is unchanged
from the base branch).

## What the branch does

- Compiles `recording_session`, `playback_session`, `wav_storage`,
  `file_index`, `storage_status` into the firmware (root `CMakeLists.txt`,
  `Application/` + `Interfaces/` include dirs; `Tests/` not built).
- `Core/Src/jack_ports.c` implements `Interfaces/storage_port.h`
  (`Storage_WriteChunk` — timed, `STORAGE_SLOW` above 150 ms, `STORAGE_FULL`
  on a short write; `Storage_ReadChunk`; `Storage_ListDir`) with FATFS
  **from inside JackStorageTask only**, plus the open/seek/expand/truncate/
  sync/mount/capacity helpers. It is now the only file that calls `f_*`.
- Recordings become `RECnn.WAV` (32 kHz mono 16-bit): header first with the
  planned size, `f_expand` pre-allocation, `recording_session` on every 32 KB
  flush (write-fail / full / 3 consecutive slow writes → safe stop and
  `faultBeepSem`), real `data_size` written back at the end.
- Playback: `wav_parse_header` on the first 44 bytes (mono 16-bit checked;
  bad/truncated header → rejected, fault reported), `playback_session` drives
  the `audioBlockPool` producer; EOF → `TOM_CMD_STOP` after a 50 ms tail.
- `file_index_list()` (.WAV only) answers `STORAGE_REQ_LIST_FILES`;
  `storage_status_get_ui_view()` feeds `StorageStatusMsg_t` (`status_text`,
  `record_enabled`, `play_enabled`, `percent_used` from `f_getfree`,
  `file_count`).
- Name clash resolved: `playback_state_t` → `jack_playback_state_t`
  (`JACK_PLAYBACK_IDLE/PLAYING/EOF/FAULT`) in `playback_session.h/.c`,
  `Tests/test_playback_session.c` and `VERIFICATION.md`, so Tony's
  `ui_types.h` keeps its `playback_state_t` unchanged.
- USB-MSC hand-over (eject-to-use) is complete end-to-end with Brendon's task:
  `ENTER_MSC` finishes any recording, unmounts, sets `g_msc_media_ready`;
  `EXIT_MSC` re-mounts and rescans.

## Task / queue / flag summary

| Task | Priority / stack | Owns | Blocks on | Produces |
|---|---|---|---|---|
| `JackAudioTask` | High / 768 w | I2S2 RX DMA | `JACK_AUDIO_HALF/FULL/CMD_FLAG` | `pcmFrameQueue` (timeout 0) |
| `JackStorageTask` | Normal / 1024 w | SDIO + FATFS (via `jack_ports.c`), SD_DETECT | `jackCmdQueue` (0/2/100 ms), `pcmFrameQueue` (5 ms) | `audioBlockPool`/`audioBlockQueue` (20 ms), `tomCmdQueue` + `TOM_CMD_FLAG`, `storageStatusQueue` (newest wins), `StorageReply_t`/`JackReply_t` on reply queues, `faultBeepSem` |

## Capture-rate decision (with Tom)

**Kept at 32 kHz** (PDM2PCM decimation 64 from I2S2 at 64 kHz "audio
frequency" = 2.048 MHz PDM clock). The WAV header says 32000 and Tom's task
keeps the 2:3 linear-interpolation upsampler to the 48 kHz DAC/effects.
Alternative (CubeMX, not done): I2S2 `AudioFreq` = 96 kHz custom (3.072 MHz
PDM clock) + `PDM_FILTER_DEC_FACTOR_64` → 48 kHz PCM; then `PcmFrame_t`
becomes 48 samples per 1 ms half, `pcmFrameQueue` grows to 200 x 96 B =
19.2 KB, Tom's adapter becomes 1:1 and `TOM_HALF_FRAMES` 192 = 3 blocks.

## Board-test checklist (not verifiable without hardware)

- [ ] Boot: `[JACK] SD mounted`, `[JACK] card N MB, N MB free (N% used, f_getfree N ms)`, `[JACK] N WAV file(s) on card`.
- [ ] `r` (or PB2): `[JACK] Recording 10 s to REC00.WAV @ 32000 Hz mono 16-bit`; after 10 s the summary shows `Frames saved: 10000 / 10000`, `Frames dropped ... 0`, `slow writes: 0`; the file opens in Audacity as 32 kHz mono.
- [ ] Second recording becomes `REC01.WAV`; `d` shows `pcm=0`.
- [ ] `p` (or PB1): `[JACK] Playing REC00.WAV (640000 bytes PCM, 10000 ms)` → audio → `[JACK] Playback finished (5000 blocks, 640000 bytes)`; `d` shows `underruns` ≈ 0.
- [ ] Pause/resume/stop from PB1/PB0; Tony's file screen lists the WAVs (PD6-Tony).
- [ ] Copy a non-WAV or truncated file onto the card: it is not listed / is rejected with `bad or truncated WAV header`.
- [ ] Pull the card mid-recording: `[JACK] SD card removed`, 200 ms beep, `Recording ABORTED`, status `No Card`; re-insert → `SD mounted`, rescan.
- [ ] Slow-card test (old class-4 card): `slow writes` counts; three in a row stops the recording safely with `write fault 3`.
- [ ] **MSC manual test:** plug USB-C into a PC → `[BRENDON] USB host attached` → `[JACK] USB-MSC mode: card handed to host` → drive appears → copy `REC00.WAV` to the PC and play it (32 kHz mono) → safely eject → unplug → `[JACK] USB-MSC mode left: card re-mounted` → `p` plays it on the board again. While the drive is mounted, `r`/`p` must log `In USB mode — command refused`.

## CubeMX steps still needed (this member)

None for this branch as delivered. Only if the team moves to 48 kHz capture:
Connectivity → I2S2 → `Audio Frequency` = 96 kHz (custom) and keep
`PDM_FILTER_DEC_FACTOR_64`; then apply the size changes listed above.
`_USE_EXPAND` must stay Enabled (FATFS → Set Defines → `USE_EXPAND`); the
code guards it and logs if it is off.

## Remaining blocking calls owned by Jack

All FATFS `f_*` calls in `jack_ports.c` (synchronous by design, inside the
owner task; stalls absorbed by the 200-frame `pcmFrameQueue` and the 16-block
pool). `f_getfree` runs only at mount. No `HAL_Delay`, no blocking UART.

## Host tests

```
cmake -S PD5b_Jack -B PD5b_Jack/build/host -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build PD5b_Jack/build/host
ctest --test-dir PD5b_Jack/build/host --output-on-failure
```
Result on this branch: 5/5 passed (`test_file_index`, `test_playback_session`,
`test_recording_session`, `test_storage_status`, `test_wav_storage`).
CI: new `.github/workflows/pd5b-jack-tests.yml` (path-scoped to `PD5b_Jack/**`).

Firmware: `cmake --preset Debug && cmake --build --preset Debug` → clean,
text 163 364 / data 536 / bss 90 560.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
