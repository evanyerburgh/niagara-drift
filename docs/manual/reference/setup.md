---
title: Setup
intro: GLO: the room's settings and the Tuning
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

<img src="../../img/page-setup.png" width="488" alt="The Setup 1 and 2 pages: the fade, morph and scene times and Sync; the Tuning's ring, Tune in Hz, the stereo width and the keys' pressure">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Setup 1 | fade | morph | scene time | sync |
| Setup 2 | tuning | tune | stereo | pressure |

Everything on GLO belongs to the FM-1 and the room, not to a sound: scenes don't change it, except the Tuning, which a sound carries.

- Each knob gives most of its turn to slow times, a second about a quarter of the way.
- **Fade** is how long PLAY takes (up to 60 s; 1.5 s to start); **Morph** is how long a black key or a new LFO
  wave (OCT) takes to move the sound to its new setting (0.02–30 s; 2 s to start); **Scene time** is how long every
  scene move takes: a tap, a crossfade pressed all the way in at once, and the way back (0.1 s to 5 minutes; about
  8 s to start). These are the FM-1's own settings, not a sound's: an FM-1 already set keeps its own. The picture
  is a row for each, its knob's whole range across it: the fade a line up from silence, Morph and the scenes a
  crossfade (a out, b in), each change ending where its knob is (a dot there while you turn it).
- **Sync** (KNOB 4): **On** follows a MIDI clock (see MIDI below), **Off** ignores it, so the Swell, the LFO, the
  delay, Wander and the Scene time run on their own knobs again. Under it, what the clock is doing: "no clock",
  "120 BPM", "keeps 120" (stopped or unplugged, at the last tempo) or "free time" (off).
- **Tune** is A's pitch in Hz, from 427 to 453 (440 to start): 1 Hz a click, 0.1 with SEL held ("432Hz").
  **Stereo** spreads the drones from mono to drone I left, drone II right; a drone playing alone (drone II out) sits in
  the middle.
- **The speaker** gets a low cut (about 120 Hz) for the deep bass it can't play, so it doesn't waste its loudness
  there. It's automatic: plug in headphones and the cut goes out, unplug and it comes back.
- **Pressure** is how hard you have to press the scene keys: up for a light touch, down for a firm one (50 as
  tuned).
- **Tuning** is the notes the pitch knobs move through: **12-tone** (semitones), **Quarter** tones, **19-**, **31-** or
  **53-tone** equal temperament, **Just** intonation (Ptolemy's 5-limit), **Harmonics** (the odd harmonics 1–15 of A,
  in each octave: the drones as notes of one harmonic series), **LMY**: La Monte Young's tuning for The Well-Tuned
  Piano (built on 3s and 7s, with its septimal intervals), **Harrison**: Lou Harrison's tuning for his National Steel
  guitar (11-limit), the 22 **Shrutis** of Indian music, or **Partch**: Harry Partch's 43 notes to the octave
  (11-limit): the equal ones by size, then the rest simplest to richest. The knob goes round both ways.
  - **The picture** is the Tuning and Tune as one ring: an octave clockwise, a tick a note, A named on its tick, and
    Tune turning the ring away from the small mark at the top (440). Inside, how many notes and, on the pure ones,
    where their 1/1 is (LMY on C, Harrison and Partch on G, Harmonics on A, Just and Shruti on drone I), its tick the
    long white one. The
    interval between the drones is on Pitch. Beside it Stereo, the drones' two waves as far apart as it puts them, and Pressure, what a press gives from light
    to firm, with the key you're pressing on it ("[C4] 62%").
  - **Just and Shruti move with drone I**, as just intonation does (a movable do; drone I is do, or Sa): drone I's
    knob picks the tonic in semitones, and drone II's knob, Pitch link, Wander and Chord are pure over it, wherever it
    is.
  - **The others are built on A**, so Tune moves them (432Hz for A at 432). LMY, Harrison and Partch are placed
    as in Ableton's tuning files, with A at 440 one of their notes (Partch's own, G at 392 Hz, is Tune 441Hz).
  - **Wander and Chord follow it too.** On 12-tone and Quarter they're as they've always been (Chord pure). On the
    other 12-note Tunings (Just, LMY, Harrison) Wander's scale plays the same keys, so a minor pentatonic on LMY takes
    LMY's own intervals; on the rest, each note is the Tuning's nearest to the pure interval (a 31-tone minor third is
    310 cents). On Harmonics, Wander moves between all the harmonics, whatever the scale.
  - <a id="intervals-on-a-tuning"></a>**Intervals on a Tuning are named in its own terms**, as composers and tuning
    files write them: on the equal ones by steps over the division ("11\19" is 19-tone's fifth, "31\53" 53-tone's),
    on the others by ratio ("3/2", "7/4", "11/9"). Pitch's interval, a partner's and Shift's read this way, so every
    click of a pitch knob changes the name. The name is the interval between the drones' nearest notes of the
    Tuning, shown when the gap you hear is within 10 cents of it (a drone left on D3, or SEL's fine tuning); further off,
    or on 12-tone, or a ratio too long to fit ("1323/1024"), it's the familiar name ("5th"). The pitches themselves still read as notes and cents ("C#3 +42").
  - **So do Partner and Shift.** A partner's interval is the Tuning's, over its own drone; Shift's knob steps from
    note to note of the Tuning over drone I, and each drone moves as many of the Tuning's notes from its own. A Shift
    set off the notes (with SEL, or a sound from before) moves its cents exactly.
  - **A sound carries its Tuning.** A scene stored on LMY brings LMY back when you play it, even with Pitch lock on
    (the lock keeps the drones' own pitch; the notes around them change: a partner's interval, Shift's steps, Wander's
    and Chord's notes become the new Tuning's). A Preset plays on 12-tone, as it was made, unless
    it has a Tuning of its own. The Tuning changes half way through the scene's glide, and lets go of SEL's fine
    tuning, as turning the knob does; a glide that goes back (undo, a scene key let go) brings that fine tuning back.
  - It only decides where a turn goes: a sound or scene plays its pitches as they were saved, until you turn the knob
    (the first click goes to the Tuning's next note).
