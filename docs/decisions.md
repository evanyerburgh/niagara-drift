# Niagara Drift: decisions

The final decisions, dated, oldest first; a later one replaces an earlier one on the same thing. Add new ones
at the end. The current state is in the [README](../README.md) and the code.

## Project

- 2026-10-07: a fresh GitHub repo that keeps FoMni's git history (not a GitHub fork).
- 2026-10-07: PRs against `main`, never stacked on another PR's branch.
- 2026-10-11: the name is **Niagara Drift**; tagline "a drone synth for the M-VAVE FM-1". The folder
  (`~/fm1-drone`), the build files (`omni.*`) and the emulator's storage key keep their old names.
- 2026-10-11: Felucca is GPL-3.0-only (checked); code published under it stays usable.
- 2026-10-14: CLAUDE.md is kept out of the repo, because it's working notes, not for players.
- 2026-10-14: released as 0.1 beta (`./build.sh --release 0.1-beta`: identity FM-1_8000100). Over USB the
  FM-1 is "Niagara Drift FM-1"; the installers find it by "FM-1". FoMni's tags are renamed `fomni-v0.x`.
- 2026-10-14: the site is the emulator page at the root, with Install, Download, Full manual and Source
  (`tools/make_pages.py`). The web installer (`web/installer.html`) is English only: install, resume a cut-off
  install, put M-VAVE's V15 back (SHA-256 checked).

## The instrument

- 2026-10-09: **two drones**, DS-2-like (four voices was tried). KNOB 1-2 drone I, KNOB 3-4 drone II on every
  page, because four voices needed too much menu diving. Page buttons: tap to stay, hold to peek.
- 2026-10-07: pitch is set by knobs, continuous, because drones are tuned by ear between notes: 60 cents a detent (20 a turn = an octave), SEL held 1 cent.
  Each detent glides over the time since the last (up to 600 ms). Range 27 Hz .. 1.6 kHz.
- 2026-10-14: the pitch knobs speed up to 2x when spun (4x went too far); the other knobs to 4x.
- 2026-10-10: the starting sound: drone I D3, drone II A3, cutoff 70 / 68, resonance 35, partner detune 12, because an
  octave lower most of it sat under 200 Hz, which small speakers barely play.
- 2026-10-07: X-WAVE blends sine, triangle, saw, square (phase-aligned, snapping to a pure wave within 2/300).
  OSC MIX crossfades the oscillators and the sub.
- 2026-10-10: the sub: -1 oct, -2 oct, 1/3, 1/5; under 20 Hz it goes up an octave, since it can't be heard there. Just tuning for the
  partner's intervals (Setup 2).
- 2026-10-07: the filter is a 12 dB trapezoidal SVF (LP, BP, HP) that can self-oscillate. 2026-10-10: in
  low-pass the passband stays level at high resonance, so resonance never thins the bass. Cutoff and resonance run 0..1000 (a detent 10, SEL 1),
  shown as 0..100 with a tenth.
- 2026-10-09: links: hard sync (band-limited, polyBLEP), pitch link (KNOB 3 moves the offset), cross-mod
  (drone I's sound moves drone II's cutoff at audio rate) on the lone black key.
- 2026-10-11: NOISE per drone, a level beside the oscillators on Tone 2: 100 is 6 dB under them, 50 -12,
  25 -18, so it's heard all along the knob and never drowns the oscillators. Colour white or pink (-3 dB/oct), crossfading over the Morph; about equally loud.
- 2026-10-11: DRIFT: pitch drift plus four slow wanders per drone (cutoff +-0.5 oct, X-WAVE +-0.12, level
  +-2.4 dB, detune +-10 cents at 100), a new aim every 10..40 s glided over ~8 s.
- 2026-10-11: CHARACTER: softening from 8 kHz to 1.2 kHz in even steps, even harmonics, about +-6 cents of
  jitter, drone II up to 6 cents sharp, -44 dB noise; skipped at 0.
- 2026-10-14: FEEDBACK: the finished sound fed back into both drones' filters, 256 samples later, lows cut at
  80 Hz; loop gain 0.9 at 100, its addition capped at 0.45, so it adds texture without running away or getting
  louder.
- 2026-10-10: MASTER: a square law up to 4/5 of the turn (the clean top), then drive to full, so the top of the
  knob is an effect, not just louder.
- 2026-10-10: every continuous setting glides in the engine (a knob detent over the time since the last), so slow
  turns sweep instead of stepping.

## Movement

- 2026-10-07: the Ultra LFO: one rate, two taps (Wave I, Wave II) of Off + 7 shapes + random, routed per drone
  to pitch or X-WAVE and to cutoff or resonance, with bipolar depths. 2026-10-14: rate 0.02 Hz (a cycle in 50 s,
  the DS-2's slowest) .. 70 Hz; longer arcs come from Scene switch and the motion loop.
- 2026-10-11: rate mod is linear (+-100 = +-3 octaves) and keeps the tempo; Random draws a new level at every
  restart, seeded at the first PLAY.
- 2026-10-11: PRESETS is Wave I, ALGORITHM is Wave II, SELECT is the LFO rate.
- 2026-10-11: OCT- / OCT+ step the MOVEMENT (octave jumps were too abrupt for drones): every LFO depth scaled 0..200 %, 10 % a tap, lit while off 100 %.
- 2026-10-11: the default sound breathes (filter LFO depth +25 on drone I, -25 on drone II), so the Movement is
  heard at once.
- 2026-10-10: the SWELL (code: Pulse) on ARP: tap on / off, hold to peek, two taps within 350 ms open its page.
  From 0.03 Hz (a 33 s breath); Shape firms a sine into a gate; Depth in dB (0 .. -30, 100 silence). It fades in
  and out over the Morph time like a breath (exponential), and ARP flashes with the beat.

## Switches, scenes and the loop

- 2026-10-11: the black keys: drone I (partner, sub, filter) | pitch lock, LFO reset | drone II | pitch link,
  hard sync | cross-mod. The LFO pages turn them into routing.
- 2026-10-10: MORPH: a switch never jumps (a jump clicks and breaks a drone); it moves the sound over the Morph
  time (0.02 .. 30 s). Stopped,
  everything snaps.
- 2026-10-09: 16 scenes on the white keys. Tap: go there over the Scene switch time (0.1 s .. 5 min) and stay.
  2026-10-11: hold: visit, and let go to come back. SAVE held + a key stores. The white keys are only scenes.
- 2026-10-11: scenes keep the sound, not the room: Fade, Tune, Speaker, Stereo, Pitch lock, Morph and Scene
  switch stay as they are, because they belong to where and how you play, not to the sound.
- 2026-10-14: the scene stored or recalled last blinks slowly until the sound is changed by hand, so you can see
  which one you're on.
- 2026-10-09: the MOTION LOOP on REC: knob moves recorded with their timing, up to 5 minutes (4096 events);
  each pass starts from where the recording began.
- 2026-10-11: SEL + PLAY fades drone II alone. Pitch lock guards only the pitch knobs.
- 2026-10-14: SAVE and storing a scene while playing write at once, the output dipped ~50 ms around it to hide the flash write's
  pause (under 6 ms). The
  autosave waits until PLAY stops.

## Effects

- 2026-10-09: a dark PT2399-style delay (70 .. 450 ms, repeats up to runaway) into a reverb of four types
  (Room, Plate, Hall, Chorus hall; Dattorro's plate structure, our own code), one mix each for both drones, the
  LFO on the delay's time. Mono-safe, because the FM-1 has one speaker. Defaults: reverb 25 % Hall, delay off.
- 2026-10-10: the SPEAKER low cut (40 .. 240 Hz), off by default, since headphones and line out don't need it.

## The screen

- 2026-10-09: concept D: dark and glowing; moving pictures on top, what's selected or moving below; live
  waveforms. 2026-10-10: no interval readout between the drones (tuned by ear).
- 2026-10-11: 13 pages, all full: Play 1 / 2, Tone 1 / 2, Osc, LFO depths / mods, Partner, Setup 1 / 2,
  Effects 1 / 2, Swell. The names match on screen, manual and README; "Scene switch" (was "Scene time").
- 2026-10-14: the text a quarter bigger (15 / 19 / 19 / 28 px), to read it on the device; a value too wide steps down a face.
- 2026-10-14: no FoMni splash; our startup animation fades in from black.

## Engineering

- 2026-10-07: the engine is `dsp/drone.{c,h}`; `drone_fx.c` is included by it.
- 2026-10-09: scenes live in the project (16 x every parameter); new parameters go at the end and bump
  PROJ_FORMAT (saved projects reset once).
- 2026-10-14: CPU on the FM-1: about 39 % with the default sound, 3.5 % stopped; the host estimate reads low
  (it misses divides), so measure on the device (`tools/fm1_cpu.py`).
- 2026-10-17: key pressure on the 20 lower keys (F3–C5; the top seven have no converter), ported from MONK-1. Absolute scale: 0 % at a light touch
  (reading 900), 100 % at a firm press (540), curve ^1.6, hysteresis 32. Shown on screen while held; nothing
  plays from it yet.

## v0.2

- 2026-10-17: OCT− / OCT+ play the LFO's Wave I / Wave II: a tap is the next shape, wrapping; SEL + OCT the one
  before; each lit while its wave is up. ALGORITHM is the Movement; PRESETS is kept for a macro.
- 2026-10-17: EDIT has Osc (X-Wave, PW) and Source (osc mix, drive); ENV has one page, Tone (noise, colour);
  Setup 2 is morph, scene switch, tune, tuning.
- 2026-10-17: the noise Colour is a blend, 0 white .. 100 pink (PROJ_FORMAT 20; a 0.1 project loads, its
  Colours turned into 0 or 100).
- 2026-10-17: lights have brightness levels (software PWM over 32 scan frames, the share lit the level squared);
  the OCT lights pulse with the LFO waves and the ARP light breathes with the Swell. The pressure test readout
  is gone (it covered the black keys' banners).
