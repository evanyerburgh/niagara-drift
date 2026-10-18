# Niagara Drift

**A drone synth for the M-VAVE FM-1.**

> **This page describes v0.2, in development.** The version you can download is the 0.1 beta: its manual is
> [the README of 0.1](https://github.com/evanyerburgh/niagara-drift/blob/v0.1-beta/README.md), and the
> [website](https://evanyerburgh.github.io/niagara-drift/) matches it until v0.2 is released.

Niagara Drift is open-source firmware that turns the M-VAVE
FM-1 into a drone synthesizer: two always-on drones you tune, shape and move rather than play note by
note. It's inspired by the Stylophone CPM DS-2, the Moog Subharmonicon and the Soma Lyra-8.

![The FM-1 with what every control does in Niagara Drift written on it](docs/img/panel.png)

*Your FM-1 with Niagara Drift installed: what each knob, button and key now does. Amber is drone I,
teal drone II, lilac the LFO. The black keys are switches (lit = on; on the LFO pages they set the
LFO's routing, and X-mod lets drone I move drone II's filter); the lower 12 white keys hold scenes, in
banks you pick with SEL and the top four.*

**Status: 0.1 beta.** It runs on the FM-1 (tested on one unit so far) and in the browser:
[try it](https://evanyerburgh.github.io/niagara-drift/) without an FM-1, the same code with sound.

## What it does

- **Two drones**, each a main oscillator that blends sine → triangle → saw → square (X-WAVE), a
  partner oscillator at an interval (tempered or just) with detune, a sub one or two octaves down
  or a subharmonic (⅓, ⅕ of the pitch), and a white or pink noise, into its own self-oscillating
  filter (low-, band- or high-pass) with drive. Pitch is free between notes, from 27 Hz to 1.6 kHz.
- **Links** between them: pitch link, hard sync, and drone I's sound moving drone II's cutoff.
- **The Ultra LFO**, after the DS-2's: one rate, two waves of eight shapes each (one of them truly
  random), routed to each drone's pitch or X-WAVE and cutoff or resonance, alone or summed. It can
  move its own rate and lock to the Swell.
- **Movement** (ALGORITHM): all the LFO's depths down or up together, from still to double.
- **The Swell**: both drones breathing in time, from a slow soft swell to a hard rhythmic gate, with swing.
- **Effects**: a stereo delay of four types (Tape, Wide, Ping-pong, Clean; the LFO can warble its time) into
  a reverb of four (Room, Hall, Cathedral, Chorus hall) whose length goes to Inf, a freeze.
- **Feedback** (the finished sound fed back into the drones) and **Character** (clean to vintage).
- **Drift**: the drones are never quite still; pitch, cutoff, timbre, level and detune slowly wander.
- **Morph**: buttons and keys never jump: the sound moves to a new setting over a time you set.
- **Scenes**: 12 on the white keys in each of three banks (36 of yours), plus a Presets bank, recalled
  with a morph, or **crossfaded into by pressing the key harder**; a **motion loop** that records your knob
  moves.

## Installing

1. Open the [web installer](https://evanyerburgh.github.io/niagara-drift/install/) in **Chrome or Edge**
   (they can talk to the FM-1 over USB; Safari and Firefox can't).
2. Connect the FM-1 straight to the computer with a USB **data** cable (no hub) and switch it on.
3. Press **Install** and allow MIDI when the browser asks. It takes about two minutes; don't touch the cable
   or the FM-1 until it says Done. Then press PLAY.

Or from a terminal, with the file from the [releases](https://github.com/evanyerburgh/niagara-drift/releases):
`pip3 install mido python-rtmidi`, then `python3 tools/fm1_install.py niagara-drift-0.1-beta.fwsc`.

**What it replaces:** Niagara Drift takes the place of M-VAVE's firmware, so the FM-1's own sounds and
sequencer are gone until you go back (below).

**Updating:** when there's a new version, run the installer again the same way. Your sound and scenes are
kept, unless that version's notes in the [changelog](CHANGELOG.md) say otherwise.

**Backing up:** from v0.2, `python3 tools/fm1_install.py --backup` saves your sound, scenes and settings to a
file (in `~/niagara-drift-backups/`), and `--restore FILE` puts them back (the FM-1 restarts). The terminal
installer saves one by itself before every install. Back up before trying another firmware, and restore
when you come back. A backup is for the same FM-1: it carries that unit's knob calibration too.

**Going back:** the installer's "Back to M-VAVE's firmware" writes M-VAVE's own FM-1 V15 back over the
same cable (or use M-VAVE's updater, M-UPGRADE, from [m-vave.com/download](https://www.m-vave.com/download)).
If it ever fails to start twice in a row it comes up in **safe mode** (no sound, USB on), where the installer
still works. If an FM-1 won't start at all, a Mac can still put the stock firmware back: see
[BUILDING.md](BUILDING.md#install). Installing firmware is at your own risk; it's a beta.

## Playing it

**KNOB 1–2 are always drone I, KNOB 3–4 always drone II.** The page buttons change what they do;
the screen shows each page's settings as a picture, and the value of the knob you're turning.

| | |
|---|---|
| **PLAY** | fade the drones in or out; **SEL + PLAY** fades drone II alone out or back in |
| **OCT− / OCT+** | the LFO's **Wave I** / **Wave II**: tap for the next shape, round and round; **SEL + OCT** for the one before. Each light pulses with its wave |
| **SAVE** | save the sound, at once. While the drone plays, the sound dips out and back over about 50 ms around the write; SAVE glows dimly while something isn't written yet |
| **SEL** (hold) | fine: a pitch knob moves in single cents, cutoff and resonance in tenths |

### Pages

Tap a page button and the knobs stay on that page. Hold it to peek: turn a knob, let go, and you're
back. Tap it again for its second page; "1/2" by the page's name shows which of its pages you're on.
Each page's screen shows a picture of what its knobs do, and the four values below it. Every knob
glides: a slow turn is a smooth sweep, never steps; turn faster to go further.

#### Play · HOME

<img src="docs/img/page-play.png" width="488" alt="The Play 1 and Play 2 pages: each drone's waveform; each drone's level and filter curve">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Play 1 | I pitch | I cutoff | II pitch | II cutoff |
| Play 2 | I level | I resonance | II level | II resonance |

- **MASTER**: past about four-fifths of its turn it drives into overdrive, at about the same loudness.

#### Tone · ENV

<img src="docs/img/page-tone.png" width="488" alt="The Tone 1 and Tone 2 pages: each drone's noise through its filter, and its colour; the character, the feedback, Age and Drift">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Tone 1 | I noise | I colour | II noise | II colour |
| Tone 2 | character | feedback | age | drift |

- **Noise** is the noise's level beside the oscillators, as in a synth's mixer: they stay as they are.
  It's heard all along the knob, and at the top it's as loud as them (100 is level with them, 50 −6 dB, 25 −12). It
  goes through the drone's filter with them, so the cutoff, resonance, LFO and Drift shape it.
- **Colour** blends from white (left) to pink (right).
- **Character** takes the oscillators from clean to vintage: softer edges, a bent saw, a little pitch
  jitter, the two drones slightly apart, a whisper of noise.
- **Feedback** sends the finished sound (after the delay and reverb) back into the drones: a resonant,
  thickening colour that grows with the knob (more with the delay up), rough at the top but never louder than
  the sound without it; it fades with the drones when you stop.
- **Age** wears the sound like an old tape: darker, a slow wow in the pitch, a little hiss, more character. It
  wears in slowly (about 20 s from none to full) and wears out as slowly when you turn it back.
- **Drift** keeps the drones from ever being quite still: besides a little pitch wander, each drone's
  cutoff, timbre (X-Wave), level and partner detune slowly wander on their own, around where you've set
  them, never repeating. Gentle by default; 0 holds everything exactly where you put it.

#### Osc · EDIT

<img src="docs/img/page-osc.png" width="488" alt="The Osc and Source pages: each drone's waveform on the sine to square scale, and its pulse width; each drone's sound through its drive">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Osc | I X-Wave | I PW (pulse width) | II X-Wave | II PW |
| Source | I osc mix | I drive | II osc mix | II drive |

- **X-Wave** blends sine → triangle → saw → square; **PW** is the square's pulse width.
- **Osc mix** goes from the oscillators to the sub; **drive** pushes each drone into its filter.

#### Partner · SEQ

<img src="docs/img/page-seq.png" width="488" alt="The Partner page; Moves: Shift's move, Wander, drone II's melody, and its scale">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Partner | I interval | I detune | II interval | II detune |
| Moves | Shift's move | glide | II wander | Wander's scale |

- **Shift's move** is where the Shift gesture takes the drones: from a fifth down through none in the middle to a
  fifth up, 10 cents a detent, 1 with SEL held; it lights while Shift (E5) is in. See [The macros](#the-macros).
- **Wander** gives drone II a slow random melody: it jumps from note to note in no order, the notes of a scale on
  drone I (in any octave, up to an octave from its own pitch), and only now and then comes home. The knob is **how
  often**, at random times within a range: 25–30 s apart at the bottom, 0.5–2 s at the top; the screen shows the
  range. It doesn't glide by itself: **Glide** slides it. At 0 it's off.
- **Glide** is how long a drone takes to reach a new pitch (up to 10 s; off by default): a pitch knob, a scene,
  Wander's notes.
- **Wander's scale**: minor pentatonic, major pentatonic, fifths and octaves, whole tone, Dorian, or just (the
  harmonics 1–8 of drone I, pure: the root, the pure major third, fifth and the seventh harmonic).

- The partner oscillator sits an interval above or below each drone; **detune** makes them beat.
- Each **interval** comes twice: tempered, then **just** ("+5th just"), a pure ratio (a fifth is exactly 3/2) that
  doesn't beat; detune still adds beating on purpose. Octaves and unison are both at once.

#### LFO · LFO

<img src="docs/img/page-lfo.png" width="488" alt="The LFO depths and LFO mods pages: the two waves and where they go; the rate modulation, cross-mod and delay LFO">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| LFO depths | I osc depth | I filter depth | II osc depth | II filter depth |
| LFO mods | rate mod | mod by (wave) | I → II cutoff amount | delay LFO depth |

- **Depths** and **rate mod** are centred: the middle is none.
- **LFO mods**, the LFO moving other things:
  - **Rate mod**: the LFO speeds up and slows down on one of its own waves (up to 3 octaves either way).
  - **Mod by**: which wave does that (I, II or both).
  - **X-mod amt**: how much drone I's sound moves drone II's filter, while the cross-mod switch (black key 11) is on.
  - **Dly LFO**: how far the LFO wobbles the delay's time (up to ±25%; negative the other way), on the wave black
    key 10 picks on the LFO pages. You hear it only with some delay mix (FX, the Delay page).
  - Movement (ALGORITHM) scales the delay's wobble too, as it does the LFO depths; not Rate mod or X-mod (see
    [The LFO](#the-lfo)).
- The LFO's rate and waves are on the top-left knobs; see [The LFO](#the-lfo) below.

#### Swell · ARP

<img src="docs/img/page-swell.png" width="244" alt="The Swell page: the swell's shape over two bars">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Swell | rate | depth | shape | swing |

- Tap ARP to start or stop the Swell; tap it twice to see this page.
- **The Swell** runs from a 33-second breath to a buzz (the starting sound has it on: a slow breath, 40 deep).
- **Depth** is how far it dips, in dB (down to −30; all the way, silence). **Shape** firms the breath up into
  a pulse, and only at the top into a hard gate.

#### Delay and Reverb · FX

<img src="docs/img/page-effects.png" width="488" alt="The Delay and Reverb pages: the echoes on the left and right, and the reverb's tail">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Delay | type | time | repeats | delay mix |
| Reverb | type | length | damp | reverb mix |

- **Delay types**: **Tape** (the same on both sides, warm, a gentle wow), **Wide** (stereo: the right side's
  echoes come at three-quarters of the left's time), **Ping-pong** (the echoes bounce left and right),
  **Clean** (bright, with no wow, so a just-tuned drone stays still). Changing type glides.
- **Time** is 70–450 ms, and turning it bends the echoes' pitch, as tape does. **Repeats** past about 85 runs
  away into self-oscillation ("runaway" on the screen), held by a soft clip.
- **Reverb types**: **Room**, **Hall**, **Cathedral** (the largest), **Chorus hall** (a hall that shimmers).
  They differ in size, distance and movement; changing type glides.
- **Length** is how long the tail takes to die away, 0.3 to 40 seconds. At the top, **Inf**, it never does: a
  freeze. Press PLAY to stop and the freeze lets go, over about twice the Fade time.
- **Damp** darkens the tail (out at Inf, so a freeze holds what's in it).
- The delay and reverb are ChoralRoot's design (see [Credits](#credits)).

#### Setup · GLO

<img src="docs/img/page-setup.png" width="488" alt="The Setup 1 and 2 pages: the fade, Morph and scene switch times; the tune, stereo width, the speaker's low cut and the keys' pressure">

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| Setup 1 | fade | morph | scene switch | |
| Setup 2 | tune | stereo | speaker (low cut) | pressure |

Everything on GLO belongs to the FM-1 and the room, not to a sound: scenes don't change it.

- **Fade** is how long PLAY takes (up to 60 s); **Morph** is the switch time for buttons and keys: how long they
  take to move the sound to its new setting (0.02–30 s); **scene switch** how long the switch to a scene takes
  (0.1 s to 5 minutes).
- **Tune** moves everything up or down by up to 50 cents. **Stereo** spreads the drones from mono to drone I
  left, drone II right.
- **Speaker** cuts the deep bass a small speaker can't play (40–240 Hz); leave it off on headphones.
- **Pressure** is how hard you have to press the scene keys: up for a light touch, down for a firm one (50 as
  tuned).

### The LFO

| Knob | Sets |
|---|---|
| SELECT | the rate, from one cycle in 50 seconds to 70 Hz (below 0.1 Hz the screen shows a cycle's length, "21s") |
| ALGORITHM | the **Movement**: every LFO depth together, from still (0%) to double (200%) |
| OCT− · OCT+ | Wave I's · Wave II's shape: tap for the next (or off), round and round; SEL + OCT for the one before |

One clock, two waves; the OCT− and OCT+ lights pulse with them. The last shape on each is random.

**Movement** (ALGORITHM) is how much of all that you hear: it scales every LFO depth together, as you've set
them. At 0% the LFO moves nothing, whatever the depths say; at 100% (the middle) the depths are as set; at 200%
they're twice as deep. It can only scale depths you've set: the starting sound breathes its filters deeply
and slowly, so Movement is heard at once; set an osc depth to hear it move the pitch too.

| Movement scales | Movement leaves alone |
|---|---|
| **LFO depths**: all four knobs (each drone's osc depth and filter depth) | **LFO mods: Rate mod** (how much the LFO speeds itself up) |
| **LFO mods: Dly LFO** (the delay's wobble) | **LFO mods: X-mod amt** (drone I moving drone II's filter: that's sound, not the LFO) |
| | the LFO's rate, its shapes, and where the waves go (the black keys on the LFO pages) |
 Turning a
depth that can't be heard says why on the screen (Movement at 0%, or its wave off).
**PRESETS** is **Macro time**: how fast every gesture moves (see below).

### The macros

The top four white keys are gestures, each a few settings together, by set amounts, on top of your sound.
Your own settings stay as they are underneath: when a gesture has gone, it's gone without a trace. A gesture's
parts come in one after another, and go in their own order when you let go. Scenes remember which are latched.

| Gesture | What it moves | Key to start |
|---|---|---|
| **Weight** | a full, warm bass: the sub comes up through the swell, the resonance and noise give way, the cutoffs ease down a little, then drive and the vintage character round it off. Let go: the brightness comes back first, the sub lingers | D5 |
| **Shift** | a new chord, a voice at a time: drone I glides to the move you set (anywhere from a fifth down to a fifth up, in cents), drone II follows. Let go: drone I comes back first | E5 |
| **Clean** | back to a clean sound, your notes kept: the grit leaves first (drive, character, feedback, noise, cross-mod, hard sync), then the movement (Movement, Drift, the Swell), then the effects (the repeats, both mixes, the reverb's length). Let go: the effects come back first, the grit last | F5 |
| **Wash** | the delay first, then the reverb opens and grows longer and darker (toward Inf), each only up to where Wash aims (if your own effects are already further up, it leaves them be; the repeats stop short of the delay's howl), the dry sound dips last and, at the top, breathes slowly in and out of the cloud. Let go: the dry comes back first, the tails ring on and go last | G5 |

- **The gestures** are the top four white keys, always the same: D5 Weight, E5 Shift, F5 Clean, G5 Wash (they
  don't sense pressure). **Hold** one and it comes in while you
  hold; **let go** and it goes, over as long as you held it (hold 2 s, it leaves in 2 s; hold 10 s, it drifts out
  over 10 s). **Tap** it (a press under half a second) and it goes all the way and stays; tap again and it goes. **SEL held as you let go** keeps
  it where it is. Its key glows dimly, and lights as it comes in.
- **PRESETS is Macro time**: it speeds up or slows down every gesture, coming in and going out, from 10 times faster
  to 10 times slower (the middle: each gesture's own time: Weight 2.8 s, Shift 3.9 s, the others 3.3 s). Turn it
  while a gesture moves to change it on the way.
- **The LFOs step aside**: while a gesture is in, the LFO that moves the same thing eases off (the filter LFO for
  Weight's cutoff, the oscillator LFO for Shift's pitch), so
  a high **Movement** doesn't wobble what the gesture is doing. **Wash** also eases the filter LFOs to a third, so
  their sweeps don't pile up in the reverb as surges. They come back as the gesture goes.
- Turning a knob while a scene switch is on its way lets go of that setting; the rest still lands.

### Black keys: switches

Lit means on. They follow the page: on the LFO pages they set the LFO's routing. Normally they're grouped
by meaning: drone I (keys 1–3), the two global keys (4–5), drone II (6–8), the links (9–11).

**Hold SEL** for a moment and the screen shows what every black key does on this page, and how each is set,
in the panel's groups. It steps aside while you turn a knob or hold a white key.

<img src="docs/img/page-keys.png" width="244" alt="The black keys' map while SEL is held: each group of keys, lit as on the panel, with its name and setting">

| Key | Normally | On the LFO pages |
|---|---|---|
| 1 | I partner | I osc wave (I, II, both) |
| 2 | I sub: −1 oct, −2 oct, ⅓, ⅕ | I osc LFO → X-Wave (instead of pitch) |
| 3 | I filter: LP, BP, HP | I filter LFO → resonance (instead of cutoff) |
| 4 | pitch lock (the pitch knobs do nothing, and scenes leave the pitches where they are) | I filter wave (I, II, both) |
| 5 | LFO reset | LFO reset |
| 6 | II partner | II osc wave (I, II, both) |
| 7 | II sub: −1 oct, −2 oct, ⅓, ⅕ | II osc LFO → X-Wave |
| 8 | II filter: LP, BP, HP | II filter LFO → resonance |
| 9 | pitch link (II follows I) | II filter wave (I, II, both) |
| 10 | hard sync (II to I) | delay wave (I, II, both) |
| 11 | I → II cutoff | I → II cutoff |

- A **wave key** goes round with each tap: Wave I (dark), Wave II (lit), both (winking).
- **LFO reset:** tap to restart the LFO. Hold it half a second to lock the LFO to the Swell (it then
  restarts every bar; the key stays lit). Hold again to free it.
- With **pitch link** on, KNOB 3 sets the gap between the drones.
- A **filter key** goes round with each tap: low-pass (dark), band-pass (winking), high-pass (lit).
- A **sub key** goes round with each tap: −1 octave (dark), −2 octaves (lit), then the subharmonics ⅓
  and ⅕ of the pitch (winking).

### White keys, ARP and REC

| | |
|---|---|
| **White key** (the lower 12, F3–C5): **tap** | morph to the scene stored there in the current bank, over the scene switch time, and stay (turn a knob to stop part way). A tap is anything under a quarter of a second, however hard. A scene keeps the sound, not the room: Speaker, Pressure, Stereo, Tune, Pitch lock, Fade, Morph and Scene switch stay as they are |
| **Hold and press** a scene key | a **crossfade**: the harder you press, the further the sound moves from where it was into that scene (a light rest does nothing). The key gets brighter as you go in; the screen says how far ("A5 40%"). **Let go** and it glides back to where it began, over the Morph time. Press it again on the way back and you pick up from there |
| **Press all the way** (and hold a moment) | the scene is **taken**: the key flashes, and letting go keeps it |
| **Press SEL while holding**, then let go | the blend **holds** where it is from the moment SEL goes down (lifting your finger can't move it), and **stays** when you let go: a sound of its own between the two |
| **SAVE, then a white key** | hold SAVE first, then tap the key: the sound is stored there (a key pressed first goes to its scene instead; lit keys hold a scene; the key of the scene you're on blinks slowly, until you turn a knob or flip a switch; while playing, the sound dips for a moment as it's written). Not into the Presets: they're read-only (pick bank A, B or C first: SEL + D5, E5 or F5) |
| **SEL + a top white key** | picks the bank the 12 scene keys play: D5 bank **A**, E5 **B**, F5 **C** (yours), G5 the **Presets** (the starting sound and a deep bass sound for now; more to come). The screen names scenes by bank: "B5" |
| **Top white key** (D5–G5) alone | a **gesture**: Weight, Shift, Clean, Wash (see [The macros](#the-macros)) |
| **ARP** (tap) | the Swell on or off: both drones chopped in time, and the ARP light breathes with it. Hold ARP to peek at its page; tap twice quickly to stay on it |
| **REC** | tap to record your knob moves, tap to loop them, tap to stop (up to 5 minutes, the time shown by REC in the top bar; all four knobs turning fast for minutes fills it sooner, and it starts looping there, "FULL"; each pass glides back to the start) |

### MIDI

Over USB or the TRS jack, a note on **channel 1** tunes drone I to it, a note on **channel 2** drone II
(with Pitch link on, a channel 2 note sets the interval instead). It tunes; it doesn't start or stop the drones.

### Trying it in the browser

[Play it on the website](https://evanyerburgh.github.io/niagara-drift/): the firmware itself, compiled for the
browser, with sound. SAVE there keeps your sound in that browser. Working on the code, `./emulator.command`
builds the same page from your checkout, serves it at http://localhost:8765 and opens it.

| Do | Mouse | Keyboard |
|---|---|---|
| Turn a knob | drag up or down, or scroll | ↑ ↓ LFO rate, ← → Movement |
| Hold a button | Shift-click or right-click (click again to let go) | |
| PLAY · SAVE (hold) · SEL (hold) | | Space · Enter · C |
| HOME · EDIT · Wave I · Wave II (OCT− · OCT+) | | Esc · V · Z · X |
| Black keys 1–11 | click | 1 … 0, − |
| Scenes 1–12 · gestures | click | Q … I, A … F · G … K |

## Problems and ideas

It's a beta, tested on one FM-1 so far. If something goes wrong, sounds wrong or could be better, please
[open an issue](https://github.com/evanyerburgh/niagara-drift/issues): say what you did, what happened, and
what the startup screen says the version is.

## Building and testing

See [BUILDING.md](BUILDING.md) for the toolchain, including the extra steps on a Mac.

```
tests/run_tests.sh      # the instrument, the platform pieces, the app in a simulator
./emulator.command      # the same code in the browser: builds it, serves it, opens it
./build.sh              # build/omni.fwsc, the installable firmware
```

`build/host/omni_host SCRIPT OUTDIR` runs the whole app on a computer from a script
(`tests/scenarios/*.omni`), writing audio and screenshots. `python3 tools/make_screens.py` remakes the pictures of the pages
above (`docs/img/page-*.png`) from `tests/scenarios/pages.omni`.

Installing firmware is at your own risk. Before installing a development build, make sure you can
put M-VAVE's firmware back (see BUILDING.md).

## Credits

Niagara Drift is the next link in a chain of FM-1 firmwares:

- [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments): the
  platform (hardware layer, USB, update loader, storage, installer), and a reference for the
  sound engine.
- [X0X](https://github.com/charlesvestal/fm1-x0x) and [FoMni](https://github.com/charlesvestal/fm1-fomni)
  by Charles Vestal: the platform reshaped for a single instrument, the host simulator and the
  browser emulator. Niagara Drift started from FoMni's code.
- [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1) by Quixotic7: the design of the delay and the
  reverb, written anew here.

GPL-3.0-only; see [LICENSING.md](LICENSING.md).

Niagara Drift isn't affiliated with or endorsed by M-VAVE, Hügelton Instruments or the makers of any
instrument that inspired it.
