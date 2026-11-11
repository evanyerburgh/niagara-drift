---
title: About
intro: Two drones that breathe and drift on their own, as open-source firmware for the M-VAVE FM-1
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## What it does

- **16 Presets** to start from, each a different side of the instrument and each moving over a minute: Niagara,
  Embers, Vespers, Dreamhouse, Abyss, Seance, Sonar and Sibyl on keys 1–8 of the Presets bank; Howl, Cistern, Piston,
  Ashram, Telegraph, Tape, Falling and Bare on keys 1–8 of the second.
- **Eight gestures**, each several settings moving together on top of your sound: **Weight**, **Shift**, **Clean** and
  **Wash** on the top keys, in while you hold and out as slowly as you held; **Tick**, **Chord**, **Snarl** and
  **Stutter** on keys 9–12, as deep as you press. Tap any of them in to stay.
- **Scenes**: 8 on the white keys in each of six banks (48 of yours), plus the Presets' two, recalled gliding over the
  Scene time, or **crossfaded into by pressing the key harder**; a **motion loop** that records your knob moves, gestures and scenes.
- **It moves on its own**: the LFO, the Swell, **Drift**, **Wander** (drone II's slow random melody on a scale) and
  **Age** (tape wear) keep it breathing without a hand on it.
- **Two drones**, each a main oscillator that blends sine → triangle → saw → square (X-WAVE), a
  partner oscillator at an interval (tempered or just) with detune, a sub one to four octaves down
  (the lowest two click under a low drone) or a subharmonic (⅓, ⅕ of the pitch), and a white or pink noise, into its own self-oscillating
  filter (low-, band- or high-pass) with drive. Pitch is free between notes, from 2 Hz to 1.6 kHz.
- **Tunings**: the pitch knobs move through the notes of a Tuning, and the screen names the intervals in its own
  terms: 12-tone, quarter tones, 19-, 31- and 53-tone equal temperament, just intonation, the harmonics, La Monte
  Young's Well-Tuned Piano, Lou Harrison's, the 22 shrutis of Indian music, or Harry Partch's 43 notes. A sound
  carries its Tuning.
- **Links** between them: pitch link, hard sync, and cross-mod: drone I's sound moving drone II's cutoff, or each
  drone's sound bending the other's pitch (cross-FM).
- **The Ultra LFO**: one rate, two waves of ten shapes each (one of them truly
  random, two stepped, AND and OR), routed to each drone's pitch or X-WAVE and cutoff or resonance, alone or summed. It can
  move its own rate and lock to the Swell.
- **Movement** (ALGORITHM): all the LFO's depths down or up together, from still to double.
- **The Swell**: both drones breathing in time, from a slow soft swell to a hard rhythmic gate, with swing.
- **Effects**: a stereo delay of six types (Clean, Wide, Ping-pong, Tape, and Grain and Self, whose
  own echoes move their time; the LFO can warble it too), a drive
  after it (the drones and their echoes distorted together), into a reverb of five (Room, Hall,
  Cathedral, Chorus hall, Shimmer) whose length goes to Inf, a freeze.
- **Feedback** (the finished sound fed back into the drones) and **Character** (clean to vintage).
- **Drift**: the drones are never quite still; pitch, cutoff, timbre, level and detune slowly wander.
- **Morph**: buttons and keys never jump: the sound moves to a new setting over a time you set.

## Technical details

- **Audio:** 44.1 kHz, stereo, rendered in 32-bit floating point and sent to the FM-1's converter at 24 bits; a block
  every 5.8 ms. Past about four-fifths of MASTER it drives into a soft overdrive at about the same loudness.
- **Keys:** the white keys sense pressure: scenes crossfade, and keys 9–12 play their gestures, as hard as you press.
- **MIDI**, over USB or the TRS jack: notes on channels 1 and 2 tune the two drones; a MIDI clock puts the Swell, the
  LFO, the delay, Wander and the scenes in time (see [MIDI](reference/midi.md)).
- **Your sounds:** six banks of eight scenes, plus the sixteen Presets; the installers back them up before they install,
  and a restore is safe even if the power goes off halfway (see [Help](help.md)).
- **The screen, out loud:** with the [FM-1 Screen Reader](reference/screen-reader.md), it says what its screen shows.
- **In the browser:** the emulator on the front page is the firmware's own code, compiled for the browser, running at
  44.1 kHz with sound; SAVE there keeps your sound in that browser. Running FM-1 firmware in a browser was
  [Groove OS](https://www.groove-os.com/)'s idea.
- **Open source:** GPL-3.0; the code, the host simulator and the tests are on
  [GitHub](https://github.com/evanyerburgh/niagara-drift).

## Where it came from

Niagara Drift is named for *Day of Niagara* (the Theatre of Eternal Music, 1965) and La Monte Young's *Drift Studies*.
It began from ideas in the Stylophone CPM DS-2, the Moog Subharmonicon and the Soma Lyra-8, and has grown into an
instrument of its own.

It's built on [FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x)
by Charles Vestal, and on [Felucca](https://github.com/hugelton/Felucca)'s FM-1 platform by Leo Kuroshita (Hügelton
Instruments), all GPL-3.0. Key pressure is read as in [MONK-1](https://github.com/charlesvestal/fm1-monk) by Charles
Vestal; the delay and reverb follow [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1)'s design by Quixotic7,
whose idea the scene crossfade was; the safe restore's flash core and the screen reader come from Charles Vestal's
[fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0). The full list is in
[LICENSING.md](../../LICENSING.md). The site is set in Inter Tight, IBM Plex Mono and Barlow Semi Condensed (SIL OFL).

Niagara Drift is made by Evan Yerburgh. It isn't made or endorsed by M-VAVE, Hügelton Instruments or the makers of
any instrument it began from.
