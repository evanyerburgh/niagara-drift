---
title: Setup
intro: GLO: the room's settings and the Tuning
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Setup 1 | fade | morph | scene time | sync |
| Setup 2 | tuning | tune | stereo | pressure |

Everything on [[GLO]] belongs to the FM-1 and the room, not to a sound: scenes don't change it, except the Tuning,
which a sound carries.

## Setup 1

<img class="shot" src="../../img/screen-setup.png" width="240" alt="The Setup 1 page: the fade, morph and scene times, and Sync">

Each time knob gives most of its turn to slow times, a second about a quarter of the way.

- **Fade:** how long PLAY takes, up to 60 s (1.5 s to start).
- **Morph:** how long a black key or a new LFO wave takes to move the sound, 0.02–30 s (2 s to start).
- **Scene time:** how long every scene move takes (a tap, a blend pressed in at once, the way back), 0.1 s to
  5 minutes (about 8 s to start).
- **Sync:** **On** follows a MIDI clock, **Off** ignores it. Under it: "no clock", "120 BPM", "keeps 120" (stopped,
  at the last tempo) or "free time" (off).
- **The picture:** a row for each time, its knob's whole range across it, the change ending where the knob is.

## Setup 2

<img class="shot" src="../../img/screen-setup2.png" width="240" alt="The Setup 2 page: the Tuning's ring, Tune in Hz, the stereo width and the keys' pressure">

- **Tuning:** the notes the pitch knobs move through (below).
- **Tune:** A's pitch, 427 to 453 Hz (440 to start); 1 Hz a click, 0.1 with [[SEL]] ("432Hz").
- **Stereo:** from mono to drone I left, drone II right. A drone playing alone sits in the middle.
- **Pressure:** how hard you press the scene keys: up for a light touch, down for a firm one (50 as tuned). The
  picture shows the key you're pressing ("[C4] 62%").
- **The picture:** the Tuning and Tune as one ring: an octave round, a tick a note, A named, Tune turning the ring
  away from 440's mark. Inside, how many notes, and where the 1/1 is on the pure Tunings.

The speaker gets a low cut (about 120 Hz) for bass it can't play. Plug in headphones and the cut goes; unplug and it
comes back.

## Tunings

The knob goes round both ways:

| Tuning | What it is |
|---|---|
| **12-tone** | semitones |
| **Quarter** | quarter tones |
| **19-, 31-, 53-tone** | equal temperaments |
| **Just** | just intonation (Ptolemy's 5-limit) |
| **Harmonics** | the odd harmonics 1–15 of A in each octave: the drones as notes of one harmonic series |
| **LMY** | La Monte Young's tuning for *The Well-Tuned Piano* (built on 3s and 7s) |
| **Harrison** | Lou Harrison's tuning for his National Steel guitar (11-limit) |
| **Shrutis** | the 22 shrutis of Indian music |
| **Partch** | Harry Partch's 43 notes to the octave (11-limit) |

- **Just and Shruti move with drone I** (a movable do: drone I is do, or Sa). Its pitch knob picks the tonic in
  semitones; drone II, Pitch link, Wander and Chord are pure over it.
- **The others are built on A**, so Tune moves them. LMY, Harrison and Partch are placed as in Ableton's tuning files
  (Partch's own G at 392 Hz is Tune 441Hz). Their 1/1: LMY on C, Harrison and Partch on G, Harmonics on A.
- **Wander, Chord, Partner and Shift follow the Tuning.** On the 12-note Tunings Wander's scale plays the same keys
  with the Tuning's intervals; on the rest, each note is the Tuning's nearest to the pure interval. On Harmonics,
  Wander moves between all the harmonics. Shift steps note to note of the Tuning; a partner's interval is the
  Tuning's.
- <a id="intervals-on-a-tuning"></a>**Intervals are named in the Tuning's terms**, as composers and tuning files
  write them: steps on the equal ones ("11\19", "31\53"), ratios on the others ("3/2", "7/4"). Pitch's interval, a
  partner's and Shift's read this way, when the gap you hear is within 10 cents of the Tuning's interval; otherwise
  the familiar name ("5th"). Pitches still read as notes and cents ("C#3 +42").
- **A sound carries its Tuning.** A scene saved on LMY brings LMY back, even with Pitch lock on (the drones keep their
  pitch; the intervals around them change). A Preset plays on 12-tone unless it has a Tuning of its own. The Tuning
  changes half way through a scene's glide, and lets go of SEL's fine tuning; going back (undo, a key let go) brings
  it back.
- **It decides only where a turn goes:** a sound plays its saved pitches until you turn a knob; the first click goes
  to the Tuning's next note.
