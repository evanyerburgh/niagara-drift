---
title: Help
intro: When something doesn't work
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## Quick answers

- **Drone II went quiet, the header says II OUT:** [[SEL]] + [[PLAY]] brings it back.
- **Back to the sound before a scene:** [[SEL]] + [[SAVE]] (again to go forward).
- **Drone II keeps jumping to other notes:** that's Wander. Turn it to 0 on Pitch moves ([[SEQ]] twice, KNOB 3).
- **A knob's value is gray:** it can't be heard as things are, and the screen says why ("needs sqr", "needs
  [[F#3]]", Movement at 0%).
- **The low end is missing on the speaker:** it can't play the lowest notes and clicks; listen on headphones.
- **A Preset doesn't play at its own pitch:** Pitch lock ([[C#4]]) keeps the pitch you're on; turn it off.

## Installing and updating

- **Install** from the [web installer](https://evanyerburgh.github.io/niagara-drift/install/) in Chrome or Edge, with a
  USB data cable straight into the computer.
- **Update:** run the installer again. Your sounds and scenes are kept, unless that version's notes in the
  [changelog](../../CHANGELOG.md) say otherwise.
- Installing firmware is at your own risk.

## Backups

- **Before every install** over Niagara Drift, both installers save your sounds, scenes and settings to a file: the
  web installer into your downloads (and it offers to put them back), the terminal one into
  `~/niagara-drift-backups/`.
- **Any time:** the web installer's "Your sounds and scenes" card saves or restores one, as
  `python3 tools/fm1_install.py --backup` and `--restore FILE` do. Each reads the other's files.
- **Trying another firmware:** back up first, restore when you come back.
- **Another FM-1:** a backup leaves out the knobs' calibration, which belongs to each FM-1, so it can go onto another.

**Restoring is safe.** The whole backup goes onto the FM-1 and is checked first, changing nothing. The FM-1 asks
**RESTORE FROM USB?**: [[SEL]] puts it back, [[HOME]] keeps what it has (as does a minute without either). Then your
sounds and scenes change all together or not at all, even if the power goes off halfway (the next power-on finishes
it). A damaged file is refused before anything is written.

## Going back, or to another firmware

- **The installer's "Back to M-VAVE's firmware"** writes M-VAVE's firmware back, V16 or V15, over the same cable. Or
  use M-VAVE's updater, M-UPGRADE, from [m-vave.com/download](https://www.m-vave.com/download).
- **Other firmwares** mostly expect M-VAVE's: go back first, then install them.
- **Safe mode:** if Niagara Drift fails to start twice in a row, it comes up with no sound and USB on, and the
  installer still works.

## If an install doesn't take

1. Use a USB data cable straight into the computer, close other apps that use MIDI (a DAW, M-UPGRADE), replug, try
   again. Nothing is written until the FM-1 has checked the whole package, so trying again is safe.
2. Going back from 0.2 or another firmware, and nothing was written: choose the other M-VAVE file (V16 or V15). Each
   takes back only the version whose bootloader the FM-1 has; from 0.2.1 on, Niagara Drift takes either.
3. Still stuck: hold [[OCT−]] and [[OCT+]] for 5 seconds. The FM-1 goes into its chip's own update mode, which no
   firmware can get in the way of, and [M-Flasher](https://github.com/jvitkauskas/m-flasher) (Windows, Mac, Linux)
   puts M-VAVE's firmware back. If an FM-1 won't start at all, see [BUILDING.md](../../BUILDING.md#install).

**Newer FM-1s on 0.2:** up to 0.2, Niagara Drift didn't recognize the memory chip of some newer FM-1s; there it couldn't
save and ignored installs. On such an FM-1 running 0.2, use step 3 to leave it. Since 0.2.1 it takes any standard
chip: the newer UC25HQ80 (ID B36014) and the first Puya P25Q80 (856014).

## Something else

It's tested on one FM-1 so far. If something goes wrong, sounds wrong or could be better,
[open an issue](https://github.com/evanyerburgh/niagara-drift/issues): what you did, what happened, and the version
on the startup screen.
