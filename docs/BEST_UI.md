# Power header and SD footer: the final firmware

## What it is

The final firmware is the portrait screen (see `docs/VERTICAL_UI.md`) plus a
power header at the top and an SD footer at the bottom of every screen,
five fixes, a pop-up for REC or PLAY with no SD card, and a Battery
screen with the average current and the battery voltage. The debug screen still
uses the whole panel. On 30 Oct it also gained effect files made ahead, every WAV in
Files, a Delete button, a frog screen saver, a 240 ms echo and a speaker
crossover (sections below). On 1 Nov the serial console and the solo
builds were removed: every check is on the screen. In the original project
repository this was the `best-ui` branch; the older landscape build of
28 Oct is kept there as tag `fallback-2026-10-28`. After a first flash,
run the board checks below.

```
BATTERY            TO EMPTY    TO FULL      header
[#####  ]  80%      01:15       --:--
RECORD                         [ HOME ]     title bar
  ...                                       the screen itself
[ SD OK ]  REC 0:07 REC03.WAV               footer
           FX ECHO            MUTE
```

## Where to start reading (for the viva)

Start with the diagram in `docs/RTOS_ARCHITECTURE.md` section 1 and the task
map at the top of `Core/Inc/app_rtos.h`. Then follow a recording from the
end of REC to the speakers, then the screen (each `.c` with its `.h`):

1. `PD5b_Jack/Application/fx_queue.c`: after a recording, chooses which file
   and effect to make next (the effect you picked first, then the cheapest).
2. `PD5b_Jack/Application/fx_job.c`: makes one effect file in small steps,
   one per pass of Jack's loop (folder, `.TMP` file, chunks out and back,
   rename to `.WAV`).
3. `PD5b_Jack/Application/fx_pipe.c`: keeps count of what one job has read,
   sent, got back and written.
4. `Core/Src/jack_fx.c`: hands each chunk to FxRenderTask through its queue
   and takes the answer back.
5. `Core/Src/tom_fx_render.c`: FxRenderTask, the lowest-priority task: runs
   the effect on each chunk, so the screen never waits for it.
6. `PD5b_Tom/Application/FxRender/fx_render.c`: the effect chain it runs, the
   same as the old live one.
7. `PD5b_Tom/Application/RateAdapter/rate_adapter.c`: turns 32 kHz
   recordings into 48 kHz (2 samples in, 3 out).
8. `PD5b_Jack/Application/fx_store.c`: effect-file names
   (`FX/ECHO/REC05.WAV`), the header, and the check that a file still
   matches its recording.
9. `PD5b_Jack/Application/play_plan.c`: what PLAY does: play the ready
   effect file, make it first ("Preparing"), or play the original.
10. `PD5b_Tom/Application/Crossover/crossover.c`: splits every sound at
    1.6 kHz, highs to the left tweeter and lows to the right subwoofer.
11. `PD5b_Tony/Application/file_window.c`: the 16 file names the screen holds
    while you scroll through all of them.
12. `PD5b_Tony/Application/delete_confirm.c`: the Delete button's "tap
    again" rule.
13. `PD5b_Tony/Application/screen_saver.c`: the frog face after 30 s without
    input.
14. The tests in each package's `Tests` folder, one file per new module (for
    example `test_fx_job.c`): every test name says, as a sentence, what must
    hold.

## The header

- **Battery icon and charge (SoC):** green-yellow is fine, yellow is low,
  red is critical or speaker locked, and a grey ` --%` means no reading.
  The nub on the icon turns cyan while charging.
- **TO EMPTY and TO FULL** in hours:minutes, as on the Battery screen
  (see "The Battery screen and the times" below). A grey `--:--` means no
  estimate, for example TO FULL while on battery.
- **The tag** (top left) says what matters most now. The first row that
  applies wins:

| Tag | Colour | Means |
|---|---|---|
| SPEAKER LOCKED | red | battery too low: speaker off until 32 % |
| SOC OVERRIDE | magenta | the debug screen is faking the charge |
| NO GAUGE | grey | no reading from the fuel gauge |
| CHARGER FAULT | red | the charger reports a fault |
| CHARGING | cyan | charging from USB |
| CRITICAL | red | under 10 % |
| LOW BATTERY | yellow | under 30 % |
| ON USB POWER | light grey | USB plugged in, not charging (pack full) |
| BATTERY | light grey | running on the battery |

## The footer

The **SD chip** says if the card can be used, and if it is safe to pull:

| Chip | Colour | Means |
|---|---|---|
| SD ... | grey | no news from the card yet (start-up) |
| SD USB | cyan | the card is on a PC over USB |
| SD BUSY | orange | recording, playing or making effect files: do not pull the card |
| NO SD | red | no card in the slot |
| SD OK | green | card ready |
| SD FULL | red | card full |
| SD WAIT | yellow | card just put in, not ready yet |
| SD ERR | red | card cannot be used (for example exFAT: format it as FAT32) |

**Line 1** says what is running: `REC 0:07 <name>` in red (the time left,
as on the Record screen), then `SAVING <name>`; `PREPARING <name>` in
yellow while the effect file PLAY waits for is made; `PLAYING <name>` in
green; `PAUSED <name>` in yellow; blank when idle. When REC or PLAY did
nothing, it says why in orange for 3 s (or until the next REC or PLAY),
for example `REC: NO SD CARD`, `REC: SD CARD FULL` or `PLAY: NO RECORDINGS`.

**Line 2** shows the effect, `FX <name>` (grey for NONE; yellow
`FX ECHO 45%` while it is being made; grey `FX ECHO N/A` when this file
cannot have it), and a red `MUTE` while muted. MUTE is not shown while
the speaker is locked: the header says so.

Only the parts that change are redrawn, so nothing blinks. Tapping the
header or footer does nothing (on the fault screen any tap still closes
it).

## REC or PLAY with no SD card

Before, REC with no card showed "Recording" for a moment and then gave
up. Now the toy checks the card first. If it cannot record or play,
nothing starts, and:

- a big **red pop-up** says why for 3 s, for example `NO SD CARD` /
  `Insert a card` / `to record`;
- the **fault beep** sounds once;
- footer line 1 says it too (`REC: NO SD CARD`).

| Pop-up | Means |
|---|---|
| NO SD CARD | no card in the slot |
| SD CARD ON PC | the card is on a PC: unplug the USB |
| SD CARD FULL | no space left |
| SD NOT READY | card just put in: wait a moment |
| SD CARD ERROR | the card cannot be used: re-insert it, or format it as FAT32 |
| NO RECORDINGS | PLAY on an empty card: press REC first |
| COULD NOT RECORD / PLAY | the card looked fine but it still did not start: try again |

It closes after 3 s, or on the next button press, knob turn or tap. That
press still does its job, so REC again once a card is in starts the
recording. A tap on the red box only closes it, so nothing hidden under
it is pressed. While no card is in, the Record screen's first row says
`Insert SD card`, and so does the Play screen under its button. On the
Files screen, `Select (play)` (or a knob push) with no files listed shows
the same pop-up as PLAY.

## The Battery screen and the times

The Battery screen has six lines:

```
Charge  80%
Current   -152 mA       current over the last second (minus = discharging)
Average   -140 mA       current over the last 2 minutes
Voltage  3.712 V        battery voltage
Time left 01:15         To full   00:42 while charging
Status Discharging      Charging, Plugged in, Full, Locked (low), No gauge
```

(30 Oct: the average current was swapped for the voltage, which used to
share the first line. 1 Nov: the 2-minute average was added under the
current of the last second, which jumps with the music; the times come
from the average.)

**TO EMPTY and TO FULL** (header and Battery screen) are worked out by our
firmware, from the gauge's charge left and full capacity and a 2-minute
average of the current (since 1 Nov; it was 10 s, and music made the time
jump by hours: about 125 mA idle, 135-220 mA for moderate music, up to
300-400 mA for loud music). The average starts again when the charger is
plugged in or out, so a time is back about 5 s after that and then
settles over the 2 minutes. The times come from a held current that only
follows the average once it has moved 5 % (at least 10 mA): on the charger
the battery may only gain or lose about 30 mA, and there a few mA of
wobble would otherwise move the time by hours. They are shown in steps
(1 min under an hour, 5 min under 10 h, 15 min above) and move one step
once the new time is one and a half steps away, or straight there from
two and a half steps. So a steady drain counts down one step at a time
(going under an hour it moves from 01:05 straight to 00:59) and the time
never flicks between two steps; a change of the current itself, such as
the music stopping, still takes the 2 minutes of the average. On the charger, loud
music can take more than the charger gives: the battery then drains, the
status says `Plugged in` and the line shows the time left at that drain.
`Full` = the gauge says the battery is full (or the charger says the
charge is complete). Before 29 Oct the times came from the gauge's own
slow average and took minutes to swing over (4 h, then 99 h, ...). The
gauge's own times are still on debug page 3 (`gauge empty / full`), next
to ours (`est`), the current of the last second and the 2-minute average.

## The five fixes

1. **Faster cursor:** a knob click on Home or Effects redraws two tiles or
   rows, not six. The Files name no longer shimmers.
2. **Effects taps:** tapping a row turns that effect on, and again off.
   Before, every row toggled the cursor's effect (Autotune after start-up).
3. **One press is one tap:** a press no longer also taps the screen it
   opens. Volume hold-repeat stops when you let go.
4. **PLAY after REC** plays the new recording, not REC00 or the last file
   picked. This also holds when PLAY is pressed the moment REC stops the
   recording.
5. **All recordings:** Files lists every WAV on the card, 16 names fetched
   at a time, and empty files from a card pull are hidden. This is Jack's
   code, so Jack, please check it.

## Effects made ahead ("Preparing")

Effects no longer run while a file plays. For each recording, and for each
file you play or pick, the toy makes one effect file per effect, for
example `FX/ECHO/REC05.WAV`: the effect you picked first, then the cheapest.
FxRenderTask, the lowest-priority task, does the sound work, so the screen
never waits for it; Jack's storage task does all the card reading and
writing.

- PLAY with an effect plays its file at once if it is made. If not, the
  Play screen shows `Preparing 45%` (footer `FX ECHO 45%`) and the file
  plays by itself when it is done. BACK goes Home meanwhile.
- Effect files are made only while nothing plays or records. The first
  switch to an effect that is not made yet pauses the sound for about
  1-3 s, Autotune about 11 s.
- While effect files are made the chip says `SD BUSY`: do not pull the
  card. A 10 s recording's six files take about 20 s.
- PLAY, REC, DELETE or an effect switch no longer throws away an effect
  file that is half made: it is **parked**. The part made so far stays on
  the card (as `FX/<EFFECT>/NAME.TMP`) and the file carries on from there
  later; the Preparing screen comes back at the same percentage at once.
  If no other effect file was made in between, it simply carries on (after
  a STOP the background finishes it first). Otherwise it is **rebuilt**:
  the effect runs again over a short stretch just before where it stopped
  (64 ms; 4 s for Echo and Reverb), that sound is thrown away, and the file
  carries on. Nothing already made is made again. Up to six parked files are remembered; a reboot, USB or
  another card forgets them.
- So an effect switch during Preparing now costs: the switch itself about
  10-50 ms (worst about 0.3 s on a slow card), and switching back costs
  the rebuild, not a remake from 0 %: Autotune about 0.1 s, Pitch up, Pitch
  down and Robot well under 0.1 s, Echo about 0.5 s, Reverb about 1.5 s
  (estimates; not yet measured on the board). Never more than making the
  file from 0 %.
- A rebuilt Autotune, Pitch up, Pitch down or Robot file is the same,
  sample for sample, as one made in one go. Echo and Reverb come very
  close, but not always exactly, far too little to hear: Echo can be 1 of
  32768 off for up to about 1.3 s, even on normal speech; Reverb after
  a quiet stretch a few units (measured up to 10 of 32768) until about a
  second after the sound comes back, or to the end if the file ends
  silent. Details: Tom's `fx_render.h`.

## Stronger Autotune (30 Oct)

The old Autotune was measured to leave speech almost unchanged: its pitch
estimate arrived about 50 ms too late, so the correction landed at random.
It now snaps the voice at once to C major pentatonic (C D E G A), using the
pitch of each moment measured 39 ms ahead (effect files are made offline,
so the sound can be played that much later). Speech comes out as a
sing-song tune. Details: `PD5b_Tom/Docs/TECHNICAL_EXPLANATION.md`.

- After flashing, every effect file is made once more (the effect version
  went from 1 to 2): about 20 s per 10 s recording, in the background.
- The Autotune file starts 39 ms late and ends 39 ms early. Switching to or
  from Autotune while a file plays can repeat or skip about 40 ms.
- Another scale is a change of one table, `ae_autotune_scale` in
  `audio_effects.c`: all true is the classic chromatic hard tune, adding F
  and B gives C major.
- A PC file longer than 60 s (or not 32 or 48 kHz) plays without effects:
  `Effect: None (n/a)` and a grey `FX ECHO N/A`.
- Deleting the whole `FX` folder on a PC is safe: the toy makes the files
  again.
- The echo now repeats after 240 ms (it was 120 ms).

## All files

Files lists every playable WAV in the card root, not just 16. The screen
holds 16 names at a time and asks Jack for more as you scroll;
`File 41 of 300` says where you are. The effect files sit in the `FX`
folder, which the list never shows. After a reboot or a card swap the last
file is selected. After a Delete a new recording can sit in the middle of
the list (it is still selected after recording).

If the card cannot be read just when Jack sends the next names (a short
read error), Files can say `No files found` although the files are there.
Leave Files and open it again: that reads the card again and shows the
files (a card that really cannot be read then shows a card error).

## Delete

The Files screen has a 4th button, `Delete`. Tap it once: the row turns red
(`Sure? Tap again`) and info line 2 asks `Delete REC05.WAV?`. Two taps 0.5
to 3 s apart delete the file and its six effect files, and the footer then
says `DELETED REC05.WAV` in grey. A faster double tap deletes nothing: the
first tap arms the row, the second is ignored, and after 3 s the row says
`Delete` again, in blue like any row you have just tapped. Delete stops
any playback, also of another file.

## Screen saver

After 30 s without input a green frog face fills the screen, and every 4 s
its mouth opens and the tongue flicks. The first touch, knob turn or button
press only wakes the screen. Sound keeps playing under it. A small orange
`SD BUSY` tag shows in the top-left corner while effect files are being
made. It never opens over the fault screen, the debug screen or the
pop-up, nor while recording or preparing.

## Speakers

The left speaker (tweeter) plays above 1.6 kHz and the right one
(subwoofer) below it, down to 10 Hz. At 1.6 kHz each plays at half level,
and together they sound as one. Tunes and the fault beep come mostly from
the subwoofer, so the debug page's `Left only` test (1 kHz) is quiet. Both
speakers play at full level (the old 30 % left trim is gone).

## Flashing it

`cmake --preset Debug` and `cmake --build --preset Debug`, then flash
`build/Debug/Sound_modulation_group_demonstration_project.elf` as usual.
If the screen looks like a mix of an older build, delete the `build`
folder and build again. Picture upside down? See `docs/VERTICAL_UI.md`.

## Board checks (first flash)

- [ ] **Start-up:** upright; header, title, tiles and footer all show; no
      dots along the bottom edge. Leaving Debug brings it all back.
- [ ] **Header:** readable at arm's length; no blue or magenta specks.
- [ ] **Charging:** plug in USB: cyan CHARGING within about 1 s; TO FULL
      shows a time or `--:--`. A full pack on USB: ON USB POWER.
- [ ] **No gauge** (the Nucleo on its own): grey ` --%`, NO GAUGE and the
      "Gauge not found" fault.
- [ ] **Debug page 5:** CRIT 8% gives SPEAKER LOCKED and a red charge,
      OK 60% gives SOC OVERRIDE, Clear overrides gives BATTERY.
- [ ] **SD chip:** empty slot NO SD; insert a card: SD WAIT, then SD OK;
      record or play: SD BUSY; connect a PC: SD USB; exFAT card: SD ERR.
      The text is centred with no stray letters (look at SD USB, SD ERR
      and SD ...).
- [ ] **Footer:** the REC countdown matches the Record screen. Play a file
      and press BACK: still PLAYING. Pause it and press BACK: the footer
      line goes blank and the chip turns SD OK (a paused file stops). Mute: MUTE, and a solid red X on the
      Volume screen.
- [ ] **No card:** REC on Home: a red NO SD CARD pop-up and a beep, Home
      stays (no "Recording"), `REC: NO SD CARD` in the footer. The Record
      screen says `Insert SD card`. Insert a card, REC again: it records.
      PLAY on an empty card: NO RECORDINGS (Home stays), and the same for
      Select on the empty Files screen. Tapping the box only closes it.
- [ ] **Battery screen:** six lines, none touching, the last one above
      the footer. Current moves with the music; Average (the 2-minute
      average) changes slowly and stays steady; Voltage reads about
      3.0-4.2 V.
- [ ] **Times:** unplug the charger after charging: TO EMPTY shows a time
      within about 10 s. Plug it in: TO FULL within about 10 s. Debug page
      3 shows both currents and both sets of times.
- [ ] **No flicker:** watch Home and Files for 25 s (the frog screen saver
      opens at 30 s). Nothing blinks.
- [ ] **Knob:** spin it fast on Home and Effects. Only two tiles or rows
      change, and the cursor keeps up.
- [ ] **Taps:** each Effects row turns its own effect on and off. 20 slow,
      rolling presses on each Home tile: the next screen never acts.
      Volume hold-repeat is smooth and stops on release.
- [ ] **Files:** with 2 or more files, record, then PLAY: the new RECnn
      plays. With 40 or more WAVs all show; after a reboot the last one is
      selected. A cut-short recording is not listed.
- [ ] **Fault timing:** pull the card during a recording: the fault screen
      appears with the beep, within about half a second. Over the debug
      screen (page 5, `Storage fault`) it also takes under half a second.
- [ ] **Boot (effect files):** Home appears with the startup tune. Debug
      page 1: heap min at least 1 KB, every one of the six stacks
      (FxRender too) shows a number with free space, and every drop
      counter (`fx` too) is 0.
- [ ] **Effects made ahead:** record 10 s with Echo picked. SD BUSY stays
      on (no blinking) for about 20 s while all six are made: the `FX job`
      row on debug page 2 shows Echo first, then Robot, Pitch up, Pitch
      down, Reverb, Autotune. PLAY: the echo plays at once, about 1/4 s
      after the sound. Pick Reverb: the chip does not flash.
- [ ] **Preparing:** record 10 s with None picked; within about 10 s pick
      Autotune and PLAY: `Preparing nn%` counts up, then it plays by
      itself. BACK during Preparing: Home shows `PREPARING ...` in yellow
      and the sound starts by itself. Record 3 s, stop it with REC and
      PLAY at once: the new recording plays.
- [ ] **Autotune:** record about 10 s of talking, pick Autotune and PLAY:
      the voice jumps between a few notes like a robot singing, clearly
      unlike None. Pauses sound clean, with no hollow ring.
- [ ] **Switching while playing:** Echo, Reverb, None: it carries on from
      about the same place; an effect not made yet shows Preparing first.
      Pitch up sounds higher, Pitch down lower. A 44.1 kHz PC file, or one
      longer than 60 s, with Echo: `NO FX FOR THIS FILE`, then a grey
      `FX ECHO N/A`. A stereo file is still refused.
- [ ] **Smooth:** moving between screens while an effect plays is as
      quick as with none. Over 1 min of playback, `underruns` on debug
      page 1 does not grow. Start and stop 20 times: it
      starts every time.
- [ ] **All files:** with 40+ WAVs, reboot: `File 4x of 4x`. One knob
      step shows the next name at once, and Select plays it. Scroll to
      File 1 and play it. No `FX` entries. Another card: its own last
      file is selected.
- [ ] **Delete:** one tap: red `Sure? Tap again` and `Delete RECnn.WAV?`;
      after 3 s a blue `Delete` again. A fast double tap deletes nothing. Two
      taps about 1 s apart delete it: `DELETED RECnn.WAV` in grey, and on
      a PC `FX/*/` has no file of that name. Delete while another file
      plays: the playback stops and the list closes up.
- [ ] **Screen saver:** leave the toy 30 s: the frog appears and the
      tongue flicks; a touch only wakes it; music keeps playing. Pick a
      50-60 s mono 32 kHz PC WAV, press BACK and wait: the frog shows the
      SD BUSY tag until its six files are made (about 2 min).
- [ ] **Speakers:** on the Speaker debug page 100/200/500 Hz come only
      from the sub (right) and 5/8/10 kHz only from the tweeter (left);
      1 kHz mostly from the sub, 2 kHz mostly from the tweeter. If one
      side is much louder, `TOM_LEFT_TRIM_PERCENT` (or `RIGHT`) is the
      one number to change.
- [ ] **USB while preparing:** plug in USB while a file is being
      prepared: the PC gets the card, usually within 2 s (if it takes
      longer it must still come, with no replug). Unplug and plug in again within a second: the PC still
      gets the card.

## Not done, and why

- **Needs CubeMX, then a full board retest:** SPI1 DMA (faster drawing),
  CPU sleep when idle, button interrupts and a 96 MHz clock.
- **Needs an OK to edit the third-party screen driver:** odd-sized colour
  fills (a bigger charge number), faster start-up and text, and the debug
  screen's noise rows.
- **Needs board measurements or listening first:** gaps in odd-sized PC
  WAVs, freeing RAM, touch power use, a hung gauge or I2C bus, and a stuck
  tone after a crash.
- **Checked by reading the code, not by a unit test:** which file is
  selected after Jack sends a new list window, and when the list gets
  shorter (`window_reply` and `storage_status_in` in `tony_ui.c`). After
  the demo this can move into `file_window.c` as one function with Unity
  tests.
- **Needs a team decision** (it changes how the toy behaves): knob push to
  play or record, volume steps of 5, a Stop button, a record countdown, a
  new low-battery sound, hold REC to talk.
- **Later:** tap-to-play Files, a mic level meter, playback progress, saved
  settings, live Home tiles, backlight control.
- **Left out on purpose:** no free space % on the chip, because the chip
  holds one short word; the Storage debug page shows the free space.
