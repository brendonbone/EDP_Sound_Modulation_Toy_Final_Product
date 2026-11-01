# Storage → UI Interface Contract
## Jack (Audio Input and Memory) → Tony (Touchscreen UI)

This document defines exactly what the storage subsystem provides to the UI
subsystem. The principle is that **all storage interpretation happens on the
storage side**. The UI renders finished values and never inspects SD card
internals, filesystem state, or error codes.

---

## 1. Storage status (HOME SCREEN indicator, fault messages)

The UI calls one function and renders the returned struct directly.

```c
#include "storage_status.h"

storage_ui_view_t view;
if (storage_status_get_ui_view(&g_storage_status, &view) == STORAGE_OK)
{
    /* render view.* */
}
```

`storage_ui_view_t` fields, all final and ready to display:

| Field | Type | Meaning for the UI |
|---|---|---|
| `status_text` | `const char *` | Display verbatim. One of: `"No Card"`, `"Card Not Ready"`, `"Ready"`, `"Card Error"`, `"Card Full"` |
| `record_enabled` | `bool` | Grey out / disable the **Record** menu item when false |
| `play_enabled` | `bool` | Grey out / disable **Play** and **File (SD)** when false |
| `low_space_warning` | `bool` | Show a low-space icon or warning when true |
| `percent_used` | `uint8_t` | 0–100, ready for a capacity bar |
| `file_count` | `size_t` | Number of recordings; also drives the FILE (SD) list length |

The string pointers are static storage owned by this module and remain valid
for the lifetime of the program — the UI must not free or modify them.

**Guarantees:**
- `record_enabled` is true only when a card is inserted, mounted and fault-free.
- `play_enabled` additionally requires `file_count > 0`.
- When no card is present, capacity and file count are always reported as 0.
- Removing a card mid-operation immediately clears status, capacity and file count.

---

## 2. File list (FILE (SD) screen)

```c
#include "file_index.h"

file_entry_t files[MAX_DISPLAYED_FILES];
size_t count = 0U;
storage_result_t r = file_index_list(files, MAX_DISPLAYED_FILES, &count);
```

| Return | UI action |
|---|---|
| `STORAGE_OK` | Render `count` entries |
| `STORAGE_TRUNCATED` | Render `count` entries; more exist than fit — optionally indicate this |
| `STORAGE_CORRUPT` | Show a card error message; `count` is 0 |
| `STORAGE_INVALID_ARG` | Programming error in the UI call |

`file_entry_t` provides `name` (null-terminated, ≤12 chars) and `size_bytes`.
Only `.WAV` files are returned; system, hidden and non-audio files are already
filtered out. Listing order is stable, so the encoder scroll index is safe to
retain between redraws.

**To show a duration next to each file**, parse its header and call:

```c
uint32_t ms = wav_duration_ms(&header);   /* wav_storage.h */
```

---

## 3. File selection (encoder scroll + tap)

```c
file_entry_t selected;
if (file_index_select(files, count, encoder_index, &selected) == STORAGE_OK)
{
    /* selected.name is the file to play */
}
```

Returns `STORAGE_NOT_FOUND` if `encoder_index` is out of range. The UI does not
need to bounds-check the encoder itself — an over-scrolled or stale index is
rejected safely rather than reading out of bounds. Handling the rejection by
ignoring the tap is sufficient.

---

## 4. What the UI must NOT do

- Do not call FatFs, SDIO or HAL functions directly.
- Do not interpret `storage_result_t` codes to build its own status strings —
  use `status_text` so wording is consistent across every screen.
- Do not cache `file_count` across a card-removal event; re-read the view.
- Do not assume a selection index remains valid after a new recording or a
  card change; re-list, then re-select.

---

## 5. What the storage side needs from the UI / platform

The UI does not supply these — the STM32 platform layer does, in Semester 2 —
but they are listed so the group agrees on ownership:

| Input | Supplied by | Feeds |
|---|---|---|
| Card-detect GPIO level | Platform (`storage_stm32.c`) | `storage_status_set_card_detected()` |
| FatFs mount result | Platform | `storage_status_set_mount_result()` |
| Card capacity / free space | Platform | `storage_status_set_capacity()` |
| Result of each write/read | Platform, via `storage_port.h` | recording/playback session modules |

---

## 6. Integration note for Semester 2

The UI subsystem should include `storage_status.h`, `file_index.h` and
`wav_storage.h` only. It must not include `storage_port.h` — that interface is
implemented by the platform layer and consumed by the storage modules, not
by the UI.
