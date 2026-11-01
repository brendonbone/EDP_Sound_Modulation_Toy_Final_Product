# Demo-day firmware fixes (28 October 2026)

A record of the six demo-day fixes of 28 Oct 2026, all part of this
firmware. In the original project repository they were six commits (one per
task) on branch `demo-fixes`; each built with 0 warnings and passed all host
tests. The checklist below was written for trying them on the board.

| Task | Commit | What you see or hear |
|---|---|---|
| 3 | `feat(demo): tune engine with startup, critical and fault tunes` | a short rising tune at power-on |
| 1 | `feat(demo): critical-battery speaker lockout` | below 10 % battery the speaker goes quiet |
| 2 | `feat(demo): time to empty / to full on the Battery screen` | time left or time to full on the Battery screen |
| 4 | `feat(demo): modal fault screen fed by a new faultQueue` | problems pop up as a red FAULT screen |
| 5 | `feat(demo): hidden TFT debug screen with SOC override and fault injection` | hold BACK 2 s on Home for the debug pages |
| 6 | `feat(demo): tolerant WAV header parser (RIFF chunk walker)` | WAV files made on a PC now play |

Task 3 is first because Task 1 uses its tunes.

## What changed

**Task 1 — Speaker lockout at critical battery.** Below 10 % the speaker
plays a short falling "da-da-dum" and then goes silent. A file that is
playing keeps going, just silently; only the warning sounds can still be
heard. The status line turns red and reads `BAT nn% MUTED`, and the Volume
screen shows `Locked: low battery`. The lockout ends when the battery reads
32 % or more again, and the old volume comes back. BrendonTask decides;
TomTask holds the speaker gain at 0.

**Task 2 — Time left on the Battery screen.** Four lines: charge and
voltage, current, time left (time to full while charging), and status.
`--:--` means the gauge has no estimate yet. Only a line that changes is
redrawn, so nothing flickers.

**Task 3 — Tunes.** A small tune engine in TomTask plays three fixed sounds:
the startup tune (rising, 0.6 s), the critical-battery tune (falling, 0.5 s)
and the fault beep (1 kHz, 0.2 s). They play at a fixed level, whatever the
volume or mute setting.

**Task 4 — Fault screen.** When a task reports a problem (SD card pulled,
bad file, card full, gauge missing, USB unplugged during a copy, critical
battery), a red `FAULT` screen opens over whatever was showing: the source,
two plain-English lines and `Press any key`. Any button, the encoder or a
tap closes it and does nothing else. Faults travel on a new `faultQueue`.

**Task 5 — Hidden debug screen.** On Home, hold BACK for 2 seconds. Five
pages of small text: System (heap, stacks, dropped messages, audio
counters), Storage (card, bus width, space, last error), Battery / power
(gauge, charger, USB), Speaker test (10 test sounds and the 3 tunes) and
Fault injection (a fake SD fault; fake battery levels of 25 % and 8 % for
5 minutes, and 60 % until the board is switched off). A short BACK press
leaves.

**Task 6 — WAV files from a PC.** The player used to accept only the plain
44-byte WAV header. Files with extra blocks before the audio (ffmpeg adds a
LIST block; other programs add bext, fact or JUNK blocks) or with the longer
"extensible" fmt block (Audacity's WAVEX setting) were refused. The parser
now walks the blocks to find the audio. Files must still be mono 16-bit.

## Board test

**You need:** a charged battery pack with the fuel gauge connected, and a
FAT32 SD card holding two of our own recordings (`REC00.WAV` …), one WAV
made on a PC (step 7) and, if you like, a text file renamed `BAD.WAV`.
Flash the firmware (these fixes are part of it).

**1. Power on (Task 3).** A short rising tune plays as the Home screen
appears. No sound at all: check the speaker and amplifier, then try the
Speaker test (step 4).

**2. Battery screen (Task 2).** Tap Battery. Four lines like:

```
 Charge  80% 3712mV
  Current  -150 mA
  Time left 01:15
Status Discharging
```

Plug in a USB charger: within about a second line 3 becomes
`To full   hh:mm` and the status `Charging`, as long as the pack is not
already full (a full pack makes the charger report "complete", and the screen
correctly stays on `Time left` / `Discharging`). `--:--` is normal until the
gauge has learned the battery. `No gauge` means the gauge is not answering.
**Before the demo, check that Charge reads 32 % or more.** If it reads
below 10 %, the speaker stays locked (Task 1): charge the pack first. If the
pack is really full and the gauge just reads wrong, push `Battery OK 60%`
on debug page 5 (step 5): one push keeps the speaker on until the board is
switched off.

**3. Debug screen (Task 5).** Go to Home, let go of BACK, then hold BACK
for 2 seconds (the whole 2 s on Home). The top row reads
`DEBUG 1/5 System   BACK = exit`. Turn the encoder to move through the
pages; a short BACK press leaves. On page 1 check that heap free is above
1000 B and that no stack row is red.

**4. Speaker test (Task 5).** On the debug screen turn to page 4, Speaker
test. Turn to a sound and push the encoder (or PLAY) to play it. A test
sound plays until you push again or press REC; a tune plays once by itself.
The test sounds are loud. Short-press BACK to leave the debug screen.

**5. Battery lockout (Task 1).** Needs the fuel gauge.
1. Play one of our recordings (Files, pick a file) at a comfortable volume,
   then press BACK: Home, and the file keeps playing.
2. Let go of BACK, hold it again for 2 s for the debug screen, then turn
   the encoder **backwards**: the first clicks land on page 5, Fault
   injection.
3. `Battery CRIT 8%`, push. Within a second: the falling tune, then
   silence. A red FAULT screen says `Battery` / `Critical charge` /
   `Speaker muted`; press any key to come back.
4. `Battery LOW 25%`, push. Still silent.
5. `Battery OK 60%`, push. The recording is heard again at the old volume.
6. `Clear overrides`, push, to go back to the real battery reading
   (otherwise `Battery OK 60%` stays in force until the board is switched
   off). Then short-press BACK to leave the debug screen.

Never skip step 5 after `CRIT 8%`: the speaker stays off until the battery
reads 32 % or more.

**6. Fault screen (Task 4).** It should appear within half a second.
- On the debug screen's page 5, push `Storage fault`: `Storage (simulated)`
  / `SD card removed` / `Recording stopped`. Any key closes it and you are
  back on page 5; short-press BACK to leave the debug screen.
- On Home press REC and pull the SD card during the 10 s recording: a beep,
  then `SD card removed` / `Recording stopped`. Put the card back and wait
  for `SD: Ready` on the status line. The recording you cut short stays on
  the card as an empty `RECnn.WAV`: playing it gives `File rejected` /
  `Bad WAV header`, which is expected (delete it on a PC later).
- Play one of the older recordings and pull the card while it plays: a
  beep, then `SD card removed` / `Playback stopped`.
- Play `BAD.WAV` from Files: `File rejected` / `Bad WAV header`.

**7. WAV from a PC (Task 6).** Make a mono, 16-bit, 32000 Hz WAV on a PC
with extra blocks before the audio (the old code refused those). With
ffmpeg: `ffmpeg -i in.wav -ac 1 -ar 32000 -c:a pcm_s16le PCTEST.WAV`
(ffmpeg adds a LIST block). In Audacity, if your version offers it: Tracks >
Mix > Mix Stereo Down to Mono, then export with format "Other uncompressed
files", header "WAVEX (Microsoft)" and encoding "Signed 16-bit PCM" (a longer
fmt block); a normal Audacity "WAV" export may have the plain header, which
played before too. Copy it to the card; on the board pick it in Files: it
plays and loops. Our own `RECnn.WAV` files play as before. A stereo or 8-bit
file gives `File rejected` / `Need mono 16-bit`; a 44.1 kHz file plays, but
slower and lower.

## If something looks wrong

| You see | Likely cause |
|---|---|
| No startup tune | speaker or amplifier wiring |
| Battery items on page 5 do nothing; Battery screen says `No gauge` | gauge not connected or not answering (the fake battery levels need the gauge) |
| Speaker still silent after the battery test | the lockout is still on: push `Battery OK 60%` |
| Speaker silent from power-on and the Battery screen reads below 10 % | the gauge reads critical: charge the pack, or push `Battery OK 60%` on debug page 5 (lasts until the board is switched off) |
| `Card error` / `Format as FAT32` | the card is exFAT or NTFS |
| `File rejected` / `Bad WAV header` on one of our own `RECnn.WAV` | that recording was cut short by a card pull and is empty: play another one |
| `Time left --:--` | normal: the gauge has not learned the battery yet |

`docs/RTOS_ARCHITECTURE.md` section 6 has the long version.

## Small differences from the request

- The Battery screen says `Status Locked (low)`, not `Locked (low bat)`:
  the full text is 23 characters and a line holds 20.
- The lockout test uses the debug screen's 25 % and 60 % items instead of
  20 % and 33 %. It shows the same thing: still silent below 32 %, sound
  back from 32 %.
- The fault screen's 500 ms is counted from when TonyTask takes the fault
  off its queue.
- BrendonTask sends the lockout state to Tom again about every 10 s whether
  locked or not (the request said while locked), so a lost "unlock" message
  cannot keep the speaker muted.
- Time left / to full are read only when the five main gauge readings
  worked, so a gauge that is not answering costs no extra bus timeouts.
- The Battery screen can also say `Status No gauge` when there is no valid
  reading.

## CubeMX changes still needed

None. (Unchanged from before and optional: turning on the SPI1 interrupt
would make screen drawing faster; see `docs/pr/PD6-Tony.md`.)

## Size and tests

ELF (`arm-none-eabi-size`, Debug build):

| | text | data | bss |
|---|---|---|---|
| before (28 Sep 2026) | 193808 | 364 | 119496 |
| after (28 Oct 2026) | 212652 | 368 | 120240 |
| change | +18844 | +4 | +744 |

RAM in use goes from 119864 B to 120608 B (91.5 % to 92.0 % of 128 KB),
flash from 194180 B to 213028 B (37.0 % to 40.6 % of 512 KB). The pcm frame queue,
the audio block pool and the SD buffers are unchanged. The new queues take
about 0.8 KB of the RTOS heap, leaving an estimated 3 KB free (read the real
figure on debug page 1).

Host tests: preset `host-windows` in each `PD5b_<name>` folder
(`cmake --preset host-windows`, `cmake --build --preset host-windows`,
`ctest --preset host-windows`). All pass, 0 failures, 0 compiler warnings:

| Module | Test programs | Test cases | New or changed for the demo |
|---|---|---|---|
| PD5b_Brendon | 5 | 172 (145 before) | battery_monitor: lockout latch, time left / to full, SOC override |
| PD5b_Jack | 5 | 200 (178 before) | wav_storage: 22 new cases for the RIFF chunk walker |
| PD5b_Tom | 4 | 100 (76 before) | tune_player (24 cases, new program) |
| PD5b_Tony | 2 | 281 (166 before) | ui: lockout lines, Battery screen, fault and debug screens; debug_menu (52 cases, new program) |

The firmware itself (`cmake --preset Debug`, `cmake --build --preset Debug`)
builds with 0 warnings. (So did the four solo builds then; they were
removed with the serial console on 1 Nov 2026.)

## Known limits

- The fake battery levels need the fuel gauge; without it they are refused.
- While `Battery OK 60%` is in force (about 18 hours), a really flat battery
  does not mute the speaker: push `Clear overrides` when you no longer need
  it.
- Test sounds on the Speaker page play at a fixed, loud level.
- A WAV whose header says the audio size is 0 is played to the end of the
  file (the task asked for this; normally only streaming tools write 0).
- Nothing here has been run on the board yet.
