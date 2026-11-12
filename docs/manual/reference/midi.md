---
title: MIDI
intro: Notes, the clock and Sync
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

MIDI comes in over USB or the TRS jack.

## Notes

- **Channel 1** tunes drone I to the note, **channel 2** drone II (with Pitch link on, the interval instead).
- Notes tune, even with Pitch lock on; they don't start or stop the drones.

## Clock

A MIDI clock from a DAW or a drum machine puts Niagara Drift in time. After about two beats it locks ("120 BPM ·
SWELL ON CLOCK"):

| What | On the clock |
|---|---|
| **The Swell** (KNOB 1 on its page) | a breath a **Division**: 32 bars to 1/4 |
| **The LFO** (SELECT) | a cycle a division: 32 bars to 1/16 ("LFO CYCLE") |
| **The delay** (Time, Details) | a **note value**: 1/16, 1/8T, 1/8, 1/4T, 1/8D, 1/4, 1/2T, 1/4D, 1/2, 1/2D, 1 bar (halved when longer than the delay holds) |
| **Wander** | its notes land on the beat (on the bar, when slow), still at random |
| **Scene time** (Setup 1) | in **bars**, 1 to 16; a scene change ends on a bar line |

- **A wobbly clock** is smoothed, and nothing drifts from it however long it runs.
- **Start** brings the Swell and the LFO back to their tops.
- **While synced,** scenes leave the Swell's on / off and division alone.
- **When the clock stops,** everything carries on at the last tempo ("CLOCK STOPPED · KEEPS 120").
- **Sync** (Setup 1, KNOB 4) off: free time, a clock ignored.
- **In Logic:** File → Project Settings → Synchronization → MIDI, tick Clock for the FM-1.

Recipe: [Play in time with a MIDI clock](../recipes/performing.md#clock)
