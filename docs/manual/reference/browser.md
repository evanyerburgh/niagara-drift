---
title: Playing in the browser
intro: The emulator: the mouse and the computer keys
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## The emulator

The [front page](https://evanyerburgh.github.io/niagara-drift/) runs the firmware itself in your browser, with sound:
no FM-1 needed.

| Do | Mouse | Keyboard |
|---|---|---|
| Turn a knob | drag up or down, or scroll | ↑ ↓ LFO rate, ← → Movement |
| Hold a button | Shift-click or right-click (click again to let go) | |
| PLAY · SAVE (hold) · SEL (hold) | | Space · Enter · C |
| HOME · EDIT · LFO I · LFO II (OCT− · OCT+) | | Esc · V · Z · X |
| Black keys 1–11 | click | 1 … 0, − |
| Scenes 1–8 · gestures 9–12 · gestures 13–16 | click | Q … I · A … F · G … K |

## Good to know

- **SAVE** keeps your sound in that browser.
- **No pressure:** on keys 9–12 a held click climbs at the gesture's own time and falls when you let go. On the FM-1
  those keys follow how hard you press.
- **Working on the code:** `./emulator.command` builds the same page from your checkout and serves it at
  http://localhost:8765.
