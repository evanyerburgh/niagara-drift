---
title: MIDI
intro: Notes, the clock and Sync
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

Over USB or the TRS jack, a note on **channel 1** tunes drone I to it, a note on **channel 2** drone II
(with Pitch link on, a channel 2 note sets the interval instead). It tunes, even with Pitch lock on; it doesn't start
or stop the drones.

**MIDI clock** (from a DAW or a drum machine, over USB or the TRS jack) puts Niagara Drift in time. After about two
beats it locks ("120 BPM · SWELL ON CLOCK"), and then:

| What | On the clock |
|---|---|
| **The Swell** (KNOB 1 on its page) | a breath a **Division**: 32 bars .. 1/4 |
| **The LFO** (SELECT) | a cycle a division: 32 bars .. 1/16 ("LFO CYCLE") |
| **The delay** (Time, Details page) | a **note value**: 1/16, 1/8T, 1/8, 1/4T, 1/8D, 1/4, 1/2T, 1/4D, 1/2, 1/2D, 1 bar (halved when longer than the delay holds) |
| **Wander** | its notes land on the beat (on the bar, when they're slow), still at random |
| **Scene time** (Setup 1) | in **bars**: 1 .. 16; a scene change ends on a bar line |

They follow the clock's beat however long it runs: a wobbly clock's tempo is smoothed, and nothing can drift from it.
**Start** (playing from the top) brings the Swell and the LFO back to their tops. While synced, scenes leave the
Swell's on / off and its division alone. When the clock stops, everything carries on at the last tempo ("CLOCK
STOPPED · KEEPS 120"). To get free time back, or to keep a clock out, turn **Sync** off (Setup 1, KNOB 4).
In Logic: File → Project Settings → Synchronization → MIDI, tick Clock for the FM-1.
