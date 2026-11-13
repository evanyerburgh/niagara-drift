---
title: LFO sends and Mods
intro: LFO: where the LFO goes, and what else it moves
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## LFO sends and Mods

Where the LFO goes, and what else it moves.

## Knobs

[[LFO]] opens LFO sends; [[LFO]] again, Mods. The LFO's rate and shapes are on the left knobs: see [LFO](lfo.md).

<img src="../../img/knobs-lfo.png" width="520" alt="Knobs 1 to 4 on LFO sends (each drone's pitch and cutoff) and Mods (speed mod, source, delay mod, cross-mod)">

## LFO sends

<img class="shot" src="../../img/screen-lfo.png" width="240" alt="The LFO sends page: the two LFOs, and each drone's sends as sentences with their keys">

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|
| I pitch (or X-Wave) | I cutoff (or resonance) | II pitch (or X-Wave) | II cutoff (or resonance) |

| Knob | What it does |
|---|---|
| **Pitch send** | How far the LFO moves the drone's pitch, or its X-Wave instead ([[G#3]], [[G#4]] on the LFO pages) |
| **Cutoff send** | How far the LFO moves the drone's cutoff, or its resonance instead ([[C#4]], [[C#5]] on the LFO pages) |

### How a send works

Each send is a chain: which LFO, how much, scaled by Movement, and what it moves.

<img class="diagram" src="../media/lfo-chain.svg" alt="One send, drone I's pitch: LFO I, LFO II or both, picked with F#3; how much with KNOB 1, the middle none; scaled by Movement (ALGORITHM); then G#3 picks what it moves, drone I's pitch (key off) or its X-Wave (key on). Drone I's cutoff is the same with A#3, KNOB 2 and C#4 (cutoff or resonance); drone II's two use its own keys and KNOB 3 and 4.">

Drone I's pitch send. Its cutoff send is the same with [[A#3]], KNOB 2 and [[C#4]] (cutoff or resonance); drone II's two use its own keys and KNOB 3 and 4.

### Which LFO, and what it moves

On the LFO pages, each drone's four black keys pick the LFO for each send and what it moves.

<img src="../../img/black-keys-sends.png" width="560" alt="The black keys on the LFO pages with each drone's four send keys picked out: F#3 to C#4 for drone I, F#4 to C#5 for drone II">

## Mods

<img class="shot" src="../../img/screen-lfo2.png" width="240" alt="The Mods page: the LFO's clock feeding LFO I and II, the delay's wobble, and cross-mod's mode">

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|
| Speed mod | Source (LFO) | Delay mod | X-mod |

| Knob | What it does |
|---|---|
| **Speed mod** (on screen Speed) | The LFO speeds up and slows down on one of its own waves, up to 3 octaves either way |
| **Source** | Which LFO moves the speed: I, II or both |
| **Delay mod** (on screen Delay) | How far the LFO wobbles the delay's time, up to ±25% |
| **X-mod** | How deep cross-mod goes, in the mode [[F#5]] picks: cutoff (I moves II's cutoff), pitch (the drones bend each other, cross-FM) or snarl |

### How the Mods work

<img class="diagram" src="../media/lfo-mods.svg" alt="Mods' three pieces. Speed mod: LFO I, II or both speed up and slow down the clock itself, KNOB 1 how far (up to 3 octaves), KNOB 2 which LFO. Delay mod: LFO I, II or both, picked with D#5, wobble the delay's time, KNOB 3 how far (up to 25%), scaled by Movement and heard with some delay mix. Cross-mod is not the LFO: the drones move each other, KNOB 4 how deep, F#5 picks the mode: I moves II's cutoff, bend each other (cross-FM), or Snarl, the sound bending both.">

Speed mod and Delay mod move with the LFO; Cross-mod doesn't: it's the drones moving each other.

## Where the LFO goes

<img class="diagram" src="../media/lfo-map.svg" alt="The whole LFO: the clock (SELECT, its rate) drives LFO I (OCT− picks its shape) and LFO II (OCT+). Both, scaled by Movement (ALGORITHM), can reach five places, each with a key that picks LFO I, II or both: drone I's pitch or X-Wave (F#3, G#3, KNOB 1), drone I's cutoff or resonance (A#3, C#4, KNOB 2), drone II's pitch or X-Wave (F#4, G#4, KNOB 3), drone II's cutoff or resonance (A#4, C#5, KNOB 4), and the delay's time (D#5, Mods KNOB 3). Speed mod (Mods KNOB 1 and 2) feeds the waves back to speed up and slow down the clock.">

Everything the LFO can reach, on both pages. Each key picks LFO I, II or both for its place; Speed mod lets the waves speed up and slow down the clock itself.

## Good to know

### Sends

- **Every black key on the LFO pages:** see [Black keys](black-keys.md#lfo-keys).
- **Under each send's value,** how far it swings as heard ("±12c", "±3.6 oct"), Movement included.
- **On X-Wave and resonance,** 100 sweeps the whole range, turning at the ends so it keeps moving.
- **Movement** (ALGORITHM) scales the sends and Delay mod, not Speed mod or Cross-mod.
- **"mvmt 0%"** in the rate's place: Movement is at 0, so nothing moves.

### Mods

- **Delay mod** is heard only with some delay mix; its wave is picked by [[D#5]] on the LFO pages.
- **Cross-mod's modes,** as the screen names them: *cutoff* (drone I's sound moves drone II's filter); *pitch* (each
  drone bends the other's pitch, from rough to metallic); *snarl* (the finished sound bends both: a growl).
- **Cross-mod** bends around the pitches you set; they stay where they are.

### Pictures

- On LFO sends, each drone's sends as a sentence with its keys ("[F#3] LFO II → [G#3] x-wave"), the
  two LFOs above. On Mods, the LFO's clock feeding LFO I and II, and cross-mod's mode with its key.
