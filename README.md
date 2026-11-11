# Niagara Drift

**A drone synth for the M-VAVE FM-1.**

> **This is 0.3, in development** (what's on `main` now); its manual is in [docs/manual](docs/manual/index.md). For
> the released 0.2.1, see
> [the README of 0.2.1](https://github.com/evanyerburgh/niagara-drift/blob/v0.2.1/README.md); for 0.1 beta,
> [the README of 0.1](https://github.com/evanyerburgh/niagara-drift/blob/v0.1-beta/README.md).

Niagara Drift is open-source firmware that turns the M-VAVE
FM-1 into a drone synthesizer: two always-on drones you tune, shape and move rather than play note by
note. It's inspired by the Stylophone CPM DS-2, the Moog Subharmonicon and the Soma Lyra-8.

![The FM-1 with what every control does in Niagara Drift written on it](docs/img/panel.png)

*Your FM-1 with Niagara Drift installed: what each knob, button and key now does. Amber is drone I,
teal drone II, lilac the LFO. The black keys are switches (lit = on; on the LFO pages they set the
LFO's routing, and cross-mod lets drone I move drone II's filter, or the drones and their sound bend each other's pitch); white keys 1–8 hold scenes, in
eight banks you pick with SEL and the gesture keys; keys 9–12 and the top four are gestures. The page buttons' pages, each tap the next: HOME
Drones, Pitch · ENV Life, Tone · EDIT Osc, Sub · Drive · SEQ Partner, Pitch moves · LFO LFO sends, Mods · ARP Swell ·
FX Space, Details · GLO Setup 1, 2.*

**Status: 0.2.** It runs on the FM-1 (tested on one unit so far) and in the browser:
[try it](https://evanyerburgh.github.io/niagara-drift/) without an FM-1, the same code with sound.

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
- **Links** between them: pitch link, hard sync, and cross-mod: drone I's sound moving drone II's cutoff, or each
  drone's sound bending the other's pitch (cross-FM, as on the Lyra-8).
- **The Ultra LFO**, after the DS-2's: one rate, two waves of ten shapes each (one of them truly
  random, two the Lyra-8's stepped AND and OR), routed to each drone's pitch or X-WAVE and cutoff or resonance, alone or summed. It can
  move its own rate and lock to the Swell.
- **Movement** (ALGORITHM): all the LFO's depths down or up together, from still to double.
- **The Swell**: both drones breathing in time, from a slow soft swell to a hard rhythmic gate, with swing.
- **Effects**: a stereo delay of six types (Clean, Wide, Ping-pong, Tape, and the Lyra-8-like Grain and Self, whose
  own echoes move their time; the LFO can warble it too), a drive
  after it (the drones and their echoes distorted together, as on the Lyra-8), into a reverb of five (Room, Hall,
  Cathedral, Chorus hall, Shimmer) whose length goes to Inf, a freeze.
- **Feedback** (the finished sound fed back into the drones) and **Character** (clean to vintage).
- **Drift**: the drones are never quite still; pitch, cutoff, timbre, level and detune slowly wander.
- **Morph**: buttons and keys never jump: the sound moves to a new setting over a time you set.

## Installing

1. Open the [web installer](https://evanyerburgh.github.io/niagara-drift/install/) in **Chrome or Edge**
   (they can talk to the FM-1 over USB; Safari and Firefox can't).
2. Connect the FM-1 straight to the computer with a USB **data** cable (no hub) and switch it on.
3. Press **Install** and allow MIDI when the browser asks. It takes about two minutes; don't touch the cable
   or the FM-1 until it says Done. Then press PLAY.

Or from a terminal, with the file from the [releases](https://github.com/evanyerburgh/niagara-drift/releases):
`pip3 install mido python-rtmidi`, then `python3 tools/fm1_install.py niagara-drift-0.2.fwsc`.

**What it replaces:** Niagara Drift takes the place of M-VAVE's firmware, so the FM-1's own sounds and
sequencer are gone until you go back (below).

**Updating:** when there's a new version, run the installer again the same way. Your sound and scenes are
kept, unless that version's notes in the [changelog](CHANGELOG.md) say otherwise.

**Backing up, going back to M-VAVE's firmware, and when an install doesn't take:** see
[Help](docs/manual/help.md). Installing firmware is at your own risk.

## The manual

- [Start here](docs/manual/start.md): two ways in, from a Preset or from a bare saw
- [Recipes](docs/manual/recipes/index.md): how to get a sound or a move, across the pages
- [Reference](docs/manual/reference/index.md): every page, knob and key
- [Shortcuts](docs/manual/reference/shortcuts.md): every key combination, hold and tap
- [Help](docs/manual/help.md): installing, backing up, going back, problems

On the [site](https://evanyerburgh.github.io/niagara-drift/) the manual's pages come with the emulator, and each recipe
plays its sound (from 0.3).

## Problems and ideas

It's tested on one FM-1 so far. If something goes wrong, sounds wrong or could be better, please
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
above (`docs/img/page-*.png`) from `tests/scenarios/pages.omni`. The manual's pages are Markdown in `docs/manual`;
`python3 tools/make_manual.py OUT` makes them into the site's pages (the emulator's build does), and
`python3 tools/make_manual.py --renders` remakes each recipe's screens and sound from its script in
`docs/manual/renders`.

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
- [MONK-1](https://github.com/charlesvestal/fm1-monk) by Charles Vestal: reading key pressure (from
  [Velo](https://github.com/charlesvestal/fm1-velocity)'s sensing), which plays the scene crossfade.
- [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1) by Quixotic7: the design of the delay and the
  reverb, written anew here; the idea of crossfading between scenes, and of a backup before installing.
- [fm1-base](https://github.com/charlesvestal/fm1-base) by Charles Vestal: the safe restore's flash core (staging,
  the journal, the commit and its finish at power-on), as it is there (MPL-2.0).
- The [FM-1 Screen Reader](https://github.com/charlesvestal/fm1-reader) by Charles Vestal: the screen's words over
  USB and the page that speaks them (its firmware side from fm1-base, MPL-2.0); thanks to João Durães (WoodJack) for
  asking for it.

GPL-3.0-only (the files from fm1-base keep their MPL-2.0); see [LICENSING.md](LICENSING.md).

Niagara Drift isn't affiliated with or endorsed by M-VAVE, Hügelton Instruments or the makers of any
instrument that inspired it.
