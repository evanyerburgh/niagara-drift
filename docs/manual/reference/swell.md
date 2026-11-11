---
title: Swell
intro: ARP: both drones breathing in time
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

<img src="../../img/page-swell.png" width="244" alt="The Swell page: two bars of the Swell, four breaths, as strands meeting at a dot">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Swell | rate (on a MIDI clock: division) | depth | shape | swing |

- Tap ARP for this page; tap it again here to start or stop the Swell (the header says **on**, or **off · tap ARP**).
  The picture shows two bars of it, four breaths, the level falling down the picture in dB (the dip is the Depth), as
  five strands that meet at a dot where the Swell is now: wide when it's slow, nearly one line when it's fast. **Shape**
  shows its number with its kind under it (gate, pulse, even, ramp).
- **The Swell** runs from a 33-second breath to a buzz (the starting sound has it on: a slow breath, 40 deep).
- **Depth** is how far it dips, in dB (down to −30; all the way, silence). **Shape** runs both ways from the
  middle, **even** (a smooth breath): to the right it becomes a **ramp**, swelling slowly and falling away quickly,
  more like a sawtooth the further you go ("ramp 100": the rise takes 97 % of each breath; at fast rates the fall
  stays at least 20 ms, so it doesn't click); to the left it firms up into a pulse ("pulse 50"), and only at the end into a hard
  gate ("gate").
- **On a MIDI clock** (see MIDI below) the Swell breathes in time: KNOB 1 becomes **Division**, how long a breath
  lasts, from 32 bars down to a quarter note, and the picture shows it with the tempo ("8 bars · 120"). A locked LFO
  (black key 5, held) restarts on the Swell's bars, so it follows the clock too.
