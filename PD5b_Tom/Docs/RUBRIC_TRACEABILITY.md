# PD5 Rubric Traceability

This document maps the submitted evidence to the published PD5 application-code
and unit-test criteria. It is a preparation aid, not a claim that a particular
mark is guaranteed.

## Application code evidence

| Rubric theme | Evidence in this package |
|---|---|
| High level of functionality | Six distinct playback effects including genuine chromatic pitch correction, configurable pitch shift, ring modulation, feedback delay and Schroeder-style reverb. |
| Independent learning / theory | Equal-tempered pitch maths, normalised autocorrelation, parabolic peak refinement, fractional-delay interpolation, granular crossfading, IIR filtering, ring modulation, comb/all-pass reverb. |
| Multiple appropriate modules | `audio_effects`, `pitch_detector`, `pitch_shifter` each have a distinct responsibility. |
| Complex code documented | Public APIs have parameter/return contracts; internal algorithms have comments; `TECHNICAL_EXPLANATION.md` explains the theory. |
| Appropriate persistent/static state | Caller-owned state structures; no heap allocation; history is explicit rather than hidden in HAL/global code. |
| Efficient/appropriate methods | 4:1 pitch-analysis decimation, recursive robot oscillator, block processing, no per-sample dynamic allocation. Target timing must still be proven using DWT. |
| Input validation | Public process/configuration APIs reject NULL, invalid block lengths, invalid modes, aliased buffers, out-of-range and non-finite parameters. |
| Error handling | Pitch-shifter return values are checked; internal failures are propagated as `AUDIO_EFFECTS_STATUS_PROCESSING_ERROR`. |
| Constants/enums/types/names | Effect modes/statuses are enums, constants use named macros, audio samples use `int16_t` and counters use fixed-width integer types where appropriate. |
| Reproducible compilation | Root CMake project and platform presets; clean-build scripts; no external application dependency download required. |

## Unit-test evidence

| Rubric theme | Evidence in this package |
|---|---|
| All tests pass | Clean host validation currently passes both CTest executables. |
| One clear scenario per test | Previous combined invalid-input tests were split into focused pointer/length/mode/range tests. |
| Normal operation | All six effects plus detector and shifter behaviours are tested. |
| Unexpected conditions | NULL pointers, zero/oversize lengths, aliasing, NaN/infinity, invalid modes/ratios/ranges and safe-null getters/reset/init are covered. |
| Every application function covered | LLVM baseline reports 100% production-function coverage. |
| Multi-step state tests | Autotune convergence/boundary transition, echo timing, reverb tail, reset and effect-mode history clearing. |
| Host-side testing | CMake + CTest presets for macOS/Windows/Linux. |
| Test fixtures | Tests are grouped using Unity-style fixture macros with group setup/teardown. |
| Doubles/mocks | Separate orchestration target replaces detector/shifter with hand-written mocks and verifies calls, parameters and error propagation. |
| Advanced infrastructure | VS Code CMake Tools/Test Explorer integration, LLVM coverage script, and optional GitHub Actions workflow. |

## Remaining actions before final submission

1. Run the package from a clean unzip on the actual Mac and save the test output.
2. Run `Tools/run_coverage_macos.sh` and retain the coverage summary/HTML report.
3. Audibly retest autotune v4 on the same real-voice WAV used previously; document whether pops are reduced.
4. Show the lecturer/tutor the host-side setup before submission if required by the course instructions.
5. Make sure the AI acknowledgement remains in the submitted documentation.
6. Later, when integrated on STM32, complete DWT profiling rather than claiming real-time performance from host results.
