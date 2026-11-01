# Prototype Design 5b — Host-Side Unit Tests
## Jack McMillan — Audio Input and Memory Subsystem (SD storage logic)

## Purpose
This project contains hardware-independent application code for the SD
storage side of the Audio Input and Memory subsystem, plus automated Unity
unit tests. The tests run on the host computer (Mac/Windows) and do not
require an STM32 board or a real SD card — all storage access is replaced
with a programmable fake in `Tests/Doubles/`.

## Modules (`Application/`)

| Module | Responsibility | Subsystem requirements addressed |
|---|---|---|
| `wav_storage.c/.h` | Build canonical 44-byte PCM WAV headers; parse any RIFF/WAVE PCM header by walking its chunks (data offset, clamped data size); compute recording duration for the UI | AR1, AR2, CR3, 1.3, 1.7, 1.11 |
| `recording_session.c/.h` | Record-side state machine: tracks bytes written, reacts to slow/failed/full writes | 1.4, 1.5, 1.8, 1.9, CR1 |
| `playback_session.c/.h` | Playback-side state machine: reads PCM chunks, explicit/repeatable EOF, reacts to read failures | 1.6, 1.7, 1.11, CR2 |
| `file_index.c/.h` | Streams the card root and gives windows of 16 names around a list position or a name, plus the total (every playable `.WAV`, no limit) — backs the touchscreen "FILE (SD)" screen | 1.6, IR1, CR2 |
| `storage_status.c/.h` | Card presence/mount/fault state machine; produces the display-ready snapshot the UI renders | 1.1, 1.2, 1.8, IR1, IR6 |
| `fx_store.c/.h` | Effect-file paths (`FX/ECHO/REC05.WAV`), the names DELETE accepts, the 68-byte effect-file header and the tag that says what it was made from | R1, R3 |
| `fx_queue.c/.h` | Which effect file to make next: the selected effect first, then the cheapest | R1 |
| `fx_pipe.c/.h` | The sample counts of one effect file being made (read, sent, back, written) | R1 |
| `fx_job.c/.h` | The effect-file job engine: makes one file in small steps, checks old ones, never blocks on the render task | R1 |
| `play_plan.c/.h` | What PLAY does: the ready effect file, Preparing, or the original; where a switch carries on | R1 |

None of these files include `stm32f4xx_hal.h` or call any `HAL_*` function.
They only call the functions declared in `Interfaces/storage_port.h`
(chunks, the root iterator, files by path).

## Project structure
- `Application/`  — application modules under test (see table above)
- `Interfaces/`   — `storage_port.h`, the abstract contract to real/fake storage
- `Tests/`        — Unity tests, one executable per `test_*.c`
- `Tests/Doubles/` — `storage_port_fake.c/.h`, the programmable fake storage used by every test
- `Tests/Unity/`  — vendored Unity test framework (downloaded from the official ThrowTheSwitch/Unity repository, unmodified)

## macOS prerequisites
- Apple Command Line Tools / Clang
- CMake >= 3.23
- Ninja

Configure, build and test:

    cmake --preset host-macos
    cmake --build --preset host-macos
    ctest --preset host-macos

## Windows prerequisites
- MSYS2 UCRT64 GCC
- MSYS2 UCRT64 CMake
- Ninja

MSYS2 is installed at `C:\msys64`.

Configure, build and test:

    cmake --preset host-windows
    cmake --build --preset host-windows
    ctest --preset host-windows

## Expected result
CTest reports **10 tests passed** — one executable per module. Each executable
contains many individual Unity test cases; **359 in total**:

| CTest entry | Unity test cases |
|---|---|
| `test_wav_storage` | 62 |
| `test_recording_session` | 31 |
| `test_playback_session` | 31 |
| `test_file_index` | 63 |
| `test_storage_status` | 48 |
| `test_fx_store` | 22 |
| `test_fx_queue` | 15 |
| `test_fx_pipe` | 12 |
| `test_fx_job` | 51 |
| `test_play_plan` | 24 |
| **Total** | **359** |

Note that CTest counts executables, not individual test cases. To see the
individual cases, run an executable directly, e.g.
`./build/host-macos/test_storage_status`, or use the VS Code Test Explorer.

Verified: clean build with zero errors and zero warnings under
`-Wall -Wextra -Wpedantic`, and all 359 cases pass.

## Verifying this submission yourself
See `VERIFICATION.md` for a rubric-by-rubric traceability matrix, a
function-coverage checklist, and copy-paste commands that check each claim
(HAL independence, input validation, no hidden global state, test counts).

## Interface to the UI subsystem
See `INTERFACE_CONTRACT.md` for exactly what this subsystem provides to the
touchscreen UI: card status text, enable/disable flags, capacity percentage,
file list and file selection.

## Hardware independence
The host test build does not compile STM32 HAL, startup, linker or
peripheral code. Storage hardware is entirely replaced by
`Tests/Doubles/storage_port_fake.c`, which implements the same
`storage_port.h` interface as the real SDIO/FatFs adapter in the firmware,
`Core/Src/jack_ports.c`.

## What is NOT tested here
Actual SDIO/FatFs calls, DMA, and physical card behaviour cannot be
verified by host unit tests — those require the real board and are
confirmed by on-target integration testing in Semester 2.

## Acknowledgements
- Course-supplied assessment documents: Prototype Design 5 Instructions and
  Rubric; PD5 Host-Side Testing Guide (CMakeLists.txt, CMakePresets.json,
  and vendoring approach adapted from the guide's Appendix A/B, used as
  supplied).
- Unity test framework: ThrowTheSwitch/Unity,
  https://github.com/ThrowTheSwitch/Unity (MIT licence), vendored
  unmodified into `Tests/Unity/`.
- Generative AI (Claude): used to help design the module boundaries,
  write the application logic, test doubles and Unity test cases, and to
  build/compile/run the test suite to confirm it passes before submission.
  All logic was reviewed against the subsystem requirements in Prototype
  Design 1/2 before inclusion.
