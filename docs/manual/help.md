---
title: Help
intro: When something doesn't work
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## Quick answers

- **Drone II went quiet, and the header says II OUT:** [[SEL]] + [[PLAY]] brings it back.
- **Back to the sound before a scene:** [[SEL]] + [[SAVE]] (again to go forward).
- **Drone II keeps jumping to other notes:** that's Wander. Turn it to 0 on Pitch moves ([[SEQ]] twice, KNOB 3).
- **A knob's value is gray:** it can't be heard as things are, and the screen says why ("needs sqr", "needs
  [[F#3]]", or Movement at 0%).
- **The low end is missing on the speaker:** the FM-1's speaker can't play the lowest notes and clicks; listen on
  headphones.
- **A Preset doesn't play at its own pitch:** Pitch lock (black key 4) keeps the pitch you're on; turn it off.

## Installing and updating

Install from the [web installer](https://evanyerburgh.github.io/niagara-drift/install/) in Chrome or Edge, with a
USB data cable straight into the computer.

**Updating:** when there's a new version, run the installer again the same way. Your sound and scenes are
kept, unless that version's notes in the [changelog](../../CHANGELOG.md) say otherwise.

**Backing up:** both installers save your sound, scenes and settings to a file before they install over
Niagara Drift: the web installer into your downloads (and it offers to put them back afterwards), the terminal
one into `~/niagara-drift-backups/`. The web installer's "Your sounds and scenes" card saves or restores one at any
time, as `python3 tools/fm1_install.py --backup` and `--restore FILE` do (the FM-1 restarts); each reads the
other's files. Back up before trying another firmware, and restore when you come back. A backup leaves out the
knobs' calibration, which belongs to each FM-1, so it can go onto another one; a damaged file is refused before
anything is written.

**Restoring is safe:** the whole backup goes onto the FM-1 and is checked first, without changing anything. Then
the FM-1 asks on its screen, **RESTORE FROM USB?**: **SEL** puts it back, **HOME** keeps what it has (as does a
minute without either). Your sounds and scenes then change all together, or not at all, even if the power goes off
halfway (the next power-on finishes it), and the FM-1 restarts with them. Afterwards it holds exactly what the backup
held.

**Going back, or to another firmware:** the installer's "Back to M-VAVE's firmware" writes M-VAVE's own
firmware back over the same cable, V16 or V15 (or use M-VAVE's updater, M-UPGRADE, from
[m-vave.com/download](https://www.m-vave.com/download)). Install other firmwares from there: most expect M-VAVE's.
If it ever fails to start twice in a row it comes up in **safe mode** (no sound, USB on), where the installer
still works. Installing firmware is at your own risk.

**If an install doesn't take:**

- Use a USB data cable straight into the computer, close other apps that use MIDI (a DAW, M-UPGRADE), replug and try
  again. Nothing is written until the FM-1 has checked the whole package, so trying again is safe.
- Going back from 0.2 (or from another firmware), and the installer says nothing was written: choose the other
  M-VAVE file (V16 or V15). Those take back only the version whose bootloader the FM-1 already has; from 0.2.1 on,
  Niagara Drift takes either.
- Still stuck: hold **OCT− and OCT+ for 5 seconds**. The FM-1 goes into its chip's own update mode, which no
  firmware can get in the way of, and [M-Flasher](https://github.com/jvitkauskas/m-flasher) (Windows, Mac, Linux)
  puts M-VAVE's firmware back from there. If an FM-1 won't start at all, see [BUILDING.md](../../BUILDING.md#install).
- Up to 0.2, Niagara Drift didn't recognize the memory chip of some newer FM-1s: on those it couldn't save, and it
  ignored installs (it just started again). On such an FM-1 running 0.2, use the step above to leave it. Since
  0.2.1 it takes any standard memory chip, the newer FM-1s' (a UC25HQ80, chip ID B36014) as well as the first ones'
  (a Puya P25Q80, 856014).

## Something else

It's tested on one FM-1 so far. If something goes wrong, sounds wrong or could be better, please
[open an issue](https://github.com/evanyerburgh/niagara-drift/issues): say what you did, what happened, and
what the startup screen says the version is.
