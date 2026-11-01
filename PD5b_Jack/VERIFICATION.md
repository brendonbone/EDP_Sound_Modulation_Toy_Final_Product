# PD5b Self-Check: Rubric and Requirements Traceability

This document is how you verify the submission yourself rather than taking
anyone's word for it. Work down each table and confirm the evidence exists.

---

## Part 1 — Application Code rubric (20% of course grade)

### Minimum for Pass

| Rubric criterion | How to verify | Where |
|---|---|---|
| Code compiles with no errors | Run the build; confirm zero errors AND zero warnings under `-Wall -Wextra -Wpedantic` | `cmake --build --preset host-macos` |
| Code addresses a minimal level of functionality | 10 modules covering record, playback, file listing, WAV format, card status and the effect files (made ahead, checked, played) | `Application/` |
| Code is indented correctly | Open any file; consistent 4-space indent, Allman braces throughout | all `.c`/`.h` |
| Written as a set of functions checkable with unit tests | Every public function is called directly by at least one test | see Part 3 |

### Meets Expectations (Credit)

| Rubric criterion | How to verify | Where |
|---|---|---|
| Significant level of functionality | 10 modules: the first five above, and five for the effect files (`fx_store`, `fx_queue`, `fx_pipe`, `fx_job`, `play_plan`), with the job phases as a state machine (`fxj_phase_t`) | `Application/` |
| Almost all correct, only minor bugs | 359 tests pass from a clean build | `ctest` output |
| Efficient and appropriate methods | Byte-wise little-endian I/O (endianness-safe); 64-bit intermediates prevent overflow | `wav_storage.c`, `storage_status.c` |
| **Appropriate design patterns, e.g. a state machine** | Four explicit state machines: `recording_state_t`, `jack_playback_state_t`, `card_state_t`, and the effect-file job's phases `fxj_phase_t` (transition table at the top of `fx_job.c`) | `recording_session.h`, `playback_session.h`, `storage_status.h`, `fx_job.h` |
| **All functions check validity of input data** | Every public function checks its pointer and range arguments before it uses them; the one exception is `fxj_want()`, which trusts its two pointers (the firmware passes Jack's own engine and name buffers, the tests their own) | grep for `== NULL` — see command in Part 4 |
| Appropriate use of functions to break up code | Static helpers `put_u32_le`, `get_u32_le`, `bits_per_sample_valid`, `has_wav_extension` | `wav_storage.c`, `file_index.c` |
| **All constants and enums defined appropriately** | `WAV_HEADER_SIZE`, `RECORDING_MAX_CONSECUTIVE_SLOW_WRITES`, `STORAGE_LOW_SPACE_WARN_PERCENT`, `FILE_INDEX_RAW_SCAN_CAPACITY`; all enums named | headers |
| **Standard integer types used** | `uint8_t`/`uint16_t`/`uint32_t`/`size_t`/`bool` throughout; no bare `int` in interfaces | all files |
| Variable and function names meaningful | e.g. `storage_status_recording_seconds_remaining`, `consecutive_slow_writes` | all files |
| Code formatting consistent | Same brace/indent style in every file | all files |
| **Each function has a comment block** | Every public function documented in its header with parameter meanings and return values | all `.h` files |
| **Error conditions from called functions are checked** | Every `Storage_*` return value is inspected and acted on; none discarded | `recording_session.c`, `playback_session.c`, `file_index.c` |

### Exceeds Expectations (D / HD)

| Rubric criterion | How to verify | Where |
|---|---|---|
| **Very high level of functionality** | Full record/playback/browse/status lifecycle, including UI-facing snapshot generation | all modules |
| **High level of independent contribution, not just adapting sources** | No code copied from ST examples; the port/adapter architecture and all logic are original to this subsystem | all `Application/` |
| **Implements techniques from other courses** | State machine design (ZEIT3223 Embedded Systems); PCM/WAV sample-rate and byte-rate arithmetic (ZEIT3215 Signals & Systems, ZEIT3221 Signal Processing) | `recording_session.c`, `wav_duration_ms()` |
| **Strong independent learning** | Dependency-inversion via `storage_port.h` so identical source compiles for host tests and ARM firmware | `Interfaces/storage_port.h` |
| **Well organised into multiple modules of appropriate size** | 10 modules. The largest, `fx_job.c` (the effect-file job engine, over 1 300 lines), is split into headed sections; every other module is about 400 lines or less | `wc -l Application/*.c` |
| **Complex parts documented** | Comments explain *why*: endianness safety, 64-bit overflow guards, clamping, why absence outranks fault | `wav_storage.c`, `storage_status.c` |
| **Appropriate use of local/static variables** | Session state passed in via pointer, never hidden globals; file-scope `static` data only in the fake and as constant tables (display strings, the WAV PCM sub-format ID, the effect-folder names, the cheapest-first effect order) | all modules |
| **No unnecessary variables** | No unused variables (verified: `-Wextra` produces zero warnings) | build output |

---

## Part 2 — Unit Tests rubric (10% of course grade)

### Minimum for Pass

| Rubric criterion | How to verify |
|---|---|
| Every function covered by unit tests | Part 3 table below: the 24 functions of the first five modules one by one, and the 72 added for the effect files, the File list and a start part-way in, by module |
| Most normal operating conditions covered | Happy-path test for every function |
| Minimal attempt at unexpected conditions | 60+ tests target invalid/error paths |
| Hardware-dependent code replaced by a test version | `Tests/Doubles/storage_port_fake.c` replaces all SD access |
| Tests pass with at most a few minor exceptions | 359/359 pass |

### Meets Expectations (Credit)

| Rubric criterion | How to verify |
|---|---|
| **All tests pass** | `ctest` reports 10/10 executables, 359/359 cases |
| **Each test covers one well-defined scenario** | One behaviour per test; names describe the behaviour, e.g. `test_card_removal_while_faulted_clears_the_fault` |
| **Good use of doubles/mocks to confirm correct calls** | Spy assertions on call counts prove *absence* of calls too: `test_writes_after_fault_are_rejected`, `test_zero_length_data_size_is_immediate_eof`, `test_fault_state_is_sticky_and_blocks_further_reads` |
| **All normal operation conditions covered** | Record, playback, list, select, status transitions all covered |
| **Good attempt at unexpected conditions** | NULL args, zero lengths, out-of-range indices, corrupt data, truncated buffers, removal mid-operation, overflow inputs |

### Exceeds Expectations (D / HD)

| Rubric criterion | How to verify |
|---|---|
| **Multi-step tests covering stateful code** | `test_ui_view_tracks_full_insert_use_remove_sequence` (5 steps), `test_list_then_select_matches_the_ui_interaction` (3 steps), `test_full_record_stop_record_cycle`, `test_full_playback_of_a_realistic_file`, `test_slow_write_counter_resets_after_a_good_write`, `test_remove_then_reinsert_requires_remount` |
| **Comprehensive coverage of unexpected conditions** | Boundary pairs tested on both sides (e.g. 89% vs 90% low-space; threshold-1 vs threshold slow writes; capacity exactly full vs truncated); overflow cases for 32 GB cards and long recordings |
| **Host side testing used to run tests more efficiently** | Entire suite is host-side; runs in ~0.01 s with no board |
| **Integration with advanced testing infrastructure** | CMake + Ninja + CTest, VS Code Test Explorer discovery, one executable per module |

---

## Part 3 — Function coverage checklist

Every public function and the tests that exercise it. Confirm each row.

| Module | Function | Covered by |
|---|---|---|
| wav_storage | `wav_build_header` | 15 tests incl. field-level byte checks, all bit depths, NULL/range rejection |
| wav_storage | `wav_parse_header` | 18 tests incl. every magic-string corruption, truncation, declared-size mismatch |
| wav_storage | `wav_duration_ms` | 6 tests incl. overflow and invalid-format cases |
| recording_session | `recording_start` | `test_start_initialises_*`, `test_restarting_after_a_fault_clears_the_fault` |
| recording_session | `recording_write_chunk` | 24 tests incl. OK/SLOW/FAIL/FULL/unexpected, thresholds, post-fault rejection |
| recording_session | `recording_stop` | `test_stop_while_recording_*`, `test_stop_while_faulted_*`, `test_stop_from_idle_*` |
| playback_session | `playback_start` | `test_start_initialises_state`, `test_restart_after_fault_*` |
| playback_session | `playback_read_chunk` | 22 tests incl. EOF, short reads, over-read prevention, fault stickiness |
| playback_session | `playback_stop` | `test_stop_returns_to_idle`, `test_stop_from_fault_*`, `test_stop_from_eof_*` |
| file_index | `file_index_list` | 24 tests incl. filtering, ordering, truncation, corruption, termination safety |
| file_index | `file_index_list_newest` | 14 tests incl. the newest 16 of 20, a 17th recording listed last, 0/43/44-byte files skipped (45 kept), Mac `_` files skipped, corruption |
| file_index | `file_index_select` | 9 tests incl. first/last/out-of-range/stale index/copy semantics |
| storage_status | `storage_status_init` | `test_init_starts_with_no_card`, `test_init_handles_null_safely` |
| storage_status | `storage_status_set_card_detected` | 6 tests incl. insert, remove, re-read, remove-then-reinsert |
| storage_status | `storage_status_set_mount_result` | 4 tests incl. no-card and remount-after-fault |
| storage_status | `storage_status_set_capacity` | `test_percent_used_*`, `test_free_space_exceeding_total_is_clamped` |
| storage_status | `storage_status_set_file_count` | `test_ui_view_ready_with_files`, lifecycle test |
| storage_status | `storage_status_report_fault` | 3 tests incl. STORAGE_OK ignored, no-card ignored |
| storage_status | `storage_status_clear_fault` | 2 tests |
| storage_status | `storage_status_percent_used` | 7 tests incl. divide-by-zero guard and 32 GB overflow |
| storage_status | `storage_status_can_record` | 5 tests |
| storage_status | `storage_status_can_play` | 4 tests |
| storage_status | `storage_status_get_ui_view` | 10 tests incl. every status string and the full lifecycle |
| storage_status | `storage_status_recording_seconds_remaining` | 6 tests incl. overflow and invalid formats |
| playback_session | `playback_start_at` | 4 `test_start_at_*` tests: an offset, the end, past the end, an odd byte |
| file_index | `fi_scan_begin`, `fi_scan_step`, `fi_scan_abort`, `fi_scan_done` | the `test_scan_*` tests incl. 300 files, a 16-entry budget, an unreadable root |
| fx_store | all 11 (`fx_folder` ... `fx_tag_matches`) | `test_fx_store.c` |
| fx_queue | all 11 (`fxq_init` ... `fxq_set_src`) | `test_fx_queue.c` |
| fx_pipe | all 10 (`fxp_begin` ... `fxp_percent`) | `test_fx_pipe.c` |
| fx_job | all 27 `fxj_*` functions | `test_fx_job.c` incl. an abort in every phase, stalls, lock slots |
| play_plan | all 8 (`pp_new_request` ... `pp_fx_offset`) | `test_play_plan.c` |

---

## Part 4 — Commands to verify the claims above

Run these yourself; do not take the tables on trust.

    # 1. Clean build, zero errors AND zero warnings
    rm -rf build
    cmake --preset host-macos
    cmake --build --preset host-macos 2>&1 | grep -i -E "error|warning"
    # Expect: no output

    # 2. All tests pass
    ctest --preset host-macos
    # Expect: 100% tests passed, 0 tests failed out of 10

    # 3. Count individual test cases (CTest reports executables, not cases)
    grep -c "RUN_TEST(" Tests/test_*.c
    # Expect: 63 + 51 + 12 + 15 + 22 + 24 + 31 + 31 + 48 + 62 = 359

    # 4. Prove NO HAL dependency anywhere in application code.
    #    Matches actual code (HAL_ calls, STM32/FatFs includes, FatFs API),
    #    not the words "HAL"/"FatFs" appearing in explanatory comments.
    grep -rnE "HAL_[A-Za-z]+\(|#include *[\"<]stm32|#include *[\"<]ff\.h|\bf_(open|close|read|write|mount|opendir|readdir)\(" \
      Application/ Interfaces/
    # Expect: no output

    # 5. Confirm input validation exists in every module
    grep -c "== NULL" Application/*.c
    # Expect: a non-zero count for every file

    # 6. Confirm no hidden mutable global state in application code
    grep -n "^static" Application/*.c
    # Expect: only constant tables and static helper functions, no mutable state

    # 7. Module sizes are reasonable
    wc -l Application/*.c Application/*.h

    # 8. Prove the tests can actually fail (guards against vacuous tests)
    #    Edit any expected value in a test, rebuild, run - confirm a FAIL
    #    is reported, then restore it.

---

## Part 5 — Subsystem requirements traceability

From Prototype Design 2. Confirms the rubric line "Functionality should be
consistent with your System Requirements".

| Req | Requirement (abbreviated) | Module | Representative test |
|---|---|---|---|
| 1.1 | Initialise interfaces before enabling record/playback | `storage_status` | `test_cannot_record_when_unmounted` |
| 1.2 | Confirm microSD state, storage availability, present errors if missing | `storage_status` | `test_ui_view_no_card`, `test_failed_mount_moves_to_fault_with_reason` |
| 1.3 | Configure capture 20–48 kHz, ≥16-bit | `wav_storage` | `test_build_header_supports_the_projects_design_sample_rates` |
| 1.4 | Deliver continuous sample data | `recording_session` | `test_long_recording_of_many_chunks_accumulates_correctly` |
| 1.5 | Buffer without missing samples | `recording_session` | `test_bytes_written_matches_bytes_seen_by_the_storage_layer` |
| 1.6 | Provide playback file data to the rest of the system | `playback_session` | `test_full_playback_of_a_realistic_file` |
| 1.7 | Reject unsupported/corrupted files without harming valid ones | `wav_storage`, `playback_session` | `test_parse_header_rejects_non_pcm_format` |
| 1.8 | Block recording and report fault if card missing/full/unavailable | `storage_status`, `recording_session` | `test_card_full_enters_fault`, `test_cannot_record_when_faulted` |
| 1.9 | Stop safely on insufficient write speed, preserve last valid data | `recording_session` | `test_slow_writes_reaching_threshold_stops_recording_safely`, `test_card_removed_midway_stops_and_preserves_earlier_data` |
| 1.11 | Report fault, reject file, return to main system | `playback_session` | `test_fault_records_the_underlying_reason` |
| CR1 | Sustain write rate without dropping audio | `recording_session` | slow-write threshold tests |
| CR2 | Sustain playback reads without underrun | `playback_session` | `test_short_read_from_storage_is_handled_without_overrunning` |
| CR3 | Support WAV/PCM | `wav_storage` | round-trip and format-rejection tests |
| IR1/IR6 | UI shows status, file list, faults | `storage_status`, `file_index` | `test_ui_view_tracks_full_insert_use_remove_sequence` |

---

## Part 6 — Honest limitations to state in your submission and viva

Being explicit about these is a strength, not a weakness. Say them before
you are asked.

1. Real SDIO/FatFs behaviour, DMA, and physical card timing are **not**
   verified here — host unit tests cannot prove them. They are confirmed
   by on-target integration testing in Semester 2.
2. `STORAGE_SLOW` is produced by the platform adapter, which must measure
   actual write duration against the CR1 budget. The threshold policy is
   tested here; the measurement itself is a Semester 2 platform concern.
3. `RECORDING_MAX_CONSECUTIVE_SLOW_WRITES = 3` is a design choice, not a
   derived figure. Be ready to justify it or change it after measuring
   real card behaviour.
4. The `.WAV` filter assumes 8.3-style names as produced by FatFs without
   long-filename support. If LFN is enabled in Semester 2, `file_entry_t.name`
   must grow and the filter tests should be extended.
5. Timing, real-time deadlines and multitasking are explicitly out of
   scope for PD5b per the assessment instructions.
