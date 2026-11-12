---
title: Space and Details
intro: FX: the delay, the drive and the reverb
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Space | delay repeats | delay mix | reverb length | reverb mix |
| Details | delay drive · SEL: — | delay time · SEL: delay type | reverb damp | reverb type · SEL: into reverb |

<img class="shot" src="../../img/screen-fx.png" width="240" alt="The Space page: the delay's echoes as half-rings on the left, the reverb's tail as rings on the right">

- **Space** is the page [[FX]] opens: the amounts you play, the delay on the left, the reverb on the right.
- **Details** ([[FX]] again) holds what you set before playing. Hold [[SEL]] and turn KNOB 2 for the delay's type,
  KNOB 4 for what goes into the reverb (shown above them: "SEL Tape", "SEL Both").

## Delay

<img class="shot" src="../../img/screen-fx-sel.png" width="240" alt="The Details page with SEL held: the delay's type turning">

- **Types**, plain to most alive (a change crossfades; the knob goes round):
  - **Clean:** bright, no wow, so a just-tuned drone stays still.
  - **Wide:** stereo, the right side's echoes at three-quarters of the left's time.
  - **Ping-pong:** the echoes bounce left and right.
  - **Tape:** warm, a gentle wow (a new sound starts with it).
  - **Grain:** Tape whose time its own echoes shake: grainy, metallic.
  - **Self:** Tape whose time stretches as its echoes get louder: they sag and wobble.
- **Time:** 70 ms to 1.46 s. Turning it bends the echoes' pitch, as tape does.
- **Repeats:** past about 85 it runs away into self-oscillation ("runaway"), held by a soft clip.
- **Drive** distorts the drones and their echoes together, after the delay and before the reverb: ghost notes, beating
  and crackle on the repeats. The first fifth blends a little in; past that, up to a hard clip, at the same loudness.
- **The picture:** the sound as a white ring, each echo a half-ring at its time (2 s at the right), as big as it's
  still loud, left channel up in blue, right down in aqua.

## Reverb

<img class="shot" src="../../img/screen-fx2.png" width="240" alt="The Details page: drive, time, damp and type, with the two SEL choices above">

- **Types** (a change crossfades; the knob goes round): **Room**, **Hall**, **Cathedral** (the largest), **Chorus
  hall** (a hall that shimmers), **Shimmer** (the Hall with an octave above what's playing, in two voices a few cents
  apart; it washes in over about 8 s and out over 6).
- **Length:** 0.3 to 40 s. At the top, **Inf**, it never dies away: a freeze. PLAY to stop lets it go.
- **Damp** darkens the tail (not at Inf, so a freeze holds what's in it).
- **Into reverb** ([[SEL]] + KNOB 4 on Details): **Both** (the drones and their echoes), **Echoes** (the drones stay
  dry), or **Drones** (the echoes stay dry and distinct on top).
- **The picture:** the tail as rings (40 s at the right), as tall as the tail is then, darker with Damp; at Inf
  "freeze" in blue. Under them, what goes into it.

The delay and reverb are ChoralRoot's design (see [About](../about.md#where-it-came-from)).
