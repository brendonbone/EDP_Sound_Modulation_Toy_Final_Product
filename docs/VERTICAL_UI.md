# Portrait screen

## What it is

The portrait layout of 29 Oct 2026: the screen turned on its side, **240 wide
x 320 tall** instead of the original 320 x 240 (landscape). This page
describes that step. The final firmware also has the power header and SD
footer of `docs/BEST_UI.md`, which replaced the status bar described below.

In that step every screen, button and message stayed the same. Only
positions and sizes changed:

- Home has 3 rows of 2 tiles instead of 2 rows of 3.
- The status bar at the bottom is two short lines instead of one long one.
- The Effects screen shows its three yellow lines under the list.
- The hidden debug screen (hold BACK for 2 s on Home) has the same five
  pages, with shorter rows.

`docs/RTOS_ARCHITECTURE.md` was first written for the landscape screen.
In that portrait step the status bar was two lines (`BAT nn%  SD: ...` /
`FX: ...`), a full debug page took about 0.45 s and the fault screen about
0.29 s.

In the original project repository the portrait layout was made on the
`vertical-ui` branch, and the older landscape build is kept there as tag
`fallback-2026-10-28`.

## Flashing

1. Build as usual: `cmake --preset Debug`, then
   `cmake --build --preset Debug`.
2. Flash `build/Debug/Sound_modulation_group_demonstration_project.elf`
   as usual.

If the screen looks like a mix of an older build, delete the `build`
folder and build again.

## Picture upside down?

The firmware uses `UI_ROT_VERTICAL_2`, which is the right way up on our board
(checked on the board, 29 Oct). If another board shows it upside down, change
one line in `PD5b_Tony/Application/ui_types.h`:

```c
#define UI_PORTRAIT_ROTATION    UI_ROT_VERTICAL_2
```

to

```c
#define UI_PORTRAIT_ROTATION    UI_ROT_VERTICAL_1
```

Then build and flash again. The touch panel uses the same line, so taps
still land on the right buttons. Nothing else needs changing.

## Board checks (first flash)

- [ ] **Start-up:** "BJT2 SOUND TOY" reads the right way up. If not, see
      above.
- [ ] **Every screen looks right:** Home, Record (the red bar reaches the
      right end at 10 s), Play, Effects (the three yellow lines under the
      list are not cut off), Volume, Files and Battery.
- [ ] **Warnings:** on the debug screen's Faults page (hold BACK 2 s on
      Home, turn to page 5), push `Storage fault`: the fault screen opens
      (any key closes it). Push `Battery CRIT 8%` (needs the fuel gauge),
      close its fault screen, press BACK and open Volume: it shows
      "Locked: low battery". Then go back and finish with
      `Battery OK 60%` and `Clear overrides`.
- [ ] **Status bar:** both lines show at the bottom, with no stray line of
      dots along the very bottom edge.
- [ ] **Debug screen:** all 5 pages look right. The last rows sit at the
      bottom and never draw over the title.
- [ ] **Fault timing:** pull the card during a recording: the fault screen
      appears with the beep, within about half a second.
- [ ] **Touch:** every Home tile, HOME on every screen, Volume - / + / mute,
      and Play. Every tap, also near the corners, acts on the button under
      the finger. (The serial console that printed the tap position was
      removed on 1 Nov 2026.)
