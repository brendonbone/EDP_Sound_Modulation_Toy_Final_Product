# Feedback Actioned

This revision actions the mark-improvement feedback from the PD5 rubric audit.

- Added detailed public API documentation for every application module and
  comment blocks for internal processing functions.
- Added `AUDIO_EFFECTS_STATUS_PROCESSING_ERROR` and stopped discarding
  `PitchShifter_SetRatio()` / `PitchShifter_ProcessBlock()` return values.
- Added a final autotune-only ~10 kHz one-pole de-click stage to further reduce
  wide-band transient pops without materially cutting the required speech band.
- Expanded and reorganised behavioural tests so invalid conditions are mostly
  isolated into one clear scenario per Unity test.
- Expanded detector/shifter unexpected-condition and reset coverage.
- Added hand-written PitchDetector/PitchShifter mocks and a second CTest target
  to verify dependency calls and error propagation.
- Increased the automated suite to 71 individual Unity scenarios.
- Added LLVM/AppleClang source coverage support and a macOS coverage script.
- Measured a validation baseline of 100% function, 97.90% line and 88.80%
  branch coverage for production application C files.
- Added CMake presets for macOS, Windows and Linux and clean-build scripts.
- Added VS Code CMake Tools recommendations/settings so CTest suites appear in
  the Testing view.
- Added an optional GitHub Actions workflow for automated Linux host tests.
- Added a full root README with build/test/demo/integration instructions.
- Added technical explanation, testing strategy, coverage evidence, rubric
  traceability, DWT profiling plan and explicit AI acknowledgement documents.
- Vendored the course-provided Unity core plus a small local fixture layer so
  the submitted host test project is self-contained.


## TD-PSOLA autotune research alternative

- Added `Application/ResearchAutotune/td_psola_autotune.c/.h`.
- Reused the tested autocorrelation `PitchDetector` to create a local F0 track.
- Added chromatic target smoothing/hysteresis and a ±3% correction bound.
- Replaced arbitrary delay-grain movement in the experimental path with pitch-synchronous Hann-windowed overlap-add.
- Added dry fallback for unvoiced regions.
- Added `autotune-soft`, `autotune`, `autotune-hard`, and `autotune-legacy` audible comparison modes.
- Added `Tools/compare_autotune.sh`.
- Added five TD-PSOLA functional tests as a third CTest target.
- Added `Docs/AUTOTUNE_RESEARCH.md` with research basis, limitations, and the planned STM32 follow-on.
