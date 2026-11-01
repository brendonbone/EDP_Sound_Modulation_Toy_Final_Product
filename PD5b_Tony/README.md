# Prototype Design 5b - Host-Side Unit Tests
**Tony — Display/UI Subsystem**
BJT2 Sound Modulating Toy

## Purpose
This project contains hardware-independent UI application code and
automated Unity unit tests: 508 tests in eight executables, `test_ui`
(282 tests across 18 groups, A-R), `test_debug_menu` (52 tests across
7 groups, A-G), `test_status_view` (68 tests across 10 groups, A-J),
`test_sd_popup` (34 tests across 5 groups, A-E), `test_touch_filter`
(19 tests across 4 groups, A-D), `test_file_window` (18 tests across
4 groups, A-D), `test_delete_confirm` (15 tests across 4 groups, A-D)
and `test_screen_saver` (20 tests across 3 groups, A-C).
The tests run on the host computer and do not require an STM32 board.

## Project structure
- `Application/` — UI state machine, touch calibration, touch press
  filter, hit testing, touch-to-event mapping, display logic, encoder
  state machine, hidden debug screen navigation, power header and
  status footer, no-SD pop-up, file list window, Delete confirm,
  screen saver
- `Interfaces/`  — interfaces to external/hardware functionality
- `Tests/`       — Unity tests (test_ui.c, test_debug_menu.c,
  test_sd_popup.c, test_status_view.c, test_touch_filter.c,
  test_file_window.c, test_delete_confirm.c, test_screen_saver.c)
- `Tests/Doubles/` — host replacements for hardware/dependencies
- `Tests/Unity/` — vendored Unity test framework (ThrowTheSwitch)

## Modules covered
| Module | File | Description |
|--------|------|-------------|
| 1 | ui_state_machine.c | Home-screen UI state machine |
| 2 | touch_calibration.c | ADC counts → pixel coordinates |
| 3 | touch_region.c | Rectangular hit testing |
| 4 | touch_region.c | Touch pixel → ui_event_t |
| 5 | display_logic.c | ui_state_t → display_info_t (incl. the Battery screen's six lines: charge, current now, 2-minute average current, voltage, hh:mm time to empty / to full, status (incl. Plugged in and Full); the Play screen's "Preparing 45%" and "Effect: None (n/a)"), which list rows a cursor move redraws, where a file is in a list of names, and the Record and Play screens' "Insert SD card" while the slot is empty |
| 6 | encoder_sm.c | Quadrature encoder decoder |
| 7 | debug_menu.c | Hidden debug screen: pages, encoder cursor, item actions, 2 s BACK-hold detector (the screen itself is `UI_SCREEN_DEBUG` in ui_state_machine.c, test_ui.c group P) |
| 8 | status_view.c | Power header and status footer (best-ui): battery tag, SoC, icon fill, time to empty / to full, SD chip (also SD BUSY while effect files are written), recording / preparing / playing line, effect (with its Preparing percentage or N/A) and MUTE, each with its colour and a fixed width; the 3 s notice with Jack's answer to REC, PLAY or Delete (why it did nothing, "DELETED REC05.WAV", "NO FX FOR THIS FILE") |
| 9 | touch_filter.c | Touch press filter: each sample is pressed, released or unsure; one press is one tap, a press ends only after 2 released samples, HELD drives the Volume hold-repeat |
| 10 | sd_popup.c | No-SD pop-up (best-ui): which steps start REC or PLAY, whether Jack's status lets the card take them (the reason when not, the same as the footer notice), the pop-up's words, its 3 s and what closes it (any input, a screen change; a tap on the box is used up), and a refused start (the UI put back where it was, the pop-up over that screen) |
| 11 | file_window.c | The 16 file names the UI holds while the user scrolls through every WAV on the card: the name at a list position (raw, as Jack sent it), when to ask Jack for the next window (near an edge with more beyond it), a window from an older list, and forgetting it all when the card goes |
| 12 | delete_confirm.c | The File screen's Delete row: the first tap arms it, a second tap 0.5 to 3 s later deletes the armed file (never another one), anything else or 3 s disarms it; the row label and the "Delete REC05.WAV?" prompt |
| 13 | screen_saver.c | The frog-tongue screen saver: opens after 30 s without input (not over a fault, debug or the pop-up, not while recording or preparing), any input closes it, and which rectangles to paint for the face and each 40 ms step of its 4 s tongue flick; the SD BUSY tag's corner |

## macOS prerequisites
- Apple Command Line Tools / Clang
- CMake >= 3.23 (`brew install cmake`)
- Ninja (`brew install ninja`)

Configure, build and test:
```
cmake --preset host-macos
cmake --build --preset host-macos
ctest --preset host-macos
```

## Windows prerequisites
- MSYS2 UCRT64 GCC
- MSYS2 UCRT64 CMake
- Ninja (installed via MSYS2)

MSYS2 must be installed at `C:\msys64`.

Configure, build and test:
```
cmake --preset host-windows
cmake --build --preset host-windows
ctest --preset host-windows
```

## Expected result
CTest should report:
```
100% tests passed, 0 tests failed out of 8
```
Running the test executables directly prints all individual results
(282 from test_ui, 52 from test_debug_menu, 34 from test_sd_popup,
68 from test_status_view, 19 from test_touch_filter, 18 from
test_file_window, 15 from test_delete_confirm, 20 from
test_screen_saver).

## Hardware independence
The host test build does not compile STM32 HAL, startup, linker or
peripheral code. All logic modules are pure C11 with no HAL calls.
No hardware doubles are required for this submission as all modules
are fully self-contained.

## Acknowledgements
- Unity test framework by ThrowTheSwitch (MIT licence).
  Source included in Tests/Unity/.
  https://github.com/ThrowTheSwitch/Unity

- Claude (Anthropic) was used to assist in generating the initial
  code structure, module design, and test cases for all six modules.
  All code has been reviewed and is understood by the author.
