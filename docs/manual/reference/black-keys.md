---
title: Black keys
intro: Switches, and the LFO's routing on the LFO pages
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## Black keys

The 11 black keys are switches, with two sets of jobs: one for the drones, on every page but the LFO pages, and one
for the LFO, while an LFO page is open. LFO reset ([[D#4]]) and cross-mod ([[F#5]]) do the same in both.

## Drone keys

On every other page, each drone has three keys, and five more set the links between them.

<img src="../../img/black-keys-drones.png" width="720" alt="The black keys for the drones: F#3 Partner, G#3 Sub, A#3 Filter for drone I; C#4 Pitch lock, D#4 LFO reset; F#4 Partner, G#4 Sub, A#4 Filter for drone II; C#5 Pitch link, D#5 Hard sync, F#5 X-mod">

Each drone's own keys:

| Switch | Drone I | Drone II | Choices |
|---|---|---|---|
| **Partner** | [[F#3]] | [[F#4]] | on or off |
| **Sub** | [[G#3]] | [[G#4]] | −1 oct, −2 oct, ⅓, ⅕, −3 oct, −4 oct |
| **Filter** | [[A#3]] | [[A#4]] | low-, band- or high-pass |

The keys for both:

| Key | Switch | Choices |
|---|---|---|
| [[C#4]] | **Pitch lock** | scenes and presets keep the drones' pitches (on to start) |
| [[D#4]] | **LFO reset** | restarts the LFO; hold to lock it to the Swell |
| [[C#5]] | **Pitch link** | drone II follows drone I |
| [[D#5]] | **Hard sync** | drone II to drone I |
| [[F#5]] | **Cross-mod** | off, I moves II's cutoff, bend each other, Snarl |

## LFO keys

On the [LFO pages](lfo-pages.md), each drone's four keys set its two sends.

<img src="../../img/black-keys-lfo.png" width="720" alt="The black keys on the LFO pages: F#3 Pitch LFO, G#3 Pitch to X-Wave, A#3 Cutoff LFO, C#4 Cutoff to reso for drone I; D#4 LFO reset; F#4, G#4, A#4, C#5 the same for drone II; D#5 Delay LFO; F#5 X-mod">

Each drone's own keys:

| Switch | Drone I | Drone II | Choices |
|---|---|---|---|
| **Pitch LFO** | [[F#3]] | [[F#4]] | Which LFO moves the pitch: I, II or both |
| **Pitch to X-Wave** | [[G#3]] | [[G#4]] | The pitch send moves X-Wave instead |
| **Cutoff LFO** | [[A#3]] | [[A#4]] | Which LFO moves the cutoff: I, II or both |
| **Cutoff to reso** | [[C#4]] | [[C#5]] | The cutoff send moves resonance instead |

The others:

| Key | Switch | Choices |
|---|---|---|
| [[D#5]] | **Delay LFO** | Which LFO moves the delay's time: I, II or both |
| [[D#4]] · [[F#5]] | **LFO reset**, **Cross-mod** | As on every other page |

## Finding a key while you play

<img class="shot" src="../../img/screen-keymap.png" width="240" alt="The black keys' map while SEL is held: the keys lit as on the panel, their groups named, the key touched named with its setting">

[[SEL]] + any black key shows what the key does, without switching it.

1. Hold [[SEL]]. The screen draws the black keys, lit as on the panel.
2. Still holding SEL, press a black key. The screen names it and its setting; nothing changes.
3. Let go of SEL, then tap the key to switch it.

## Good to know

**Lights:** dark is off, lit is on. A key of several choices shows which by its light:

| Key | Dim | Lit | Winking |
|---|---|---|---|
| **Filter** | low-pass | high-pass | band-pass |
| **Sub** | −1 oct | −2 oct | ⅓, ⅕ |
| **An LFO key** | LFO I | LFO II | both |

- **An LFO key** switched: the screen says what now moves what ("LFO II → I PITCH").
- **Pitch lock:** the pitch knobs still tune.
- **Pitch link:** KNOB 3 on Pitch sets the gap ("+7 st").
- **LFO reset:** hold half a second to lock the LFO to the Swell; hold again to free it.
- **A key the screen names in a box** ("needs [F#3]") blinks while the message shows.
