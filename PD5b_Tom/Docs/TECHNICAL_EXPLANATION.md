# Technical Explanation

## Architecture

The DSP is divided into three production modules.

`audio_effects.c/.h` is the high-level effect manager. It owns persistent state,
validates the public interface, selects the active effect, manages effect
configuration and exposes diagnostics.

`pitch_detector.c/.h` estimates fundamental frequency independently of any
particular effect. The input is decimated 4:1 from 48 kHz to 12 kHz, accumulated
into a 512-sample analysis window, then processed using normalised
autocorrelation over lags corresponding to approximately 80–1000 Hz. A first
strong local peak is preferred and parabolic interpolation refines the lag.

`pitch_shifter.c/.h` performs a granular dual-read-head delay-line pitch shift.
Two read positions separated by half a grain are smoothly crossfaded. Fractional
sample positions are linearly interpolated. The read-head phase step changes
with pitch ratio and remains continuous through ratio 1.0.

All state is caller-owned and persistent. No application module allocates
memory dynamically and no HAL call appears in the application source.

Only one effect runs at a time, so `AudioEffectsState` keeps the five
per-effect states (autotune, pitch, robot, echo, reverb) in one union: only
the selected mode's member is in use, and Init, Reset and SetMode clear only
that member (writing another one would overwrite it). This keeps the state at
23 072 B with the 240 ms echo, which did not fit in the STM32's RAM as five
separate states.

On the STM32 the effects are no longer applied live. `FxRender/fx_render.c`
runs the same chain offline (the 2:3 upsampler, then `ProcessBlock` on 64 and
32 samples per 32 kHz block) to make one effect file per effect, and
`Crossover/crossover.c` splits the output between the tweeter (left, above
1600 Hz) and the subwoofer (right, below 1600 Hz).

## Pitch ratios

For a pitch change of `n` semitones, equal temperament gives:

```text
ratio = 2^(n/12)
```

Therefore +12 semitones is 2.0, -12 is 0.5, +7 is approximately 1.4983 and -7
is approximately 0.6674. The public API stores semitone offsets so a later UI
can scale pitch without changing the shifter implementation.

## Autotune

Autotune performs:

Since 30 Oct 2026 the Autotune is a **hard tune to C major pentatonic**
(C D E G A): speech comes out as a sing-song tune of a few notes.

```text
PCM block (the newest sound)                 the same block, 39 ms late
 -> pitch detector (estimate every 21.3 ms)   -> look-ahead delay (1888 samples)
 -> octave fold + straight line between the      |
    two estimates around the sound played now     |
 -> nearest note of the scale, 30-cent hysteresis |
 -> ratio = note / pitch (limited to +-2 semitones)
 -> granular pitch shifter in hard-tune mode: the ratio is used at once
 -> ~10 kHz de-click low-pass
```

Why the look-ahead: the detector analyses a 42.7 ms window, so its estimate
describes sound about 32 ms old. The old Autotune corrected the sound of now
with that old pitch (plus a 21 ms glide in the shifter). On speech, whose
pitch moves all the time, the correction then landed at random: measured on
synthetic speech, the old effect moved the pitch about 23 cents in random
directions and left it no nearer to a note. Effect files are made offline,
so the sound can simply be played 1888 samples (39.3 ms) late: by then the
pitch of each moment is known, from the two estimates on either side of it.

The scale lookup converts the frequency to a note number (MIDI-note space)
and picks the nearest note whose place in the octave is in the scale table
(`ae_autotune_scale`; all true = every semitone). A new note is taken only
when the voice is 30 cents closer to it than to the held note, so a voice
on the boundary does not flip between two notes. Without a reliable pitch
for 150 ms the ratio is released to 1 and the shifter fades to the dry
voice over 10 ms (no comb colouring in pauses). A final one-pole low-pass
is restricted to autotune to reduce wide-band transient pops.

Measured offline on synthetic voices through the real effect chain
(`FxRender`), share of voiced output within 10 cents of a note: adult speech
16 % -> 70 %, child speech 16 % -> 64 %, a sung phrase 22 % -> 88 %; no
clicks; render time about 1.06x the old Autotune. Not yet heard on the
board with the real microphone.

## Robot

Robot mode first narrows the speech spectrum with a first-order 450 Hz
high-pass and two cascaded 2.4 kHz low-pass stages. It then multiplies the
filtered speech by an 80 Hz oscillator containing a second-harmonic component.
This ring modulation adds unnatural sum/difference spectral components. The
oscillator is advanced recursively rather than calling `sinf()` for every
sample, with once-per-block normalisation to control floating-point drift.

## Echo

Echo uses an 11 520-sample circular delay. At 48 kHz this is exactly 240 ms. Each
output contains the dry sample plus 35% of the delayed sample, while 30% of the
delayed sample is fed back into the delay buffer to create decaying repeats.

## Reverb

The reverb uses a compact Schroeder-style structure: four different parallel
feedback comb delays produce a set of repeated reflections, their average is
fed through two all-pass filters to increase diffusion, and dry/wet paths are
mixed at the output. The different relatively-prime-ish delay lengths avoid a
single obvious echo period.

## Numerical choices

Processing uses single-precision `float`, matching the Cortex-M4F hardware FPU.
Samples enter/leave as signed 16-bit PCM. Arithmetic results that return to PCM
are saturated to the int16 range to avoid wraparound clipping errors.
