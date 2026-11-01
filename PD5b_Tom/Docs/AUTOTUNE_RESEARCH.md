# Autotune Alternative Research — TD-PSOLA

## Why the previous algorithm was replaced for audible comparison

The previous autotune used a continuously varying granular/delay-line pitch shifter. It passed synthetic pitch and discontinuity tests, but real voice listening still produced audible distortion and occasional pops. That is a valid reason to change algorithm rather than continuing to add smoothing stages around the same architecture.

The replacement in this research package is **time-domain pitch-synchronous overlap-add (TD-PSOLA)**. The key difference is that grains are aligned to estimated voice pitch periods before being overlapped. For monophonic voice this is much closer to the structure of the signal than using arbitrary moving delay read heads.

## Research basis

Moulines and Charpentier's PSOLA work describes pitch-synchronous overlap-add methods for speech prosody modification and distinguishes time-domain PSOLA as an efficient approach suitable for real-time synthesis systems:

- E. Moulines and F. Charpentier, “Pitch-synchronous waveform processing techniques for text-to-speech synthesis using diphones,” Speech Communication, vol. 9, no. 5–6, pp. 453–467, 1990. DOI: 10.1016/0167-6393(90)90021-Z
- https://www.sciencedirect.com/science/article/pii/016763939090021Z

A modern implementation survey of pitch-shifting methods describes PSOLA as particularly suited to **speech / mono voice**, using autocorrelation-derived pitch periods and pitch-synchronous grains to reduce grain-boundary artefacts compared with non-synchronous overlap methods:

- https://github.com/audiojs/shift

A separate open TD-PSOLA demonstration also notes that moderate pitch changes are the intended operating region, while unvoiced material and pitch-mark errors are important failure cases:

- https://github.com/sannawag/TD-PSOLA

For comparison, Bill Cox's Sonic library was also investigated. Sonic is an ANSI C streaming speech-processing library, uses pitch-period-based processing, and is explicitly designed for integration into voice applications. Its current implementation also uses a 12-point sinc FIR for pitch-rate interpolation. It is a strong possible future alternative, but this experiment does **not** vendor or copy Sonic source code; implementing our own TD-PSOLA keeps the research code small enough to understand and modify for PD5.

- https://github.com/waywardgeek/sonic

## Why TD-PSOLA fits this project

The project conditions are unusually favourable for this algorithm:

- input is mono voice rather than polyphonic music;
- autotune correction is to the nearest chromatic semitone, so the theoretical maximum correction is only half a semitone (about ±2.9% in frequency);
- playback-only processing means tens of milliseconds of analysis latency are acceptable;
- the existing project already has a tested autocorrelation pitch detector;
- time-domain processing avoids an FFT/phase-vocoder path and its associated phase-coherence issues.

The current research implementation therefore clamps full chromatic correction to approximately `0.970 .. 1.030` and provides 65%, 90% and 100% correction-strength variants for listening comparison.

## Processing structure

The experimental path is:

```text
48 kHz mono PCM
      |
      v
existing autocorrelation PitchDetector
      |
      v
smoothed F0 + confidence gate
      |
      v
nearest chromatic note + note hysteresis
      |
      v
local pitch-period track
      |
      v
find waveform peaks near predicted pitch marks
      |
      v
Hann-windowed pitch-synchronous grains
      |
      v
synthesis marks spaced at target pitch period
      |
      v
overlap-add + normalisation
      |
      v
corrected PCM
```

Unvoiced/silence regions are passed through dry instead of attempting pitch correction.

## Important implementation status

This version is deliberately an **audible host-side research prototype**. `TdPsola_ProcessBuffer()` processes a complete recording and allocates work buffers proportional to the WAV length. That is appropriate for selecting the algorithm by listening, but it is not yet the final STM32 streaming implementation.

The existing six-effect `AudioEffects_ProcessBlock()` remains intact, including the previous autotune, so the project still has all prior regression tests. In the host demo:

- `autotune` = new TD-PSOLA, 90% correction strength
- `autotune-soft` = new TD-PSOLA, 65%
- `autotune-hard` = new TD-PSOLA, 100%
- `autotune-legacy` = previous granular/delay-line implementation

If the new method sounds clearly better on real voice, the next engineering task is to convert the TD-PSOLA processor to a fixed-memory streaming/frame implementation for STM32 integration.

## Validation performed before packaging

The research tests verify:

- 445 Hz maps to the 440 Hz chromatic target;
- silence remains silence;
- a harmonic 445 Hz voice-like signal is shifted toward 440 Hz;
- steady-tone output does not contain a large sample discontinuity;
- invalid configuration is rejected.

A two-part synthetic voice-like signal was also checked numerically. A 445 Hz section measured approximately 440 Hz after TD-PSOLA, and a 458 Hz section measured approximately 466 Hz, demonstrating correction in both directions across a chromatic boundary.

This is not a substitute for listening to a real recorded voice. Real-voice audition is the purpose of the comparison script included in `Tools/compare_autotune.sh`.
