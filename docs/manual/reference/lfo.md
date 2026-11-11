---
title: LFO
intro: One clock, two waves, and Movement
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

| Knob | Sets |
|---|---|
| SELECT | the rate, from one cycle in 100 seconds to 70 Hz: below 100 seconds each click is a second, down to 10 seconds (most of the turn), then steps of the same size up to 70 Hz; below 0.1 Hz the screen shows a cycle's length ("21s", "1m40"). While SELECT, ALGORITHM or PRESETS turns, its value fills the bottom of the screen, large |
| ALGORITHM | the **Movement**: every LFO depth together, from still (0%) to double (200%) |
| OCT− · OCT+ | LFO I's · LFO II's shape: tap for the next (or off), round and round; SEL + OCT for the one before |

One clock, two waves; the OCT− and OCT+ lights pulse with them. After the DS-2's shapes comes a random one,
then the Lyra-8's two: **AND** and **OR**, two square waves together, one at the LFO's rate and one inside it
(3 times as fast on LFO I, 5 times on LFO II). AND is up only while both are, so it steps between 2 levels; OR
adds them, 4 levels. On a drone's pitch that's the Lyra-8's "acid": the drone jumps between 2 or 4 notes in a
pattern, as far apart as the osc depth says.

**Movement** (ALGORITHM) is how much of all that you hear: it scales every LFO depth together, as you've set
them. At 0% the LFO moves nothing, whatever the depths say; at 100% (the middle) the depths are as set; at 200%
they're twice as deep. It can only scale depths you've set: the starting sound breathes its filters deeply
and slowly, so Movement is heard at once; set an osc depth to hear it move the pitch too.

| Movement scales | Movement leaves alone |
|---|---|
| **LFO sends**: all four knobs (each drone's two sends) | **Mods: Speed mod** (how much the LFO speeds itself up) |
| **Mods: Delay mod** (the delay's wobble) | **Mods: Cross-mod** (the drones moving each other: that's sound, not the LFO) |
| | the LFO's rate, its shapes, and where the waves go (the black keys on the LFO pages) |
 Turning a
depth that can't be heard says why on the screen (Movement at 0%, or its wave off).
**PRESETS** is **Gesture time**: how fast every gesture moves (see [Gestures](gestures.md)).
