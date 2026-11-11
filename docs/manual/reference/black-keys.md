---
title: Black keys
intro: Switches, and the LFO's routing on the LFO pages
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

Dark means off, lit on. A key that goes round two or three choices that all play (the sub, the filter, an LFO
send's wave and target) glows dimly on its first choice, is lit on the second and winks on a third, so dark always
means off. They follow the page: on the LFO pages they set the LFO's routing. Normally they're grouped by meaning:
drone I (keys 1–3), the two global keys (4–5), drone II (6–8), the links (9–11).

When the screen names a key or button in a box ("needs [F#3]", "turn on [SEL] + [PLAY]"), its light blinks quickly
while the message shows, so you can find it.

**Hold SEL** for a moment and the screen draws the black keys as they sit on the keyboard, lit as on the panel,
with their groups named, and the eight gesture keys as the banks they pick (A to F, P1, P2), the one playing lit;
on the panel those eight keys light as the banks too. On a
page where a knob has a second job with SEL (Sub · Drive, Delay, Reverb) it waits longer, 1.5 s, and
once you've turned that job, until you let go, so SEL + a knob never flashes the map.
**Touch a black key while SEL is held** and the screen names it, its group and its setting ("I · G#3", "SUB -1
OCT"), without switching it: let go of SEL to play it. The map steps aside while you turn a knob or hold a white
key, and once SEL has done something else in a hold (SEL + PLAY, a bank, a knob) it waits until you let go.

<img src="../../img/page-keys.png" width="244" alt="The black keys' map while SEL is held: the keyboard with its black keys lit as on the panel and their groups named, and the key touched named with its setting">

| Key | Normally | On the LFO pages |
|---|---|---|
| 1 | I partner | which LFO moves I's pitch (I, II, both) |
| 2 | I sub: −1 oct, −2 oct, ⅓, ⅕, −3 oct, −4 oct | I's pitch send moves X-Wave instead |
| 3 | I filter: LP, BP, HP | which LFO moves I's cutoff (I, II, both) |
| 4 | pitch lock: scenes and Presets leave the drones' pitches where they are (on when you first start; the pitch knobs still tune); Wander comes with the scene | I's cutoff send moves resonance instead |
| 5 | LFO reset | LFO reset |
| 6 | II partner | which LFO moves II's pitch (I, II, both) |
| 7 | II sub: −1 oct, −2 oct, ⅓, ⅕, −3 oct, −4 oct | II's pitch send moves X-Wave instead |
| 8 | II filter: LP, BP, HP | which LFO moves II's cutoff (I, II, both) |
| 9 | pitch link (II follows I) | II's cutoff send moves resonance instead |
| 10 | hard sync (II to I) | which LFO moves the delay's time (I, II, both) |
| 11 | cross-mod: off (dark), I → II cutoff (lit), pitch I ↔ II (winking), Snarl: the finished sound bends both (winking) | the same |

- A **wave key** goes round with each tap: LFO I (dim), LFO II (lit), both (winking); a **target key**: pitch or
  cutoff (dim), X-Wave or resonance (lit). The screen says which LFO now
  moves what ("LFO II → I PITCH").
- **LFO reset:** tap to restart the LFO. Hold it half a second to lock the LFO to the Swell (it then
  restarts every bar; the key stays lit). Hold again to free it.
- With **pitch link** on, KNOB 3 sets the gap between the drones, in semitones and cents ("+7 st", "+8 st +20").
- A **filter key** goes round with each tap: low-pass (dim), band-pass (winking), high-pass (lit).
- A **sub key** goes round with each tap: −1 octave (dim), −2 octaves (lit), then the subharmonics ⅓
  and ⅕ of the pitch (winking). With the drone's OSC MIX at 0 (Sub · Drive) the sub isn't heard: the screen says
  so ("OSC MIX 0 · EDIT x2", EDIT in a box).
