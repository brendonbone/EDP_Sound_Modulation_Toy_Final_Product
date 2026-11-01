# Coverage Baseline

A validation run was performed with Clang 17 using LLVM source coverage after
all tests in this package passed.

| Production module | Functions | Lines | Branches |
|---|---:|---:|---:|
| `audio_effects.c` | 100.00% | 96.74% | 88.57% |
| `pitch_detector.c` | 100.00% | 100.00% | 86.76% |
| `pitch_shifter.c` | 100.00% | 100.00% | 92.86% |
| **Total** | **100.00%** | **97.90%** | **88.80%** |

This baseline proves that every production function was executed by the host
suite in the validation environment. It should be re-generated on macOS with:

```bash
./Tools/run_coverage_macos.sh
```

Coverage is evidence, not proof of correctness. The behavioural assertions and
unexpected-condition tests remain necessary even where a line is covered.

## TD-PSOLA research module

The earlier coverage figures in this document refer to the rubric-ready production modules before the experimental TD-PSOLA module was added. The experimental module has its own functional test executable (`td_psola_autotune_research`) but is not being presented as part of the previous production coverage baseline until the algorithm is selected for final integration.
