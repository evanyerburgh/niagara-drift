---
title: LFO
intro: One clock, two waves, and Movement
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## LFO

One clock with two waves, LFO I and LFO II, and Movement, which sets how much of them you hear.

## Controls

<img class="shot" src="../../img/panel-left.png" width="200" alt="The FM-1's left knobs: SELECT is the LFO rate, ALGORITHM Movement, PRESETS Gesture time; OCT− and OCT+ pick LFO I's and II's waves">

Where the waves go is on the [LFO pages](lfo-pages.md).

### Rate (SELECT)

One cycle in 100 s to 70 Hz.

### Movement (ALGORITHM)

Every LFO depth together, from still (0%) to double (200%).

> **Movement scales what you set.** It only scales the depths you've set: at 0% nothing moves, at 100% (the middle)
> the depths are as set.

### Shapes (OCT− and OCT+)

LFO I's and LFO II's wave: tap for the next.

## Shapes

| LFO I | LFO II |
|---|---|
| Off | Off |
| Triangle | Tri 90 |
| Square | Square 90 |
| Saw | Exp saw |
| Roundtooth | Log tri |
| 2x Tri | 2x Saw |
| 2x Exp | 2x Pattern |
| 4x Pattern | 4x Exp |
| Random | Rnd glide |
| AND 3 | AND 5 |
| OR 3 | OR 5 |

## Movement

| Movement scales | Movement leaves alone |
|---|---|
| **LFO sends:** all four | **Mods: Speed mod** |
| **Mods: Delay mod** (the delay's wobble) | **Mods: Cross-mod** (that's sound, not the LFO) |
| | The rate, the shapes, and where the waves go |

## Good to know

### Rate and shapes

- **Rate:** most of the turn is 100 s to 10 s, a second a click. Slow rates show a cycle's length ("21s", "1m40").
- **Shapes:** [[SEL]] + OCT steps back; each light pulses with its wave.
- **AND and OR** are two square waves together, one 3 times as fast (LFO I) or 5 times (LFO II). AND steps between 2
  levels, OR 4: on a drone's pitch, a pattern of 2 or 4 notes.

### Movement and the other knob

- **A depth that can't be heard** says why on the screen (Movement at 0%, or its wave off).
- **PRESETS,** the other left knob, is Gesture time: see [Gestures](gestures.md).
