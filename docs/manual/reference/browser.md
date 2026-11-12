---
title: Playing in the browser
intro: The emulator: the mouse and the computer keys
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

[Play it on the website](https://evanyerburgh.github.io/niagara-drift/): the firmware itself, compiled for the
browser, with sound. SAVE keeps your sound in that browser.

| Do | Mouse | Keyboard |
|---|---|---|
| Turn a knob | drag up or down, or scroll | ↑ ↓ LFO rate, ← → Movement |
| Hold a button | Shift-click or right-click (click again to let go) | |
| PLAY · SAVE (hold) · SEL (hold) | | Space · Enter · C |
| HOME · EDIT · LFO I · LFO II (OCT− · OCT+) | | Esc · V · Z · X |
| Black keys 1–11 | click | 1 … 0, − |
| Scenes 1–8 · gestures 9–12 · gestures 13–16 | click | Q … I · A … F · G … K |

- **No pressure:** the mouse can't press harder, so on keys 9–12 a held click climbs at the gesture's own time
  (PRESETS, Gesture time) and falls when you let go. On the FM-1 those keys follow how hard you press.
- **Working on the code:** `./emulator.command` builds the same page from your checkout, serves it at
  http://localhost:8765 and opens it.
