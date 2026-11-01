# TD-PSOLA Autotune Research Package — Read This First

This package is a **complete audible comparison build** based on the rubric-ready PD5 project. The existing effects and tests are preserved, but a second autotune architecture has been added for research and listening.

The new host-demo commands are:

```text
autotune         TD-PSOLA, 90% correction strength
autotune-soft    TD-PSOLA, 65% correction strength
autotune-hard    TD-PSOLA, 100% correction strength
autotune-legacy  previous granular/delay-line autotune
```

The TD-PSOLA version is intentionally **offline/host-side for this audition round**. It processes the complete WAV so that algorithm quality can be judged before spending time converting it into an STM32 fixed-memory streaming implementation. See `Docs/AUTOTUNE_RESEARCH.md` for the research basis and architecture.

## Fastest way to compare on macOS

From the project root:

```bash
cmake --preset host-macos
cmake --build --preset host-macos
ctest --preset host-macos --output-on-failure
./Tools/compare_autotune.sh AudioSamples/voice.wav
```

Then listen to the files in the order printed by the script. The most important comparison is **real voice: soft TD-PSOLA vs 90% TD-PSOLA vs hard TD-PSOLA vs legacy**.

---

# PD5 Audio Effects — Self-Contained Host-Testable Submission

This project contains hardware-independent C application code for six playback
DSP effects, automated Unity/CTest tests, hand-written test doubles, an audible
WAV demo, LLVM coverage support, and reproducible CMake presets.

The application code makes no STM32 HAL calls. It is designed to be integrated
later between a 48 kHz mono PCM playback source and the STM32 I2S/DMA output
adapter.

## Implemented effects

1. **Autotune** — normalised-autocorrelation pitch detection, chromatic note
   quantisation, smoothed correction, granular pitch shifting, hysteresis and a
   final de-click low-pass stage.
2. **Pitch Up** — default +7 semitones, adjustable from 0 to +12 semitones.
3. **Pitch Down** — default -7 semitones, adjustable from -12 to 0 semitones.
4. **Robot** — narrowed speech band plus harmonic ring modulation.
5. **Echo** — 240 ms feedback delay.
6. **Reverb** — four parallel comb filters followed by two all-pass diffusers.

## Required audio interface

- 48 kHz
- mono
- signed 16-bit PCM
- maximum block size: 64 samples
- input and output buffers must be distinct

The 64-sample block period is approximately 1.333 ms at 48 kHz. On the STM32
the effects no longer have to keep up with playback: since 29 Oct 2026 they
are made ahead of time into effect files (see "Integration interface" below).

## Project structure

```text
Application/AudioEffects/          Production, hardware-independent DSP
Application/RateAdapter/           2:3 upsampler, 32 kHz recordings -> 48 kHz
Application/FxRender/              The effect chain run offline: makes effect files
Application/Crossover/             1600 Hz speaker crossover (tweeter left, sub right)
Application/TunePlayer/            Startup sound and warning tunes
Tests/AudioEffects/                Behavioural Unity tests
Tests/OutputChain/                 Rate adapter, effect-file renderer and crossover tests
Tests/Mocks/                       Hand-written dependency test doubles
Tests/Unity/                       Self-contained Unity core + fixture layer
Tools/audio_effect_demo.c          Host WAV processor for audible checking
Tools/verify_clean_build.sh        Clean macOS/Linux verification
Tools/verify_clean_build.ps1       Clean Windows verification
Tools/run_coverage_macos.sh        LLVM line/function/branch coverage
Docs/                              Design, rubric and profiling notes
.vscode/                           CMake Tools recommendations/settings
../.github/workflows/pd5b-tom-tests.yml   CI host test workflow (repository root)
CMakeLists.txt
CMakePresets.json
```

## macOS: configure, build and test

Prerequisites: Xcode Command Line Tools, CMake >= 3.23 and Ninja.

```bash
cmake --preset host-macos
cmake --build --preset host-macos
ctest --preset host-macos --output-on-failure
```

For a clean reproducibility check:

```bash
./Tools/verify_clean_build.sh host-macos
```

## Windows: configure, build and test

The intended setup is MSYS2 UCRT64 with GCC, CMake and Ninja available in PATH.
From PowerShell or the VS Code terminal:

```powershell
cmake --preset host-windows
cmake --build --preset host-windows
ctest --preset host-windows --output-on-failure
```

Or:

```powershell
./Tools/verify_clean_build.ps1
```

## Automated tests

CTest exposes five automated test executables:

- `audio_effects_behaviour` — real detector/shifter/effects behavioural tests.
- `audio_effects_orchestration` — the high-level `audio_effects.c` module is
  compiled against hand-written PitchDetector/PitchShifter mocks to verify
  calls, arguments and error propagation.
- `tune_player` — the tune engine TomTask plays the startup sound and the
  warning tunes with (`Application/TunePlayer/tune_player.c`): tune lengths,
  note order and pitch, 5 ms fades, the 50 % amplitude limit and the
  one-slot queue rule.
- `td_psola_autotune_research` — the offline TD-PSOLA research prototype
  described at the top of this README.
- `output_chain` — the path between a recording and the speakers: the 2:3
  rate adapter and the effect-file renderer give sample for sample what
  TomTask's old live chain would play with today's effects (the oracle
  copies its upsampler and effect loop; Echo is now 240 ms), how much of a
  32 kHz block TomTask fits into a half, and the 1600 Hz crossover's gains,
  flat L + R sum, 10 Hz high-pass, silence bypass, rounding and clipping.

The current package contains **144 individual Unity test scenarios** across the
five executables. They cover normal operation, stateful/multi-step behaviour,
invalid pointers and lengths, configuration bounds, pitch quantisation,
autotune correction, discontinuity regression, robot frequency narrowing,
echo timing, reverb impulse response, reset behaviour, dependency calls and
called-function failure propagation.

Run only the main behavioural tests:

```bash
ctest --preset host-macos -R audio_effects_behaviour --output-on-failure
```

Run only the test-double suite:

```bash
ctest --preset host-macos -R audio_effects_orchestration --output-on-failure
```

## VS Code Test Explorer

Install the recommended `CMake Tools` and `C/C++` extensions when VS Code
prompts. Select the `host-macos` (or `host-windows`) CMake preset and configure
the project. CMake Tools exposes the registered CTest tests in VS Code's
Testing view, so the suites can be run without typing terminal commands.

CLI CTest remains the authoritative/reproducible method for marking.

## Code coverage on macOS

The project has an LLVM coverage preset and script. Run:

```bash
./Tools/run_coverage_macos.sh
```

The script:

1. configures/builds an instrumented AppleClang build;
2. runs both CTest executables;
3. merges `.profraw` data with `llvm-profdata`;
4. prints function/line/branch coverage for `Application/`; and
5. writes an HTML report to
   `build/coverage-macos/coverage-html/index.html`.

A Linux/Clang validation run of this package measured **100% function coverage,
97.90% line coverage and 88.80% branch coverage** for the three production C
modules. Re-run the script on the submission machine and retain the generated
summary as evidence; see `Docs/COVERAGE_BASELINE.md`.

## Audible host-side demo

The demo processes the exact production DSP code and writes another WAV. Input
must be 48 kHz mono 16-bit PCM.

```bash
./build/host-macos/audio_effect_demo \
    AudioSamples/voice.wav \
    AudioSamples/robot.wav \
    robot

afplay AudioSamples/robot.wav
```

Available names:

```text
autotune
pitch-up
pitch-down
robot
echo
reverb
```

The demo is verification tooling only and is not required on the STM32.

## Integration interface

On the STM32 the effects are made ahead of time, one effect file per effect
(for example `FX/ECHO/REC05.WAV`), so no effect runs while sound plays:

```text
making an effect file (FxRenderTask, the lowest priority, while nothing plays):
microSD WAV -> JackStorageTask -> FxRenderTask
    (FxRender_Process: 2:3 upsampler for a 32 kHz recording,
     then AudioEffects_ProcessBlock on 64 + 32 samples)
    -> JackStorageTask -> effect file on the card

playback (the original, or its effect file):
microSD WAV -> JackStorageTask -> TomTask
    (rate adapter for 32 kHz, crossover, speaker trim)
    -> I2S/DMA output -> MAX98357A / speakers (left tweeter, right subwoofer)
```

Allocate the large persistent state statically rather than on the stack:

```c
static AudioEffectsState g_audio_effects;

void Playback_Init(void)
{
    AudioEffects_Init(&g_audio_effects);
}
```

In the firmware the only `AudioEffectsState` is inside FxRenderTask's
`FxRender` (`Core/Src/tom_fx_render.c`); `FxRender_Begin` initialises it for
each file. The UI selects an effect, and Jack plays (or first has made) the
matching effect file.

## Known limitation / engineering status

Since 30 Oct 2026 the Autotune is a hard tune to C major pentatonic with a
39 ms look-ahead (see Docs/TECHNICAL_EXPLANATION.md): the old gentle version
was measured to leave speech practically unchanged. The ~10 kHz de-click
filter, grain crossfades and note hysteresis stay. Automated discontinuity
regression tests pass, but **real-voice audible re-testing is still required**;
it was tuned on synthetic voices, not the board microphone.

## Documentation

- `Docs/TECHNICAL_EXPLANATION.md` — module and algorithm explanation.
- `Docs/TEST_STRATEGY.md` — what each test category proves.
- `Docs/COVERAGE_BASELINE.md` — measured host coverage baseline.
- `Docs/RUBRIC_TRACEABILITY.md` — PD5 rubric-to-evidence mapping and remaining
  actions.
- `Docs/STM32_DWT_PROFILING.md` — future target-side timing measurement plan.
- `Docs/AI_ACKNOWLEDGEMENT.md` — explicit acknowledgement wording.

## Acknowledgement

Generative AI (OpenAI ChatGPT) was used to assist with the architecture and
initial implementation of the audio DSP modules, generation and refinement of
unit tests, debugging, host-build/coverage infrastructure, documentation and
iterative changes following audible evaluation. The student is responsible for
reviewing the submitted code, running and interpreting the tests, performing
audible validation, integrating it into the project, and being able to explain
and modify the implementation.

Unity test-framework core files were supplied through the ZEIT4230 course test
template. The application source itself does not depend on Unity.
