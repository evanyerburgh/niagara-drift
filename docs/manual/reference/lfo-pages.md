---
title: LFO sends and Mods
intro: LFO: where the LFO goes, and what else it moves
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

<img src="../../img/page-lfo.png" width="488" alt="The LFO sends and Mods pages: the two LFOs and where they go; the speed modulation, the delay modulation and cross-mod">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| LFO sends | I pitch (or X-Wave) | I cutoff (or resonance) | II pitch (or X-Wave) | II cutoff (or resonance) |
| Mods | speed mod | source (LFO) | delay mod | cross-mod |

- **The sends** and **speed mod** are centered: the middle is none. Each send is named for what it moves: Pitch, or
  X-Wave with black key 2 / 7 on the LFO pages; Cutoff, or Reso with key 4 / 9. Under each value, how far it swings
  in what you hear ("±12c", "±3.6 oct"), with the Movement. On X-Wave and resonance, 100 sweeps the whole range
  (sine to square, none to full resonance); near an end the sweep turns there instead of stopping, so it keeps
  moving all the way round. The picture is the wiring, each drone's two sends as a
  sentence with its keys: "[F#3] LFO II → [G#3] x-wave" (F#3 picks the LFO, G#3 what it moves), "[A#3] LFO I+II →
  [C#4] reso"; a send at 0 (or fed only by an LFO that's off) is gray. Above them the two LFOs, I over II, each a
  cycle with its dot (bent by Speed mod, as on Mods), the rate, and the Swell lock (D#4, held to switch): free, or on
  Swell (the LFO starts over each Swell bar; dim while the Swell is stopped, waiting for it). "mvmt 0%" in the rate's
  place: ALGORITHM's Movement is at 0, so nothing moves. The delay's send is on Mods, with its knob.
- **Mods**, the LFO moving other things, and cross-mod's amount:
  - **Speed mod** ("Speed" on the knob): the LFO speeds up and slows down on one of its own waves (up to 3 octaves either way).
  - **Source**: which LFO does that (I, II or both).
  - **Delay mod** ("Delay" on the knob): how far the LFO wobbles the delay's time (up to ±25%; negative the other way), on the wave black
    key 10 picks on the LFO pages. You hear it only with some delay mix (FX, the Space page).
  - **Cross-mod** ("X-mod" on the knob): how deep cross-mod goes (black key 11): how much drone I's sound moves drone II's filter; how
    far each drone's sound bends the other's pitch (**cross-FM**: a little roughens and thickens the two, a lot
    tangles them into one restless, metallic sound); or, with **Snarl**, how far the finished sound, echoes and all,
    bends both drones' pitch (growly with two different pitches; on a single plain drone it only reshapes the tone).
    Their pitches stay where you set them: the bending is around them.
  - The picture: the LFO's clock feeding LFO I and II (bent as Speed mod bends the clock), the Source wave's line
    looping straight back into the clock, and a line from the delay's wave to the delay's time; under it, cross-mod as it is now ("I moves II's cutoff", "bend each other",
    "Snarl" or off) and its key, F#5.
  - Movement (ALGORITHM) scales the delay's wobble too, as it does the LFO sends; not Speed mod or Cross-mod (see
    [LFO](lfo.md)).
- The LFO's rate and waves are on the top-left knobs; see [LFO](lfo.md).
