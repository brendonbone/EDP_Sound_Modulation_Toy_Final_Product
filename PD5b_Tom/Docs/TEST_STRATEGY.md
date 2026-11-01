# Test Strategy

## Behavioural tests

`test_audio_effects` links the real `audio_effects.c`, `pitch_detector.c`, and
`pitch_shifter.c` implementations. Tests focus on externally observable DSP
behaviour rather than re-testing library primitives.

Key stateful scenarios include:

- sustained 445 Hz input is pulled toward A4/440 Hz;
- autotune remains bounded through a 445 -> 458 Hz chromatic boundary change;
- an echo impulse reappears at the exact 240 ms delay;
- reverb produces early reflections and a continuing impulse-response tail;
- changing effect mode clears stale echo history; a mode change writes
  only the new mode's part of the shared (union) state, and the new mode
  (pitch, robot) starts exactly like a fresh one, whatever the old effect
  left in that memory;
- public reset clears DSP history while preserving configuration;
- robot processing attenuates a 6 kHz tone substantially more than a 1 kHz
  tone, demonstrating the intended narrowed speech band.

Unexpected-condition tests separately exercise NULL state/input/output,
zero/oversized block lengths, aliased buffers, invalid modes, non-finite values,
and pitch configuration bounds. Splitting invalid cases into separate tests
makes failures diagnose one specific contract violation.

## Output chain

`test_output_chain` checks the three modules between a recording and the
speakers:

- the 2:3 rate adapter gives exactly the samples of TomTask's old
  upsampler, and its fit rule (how much of a 32 kHz block TomTask fits into
  the rest of a 4 ms half) fills the room without overflowing it and never
  answers 0;
- the effect-file renderer gives sample for sample what TomTask's old live
  chain would play with today's effects (the oracle,
  `Tests/OutputChain/live_chain_oracle.c`, copies its upsampler and effect
  loop; Echo is now 240 ms);
- the renderer sets the +7 / -7 semitone pitch presets itself, its output
  does not depend on the chunk size, a 48 kHz source skips the upsampler
  (also right after a 32 kHz file), and one renderer makes every file the
  same whatever it made before;
- the crossover is -6 dB on each side at 1600 Hz, L + R stays within 0.5 dB
  from 50 Hz to 20 kHz, the 10 Hz high-pass is -3 dB at 10 Hz, a channel
  never leaks into the other, the output is rounded to the nearest sample,
  and a silent half is bypassed exactly, only once the filters' own output
  has reached zero.

TomTask's own block bookkeeping in `Core/Src/tom_dac_output.c` (the 48 kHz
block held across halves, `s_hold`, and its release on STOP, SOURCE_TEST,
TEST_PATTERN and a rate change) uses the RTOS queue and pool, so it is not in
these host tests; it is checked on the board: start and stop playback 20
times, and it still starts every time.

## Test doubles / interaction tests

`test_audio_effects_orchestration` compiles `audio_effects.c` against
hand-written mock PitchDetector/PitchShifter implementations. This allows the
suite to verify things that are otherwise difficult to force with the real
modules:

- pitch-up sends the expected ratio and sample count to the shifter;
- autotune sends a detector-derived correction ratio to the shifter;
- a failed `PitchShifter_SetRatio()` is propagated as
  `AUDIO_EFFECTS_STATUS_PROCESSING_ERROR`;
- a failed `PitchShifter_ProcessBlock()` is likewise propagated.

This target demonstrates deliberate use of test doubles while keeping the
production design free from test-only hooks.

## Coverage

The LLVM coverage target excludes `Tests/` and `Tools/` from the summary and
reports only application-code coverage. Current validation baseline is 100%
functions, 97.90% lines and 88.80% branches.
