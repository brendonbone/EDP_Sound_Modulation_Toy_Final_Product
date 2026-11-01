# STM32 DWT Profiling Plan

Host tests establish functional behaviour, but they do not prove that the
STM32F411 meets the real-time deadline.

At 48 kHz with 64-sample blocks, one block represents:

```text
64 / 48000 = 1.333 ms
```

When the audio pipeline is integrated on target, use the Cortex-M4 DWT cycle
counter around `AudioEffects_ProcessBlock()` and record typical and worst-case
cycles for all six effects.

Example CMSIS-level setup (keep this in the STM32 adapter/integration code, not
inside `Application/AudioEffects`):

```c
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0U;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

uint32_t start = DWT->CYCCNT;
AudioEffectsStatus status = AudioEffects_ProcessBlock(&g_audio_effects,
                                                       input,
                                                       output,
                                                       64U);
uint32_t cycles = DWT->CYCCNT - start;
```

Convert cycles to time using the actual CPU clock:

```text
processing_time_us = cycles / CPU_Hz * 1,000,000
```

Create a table for the report/lab evidence:

| Effect | Typical cycles | Worst cycles | Worst time | <1.333 ms? |
|---|---:|---:|---:|:---:|
| Autotune | | | | |
| Pitch Up | | | | |
| Pitch Down | | | | |
| Robot | | | | |
| Echo | | | | |
| Reverb | | | | |

Autotune is expected to be the most expensive because autocorrelation analysis
is periodically performed across many lags. Optimise only if measured timing
shows insufficient margin. Possible later optimisations include reducing pitch
analysis update rate, replacing selected operations with CMSIS-DSP routines, or
changing block/analysis parameters. Do not move to fixed point solely on
assumption.
