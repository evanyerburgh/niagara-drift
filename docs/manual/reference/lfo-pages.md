---
title: LFO sends and Mods
intro: LFO: where the LFO goes, and what else it moves
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| LFO sends | I pitch (or X-Wave) | I cutoff (or resonance) | II pitch (or X-Wave) | II cutoff (or resonance) |
| Mods | speed mod | source (LFO) | delay mod | cross-mod |

The LFO's rate and waves are on the top-left knobs: see [LFO](lfo.md).

## LFO sends

<img class="shot" src="../../img/screen-lfo.png" width="240" alt="The LFO sends page: the two LFOs, and each drone's sends as sentences with their keys">

How far the LFO moves each drone's pitch and cutoff. The middle of each knob is none.

- **What each send moves:** Pitch, or X-Wave with black key [[G#3]] / [[G#4]] on the LFO pages; Cutoff, or resonance
  with [[C#4]] / [[C#5]].
- **Under each value:** how far it swings, as heard ("±12c", "±3.6 oct"), Movement included. On X-Wave and resonance,
  100 sweeps the whole range, turning at the ends so it keeps moving.
- **The picture:** each drone's two sends as a sentence with its keys: "[F#3] LFO II → [G#3] x-wave" (F#3 picks the
  LFO, G#3 what it moves). A send at 0, or fed by an LFO that's off, is gray.
- **Above them:** LFO I over II, each a cycle with its dot, the rate, and the Swell lock ([[D#4]], held): free, or on
  Swell (the LFO starts over each Swell bar). "mvmt 0%" in the rate's place: Movement is at 0, so nothing moves.

## Mods

<img class="shot" src="../../img/screen-lfo2.png" width="240" alt="The Mods page: the LFO's clock feeding LFO I and II, the delay's wobble, and cross-mod's mode">

- **Speed mod** ("Speed"): the LFO speeds up and slows down on one of its own waves, up to 3 octaves either way.
- **Source:** which LFO does that (I, II or both).
- **Delay mod** ("Delay"): how far the LFO wobbles the delay's time, up to ±25%, on the wave black key [[D#5]] picks
  on the LFO pages. Heard only with some delay mix (Space).
- **Cross-mod** ("X-mod"): how deep cross-mod goes, in the mode black key [[F#5]] picks:
  - **I moves II's cutoff:** drone I's sound moves drone II's filter.
  - **Bend each other** (cross-FM): each drone's sound bends the other's pitch. A little roughens and thickens them; a
    lot tangles them into one metallic sound.
  - **Snarl:** the finished sound, echoes and all, bends both drones' pitch: a growl.
  - The drones' pitches stay where you set them; the bending is around them.
- **The picture:** the LFO's clock feeding LFO I and II, the Source wave looping back into it, a line to the delay's
  time; under it, cross-mod's mode and its key.
- **Movement** (ALGORITHM) scales the delay's wobble too, not Speed mod or Cross-mod.
