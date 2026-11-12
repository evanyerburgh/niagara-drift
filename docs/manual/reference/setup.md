---
title: Setup
intro: GLO: the room's settings and the Tuning
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## The knobs

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Setup 1 | fade | morph | scene time | sync |
| Setup 2 | tuning | tune | stereo | pressure |

Everything on [[GLO]] belongs to the FM-1, not to a sound: scenes don't change it, except the Tuning.

## Setup 1

<img class="shot" src="../../img/screen-setup.png" width="240" alt="The Setup 1 page: the fade, morph and scene times, and Sync">

The time knobs give most of their turn to slow times. *Picture:* a row for each time, ending where its knob is.

### Fade

How long PLAY takes: up to 60 s (1.5 s to start).

### Morph

How long a black key or a new LFO wave takes to move the sound: 0.02–30 s (2 s to start).

### Scene time

How long every scene move takes: 0.1 s to 5 minutes (about 8 s to start).

### Sync

**On** follows a MIDI clock, **Off** ignores it. Under it: "no clock", "120 BPM", "keeps 120" or "free time".

## Setup 2

<img class="shot" src="../../img/screen-setup2.png" width="240" alt="The Setup 2 page: the Tuning's ring, Tune in Hz, the stereo width and the keys' pressure">

*Picture:* the Tuning as a ring, a tick a note, turned by Tune.

### Tuning

The notes the pitch knobs move through ([below](#tunings)).

### Tune

A's pitch: 427 to 453 Hz (440 to start); 1 Hz a click, 0.1 with [[SEL]].

### Stereo

Mono to drone I left, drone II right. A drone playing alone sits in the middle.

### Pressure

How hard you press the scene keys: up for a light touch, down for a firm one.

### The speaker

A low cut (about 120 Hz) for bass it can't play; off while headphones are in.

## Tunings

| Tuning | What it is |
|---|---|
| **12-tone** | semitones |
| **Quarter** | quarter tones |
| **19-, 31-, 53-tone** | equal temperaments |
| **Just** | just intonation (Ptolemy's 5-limit) |
| **Harmonics** | the odd harmonics 1–15 of A: the drones as notes of one harmonic series |
| **LMY** | La Monte Young's tuning for *The Well-Tuned Piano* |
| **Harrison** | Lou Harrison's tuning for his National Steel guitar |
| **Shrutis** | the 22 shrutis of Indian music |
| **Partch** | Harry Partch's 43 notes to the octave |

### Where they sit

- **Just and Shruti move with drone I** (a movable do): its pitch knob picks the tonic.
- **The others are built on A**, so Tune moves them. LMY's 1/1 is C, Harrison's and Partch's G, Harmonics' A.

### What follows the Tuning

Wander, Chord, a partner's interval and Shift's steps all take the Tuning's notes.

### Intervals on a Tuning

Named as composers write them: steps on the equal ones ("11\19"), ratios on the others ("3/2", "7/4"), when what you
hear is within 10 cents; otherwise the familiar name ("5th").

### A sound carries its Tuning

A scene saved on LMY brings LMY back, even with Pitch lock on. A Preset plays on 12-tone unless it has its own. The
Tuning changes half way through a scene's glide.
