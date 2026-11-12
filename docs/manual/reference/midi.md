---
title: MIDI
intro: Notes, the clock and Sync
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## MIDI

Notes tune the drones and a clock puts them in time, over USB or the TRS jack.

## MIDI in

### Notes

Channel 1 tunes drone I, channel 2 drone II.

### Clock

A MIDI clock from a DAW or a drum machine. After about two beats it locks ("120 BPM").

### Sync

Setup 1, KNOB 4: on follows a clock, off ignores it.

## On a clock

| What | On the clock |
|---|---|
| **The Swell** (KNOB 1 on its page) | a breath a **Division**: 32 bars to 1/4 |
| **The LFO** (SELECT) | a cycle a division: 32 bars to 1/16 ("LFO CYCLE") |
| **The delay** (Time, Details) | a **note value**: 1/16, 1/8T, 1/8, 1/4T, 1/8D, 1/4, 1/2T, 1/4D, 1/2, 1/2D, 1 bar |
| **Wander** | its notes land on the beat (on the bar, when slow), still at random |
| **Scene time** (Setup 1) | in **bars**, 1 to 16; a scene change ends on a bar line |

## Good to know

- **Notes tune,** even with Pitch lock on; they don't start or stop the drones. With Pitch link on, channel 2 sets the
  interval.
- **A wobbly clock** is smoothed, and nothing drifts from it however long it runs.
- **Start** brings the Swell and the LFO back to their tops.
- **While synced,** scenes leave the Swell's on / off and division alone.
- **When the clock stops,** everything carries on at the last tempo ("KEEPS 120").
- **A delay note value** longer than the delay holds is halved.
- **In Logic:** File → Project Settings → Synchronization → MIDI, tick Clock for the FM-1.
- Recipe: [Play in time with a MIDI clock](../recipes/performing.md#clock)
