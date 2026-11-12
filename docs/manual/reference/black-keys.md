---
title: Black keys
intro: Switches, and the LFO's routing on the LFO pages
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

| Key | Normally | On the LFO pages |
|---|---|---|
| 1 · F#3 | I partner | which LFO moves I's pitch (I, II, both) |
| 2 · G#3 | I sub: −1 oct, −2 oct, ⅓, ⅕, −3 oct, −4 oct | I's pitch send moves X-Wave instead |
| 3 · A#3 | I filter: LP, BP, HP | which LFO moves I's cutoff (I, II, both) |
| 4 · C#4 | pitch lock: scenes and Presets keep the drones' pitches (on to start) | I's cutoff send moves resonance instead |
| 5 · D#4 | LFO reset | LFO reset |
| 6 · F#4 | II partner | which LFO moves II's pitch (I, II, both) |
| 7 · G#4 | II sub: −1 oct, −2 oct, ⅓, ⅕, −3 oct, −4 oct | II's pitch send moves X-Wave instead |
| 8 · A#4 | II filter: LP, BP, HP | which LFO moves II's cutoff (I, II, both) |
| 9 · C#5 | pitch link (II follows I) | II's cutoff send moves resonance instead |
| 10 · D#5 | hard sync (II to I) | which LFO moves the delay's time (I, II, both) |
| 11 · F#5 | cross-mod: off, I moves II's cutoff, bend each other, Snarl | the same |

Grouped by meaning: drone I (keys 1–3), the two global keys (4–5), drone II (6–8), the links (9–11).

## Lights

- **Dark is off, lit is on.** A key of several choices glows dimly on its first, is lit on the second, winks on a
  third.
- **A key the screen names in a box** ("needs [F#3]") blinks while the message shows, so you can find it.

## Keys of choices

- **Filter:** low-pass (dim), band-pass (winking), high-pass (lit).
- **Sub:** −1 octave (dim), −2 octaves (lit), then the subharmonics ⅓ and ⅕ of the pitch (winking). With Osc/Sub at 0
  the sub isn't heard, and the screen says so.
- **On the LFO pages, a wave key:** LFO I (dim), LFO II (lit), both (winking). **A target key:** pitch or cutoff
  (dim), X-Wave or resonance (lit). The screen says what now moves what ("LFO II → I PITCH").
- **Pitch lock** keeps the drones' pitches through scenes and Presets; the pitch knobs still tune. Wander comes with
  the scene.
- **Pitch link:** drone II follows drone I; KNOB 3 sets the gap ("+7 st", "+8 st +20").
- **LFO reset:** tap to restart the LFO. Hold half a second to lock it to the Swell (it restarts every bar, the key
  lit); hold again to free it.

## The map

<img class="shot" src="../../img/screen-keymap.png" width="240" alt="The black keys' map while SEL is held: the keys lit as on the panel, their groups named, the key touched named with its setting">

**Hold [[SEL]]** a moment: the screen draws the black keys as on the keyboard, lit as on the panel, their groups
named, and the gesture keys as the banks they pick. **Touch a black key** while SEL is held and the screen names it
and its setting ("I · G#3", "SUB -1 OCT") without switching it. On a page where SEL gives a knob a second job, the map
waits 1.5 s. It steps aside while you turn a knob or hold a white key.
