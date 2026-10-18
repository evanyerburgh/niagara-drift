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
- 2026-10-17: turning an LFO depth that can't be heard shows why: "MOVEMENT 0% ALGORITHM", or "WAVE I OFF tap OCT-"
  (II and OCT+ likewise). The top bar's right-hand text (PLAY to start, FADING) shortens or hides where the
  badges on the left (II OUT) would overlap it.
- 2026-10-17: scenes are in banks: 12 a bank on the lower white keys (F3–C5, the keys that sense pressure);
  banks A, B, C are yours (A in the project object, B and C in their own at 0x97000 and 0x99000, above the
  app area the loader writes), and a read-only Presets bank is in the firmware (so far the starting sound and
  the bass test). SEL + D5 / E5 / F5 / G5 picks A / B / C / Presets; the top four keys alone are kept for the
  gesture macros. A 0.1 project's scenes 1–12 become A1–A12 and 13–16 B1–B4 (PROJ_FORMAT 21; the banks are
  written before the project, so a power cut can't lose them).
- 2026-10-17: scene switches and visits follow Pitch lock: lock on, the drones keep their pitch (and the link
  offset).
- 2026-10-17: backup and restore over USB (SysEx 0x70–0x73: read, write, commit an object; restart):
  `fm1_install.py --backup [FILE]`, `--restore FILE`, and a backup in `~/niagara-drift-backups/` before every
  install over Niagara Drift (`--no-backup`: not). The web installer's comes later.
- 2026-10-17: the pressure crossfade: a scene key held is a fader from the sound before into its scene; under 8 %
  nothing moves, 90 % is all the way, and held there 0.3 s the scene is taken (the key flashes). A tap (under
  0.25 s, however hard) goes there over the Scene switch time; letting go glides back over the Morph time
  (pressed again on the way, from there); SEL held at release keeps the blend. The key's brightness shows how
  far in. Where a key has no pressure (the emulator), a hold is 0.1's timed visit.
- 2026-10-17: LFO mods: a negative delay wobble reads "-+25%"; X-mod draws the range it pushes drone II's
  filter through.
- 2026-10-17: a value a hand moves while a scene switch (or a crossfade) is on its way is let go by the switch:
  the left encoders (LFO rate, Movement, the macro) and the OCT wave taps; the rest still lands. A page knob
  still stops the switch where it is. A new switch moves everything again.
- 2026-10-17: the drones' pictures start on a cycle, or after 0.1 s without one (hard sync could freeze
  drone II's).
- 2026-10-17: the macros: eight fixed moves, each a few settings by set amounts, added on top of the sound
  (the sound's own values untouched; what's heard is clamped to each range). Set macros Bloom, Weight, Tide,
  Tear on SEQ's Macros page (SEQ again: Partner); PRESETS plays one, Bloom to start, SEL + PRESETS picks
  (the player's choice, not a scene's). Gestures Beat, Heat, Converge, Wash on D5–G5: held they swell in, let
  go out; a tap latches, a press releases, SEL at release latches where it is; held, PRESETS sets the depth
  and ALGORITHM the swell (1–30 s). Tear switches hard sync on and lifts drone II up to an octave; Converge
  turns cross-mod on and leans drone II toward the nearest unison or octave of drone I. Amounts, depths,
  swells and latches are values scenes keep; with Pitch lock on a scene leaves Converge's latch alone.
  PROJ_FORMAT 22 (19–21 converted: their 78 values a sound widened, the new ones at their defaults).
- 2026-10-17: a new starting sound (also Preset P1), slow and breathing like the calm Lyra-8: filter LFO depths
  ±85 at LFO rate 8, Wave II a random glide, the Swell on (rate 6, depth 40), resonance 250, delay mix 25 with
  repeats 50 and a +20 wobble, reverb mix 35. Measured: a 16 dB loudness swing, 1.6 octaves of brightness, 95–99 %
  of the movement slower than 0.5 Hz (0.1's start: 3.7 dB, 0.7 octaves, ~42 %).
- 2026-10-17: a restart brings back every saved value (the engine's queue held 64 and dropped the rest of a
  load, so effects, Character, Feedback and the macros came back at their defaults); it holds 128 now.
- 2026-10-17: a save writes the project only when it differs from what's on the flash (the banks already were).
- 2026-10-18: the effects are ChoralRoot's design, written anew in float. The delay is stereo, with four types
  (each a mode, spread, tone and wow): Tape (mono, warm, gentle wow), Wide (the right at 3/4 of the left's time),
  Ping-pong, Clean (bright, no wow). The reverb is an eight-line feedback delay network at 22.05 kHz with four
  types (size, pre-delay, swing, low cut, width): Room, Hall, Cathedral, Chorus hall. FX: Delay (type, time,
  repeats, mix) / Reverb (type, length 0.3–40 s and Inf, damp, mix); a type change glides. Inf is a freeze
  (damping out of the loop); stopped, it lets go over about twice the Fade time. Wash adds +45 Length (Inf at
  full depth). Defaults: Tape, Length 60 (5.9 s), Damp 40.
- 2026-10-18: ENV has two pages: Tone 1 (noise, colour) and Tone 2 (character, feedback, speaker, pressure
  sensitivity x0.5–x2). Speaker and Pressure are room settings, not in scenes. PROJ_FORMAT 23 (22 converted).
- 2026-10-18: worst-case sound on the FM-1 (everything on, full feedback and character, all macros): 57 %
  average, 70 % peak block, no late blocks (0.1's effects). The new effects cost the same on the host.
- 2026-10-18: SEL held 0.4 s shows the black keys' map over the whole screen: the five groups as rows, each key lit
  as on the panel with its name and setting (the LFO pages' layer there); a turning knob or a held white key
  takes the screen back.
- 2026-10-18: during a crossfade, SEL down holds the blend where it is; letting go of the key then keeps it (a
  finger's pressure falls on the way up, which had carried the blend back first).
- 2026-10-18: the drones' pictures blend each new capture in over ~0.1 s.
- 2026-10-18: Play 2 is level, then resonance (KNOB 1 / 3 level, 2 / 4 resonance); its picture shows the level above
  the filter curve. Speaker and Pressure move from Tone 2 to a new GLO page, Setup 3; Tone 2 keeps character and
  feedback (KNOB 3-4 nothing).
- 2026-10-18: the screen takes the site's palette: plum black (20, 0, 15), aubergine panels, the site's card for a lit
  value, pale pink ink; amber drone I, teal drone II, lilac the LFO, everything else in the ink (no blue, pink or
  gold). All text 4.5:1 or more on its background. No dots: page dots become "1/3", the unsaved dot becomes a dim
  SAVE light, the LFO rate lights go (OCT- / OCT+ pulse), Setup 1's stereo dots become two bars. Barlow stays.
- 2026-10-18: the gestures (D5-G5) are Weight, Shift, Converge, Wash (Beat and Heat gone). Each macro's moves come in
  over their own part of its amount and leave over another (eased, gliding up to 0.6 s), so parts arrive in order.
  Shift: drone I then drone II to a 5th up (default), 4th up, 4th down or 5th down (PRESETS held). Converge pulls to
  the nearest unison, 4th, 5th or octave (at most 2.5 semitones), detune and Drift to 0, cross-mod last. Weight adds
  drive +30 and resonance -80. Wash adds Damp +15. Swells to start: Weight 45 (4.6 s), Shift 70 (11 s), Converge 60
  (7.7 s), Wash 55 (6.5 s). PROJ_FORMAT 24 (23's gesture values reset). The Macros page is unchanged for now.
- 2026-10-18 (Evan's listening): Weight focuses: Width, the partners' detune to 0 and Movement -25 over its first 60 %,
  cutoffs -100, the sub over 20-100 %. Converge: drone I dips (level -25, cutoff -60) while drone II moves (a bump, in
  over 0-20 %, out over 45-75 %, both ways). Wash: its release is 2.5x its swell; at the top (85-100 %) the dry levels
  breathe -14 at 0.15 Hz.
- 2026-10-18 (Evan's listening, round 3): Weight is a full, warm bass, not focused: osc mix +45 (0-70 %), cutoffs -30,
  resonance -40, drive +35, Character +25, noise -40, drone I's level +8; the width, detune and Movement moves are gone.
  Converge never quite converges: drone II stays 0.09 semitone short of the consonance, wavering by 0.05 at 0.07 Hz
  (about 1.3 beats a second at A3); the partners' detune and Drift ease by 60 %, not to 0.
- 2026-10-18: the gestures play by hold (in while held, out when let go) and tap (to its depth, stays; tap again: out);
  depth on PRESETS while held. Swells shorter: Weight 30 (2.8 s), Shift 40 (3.9 s), Converge 35 (3.3 s), Wash 35
  (3.3 s); letting go takes as long as the swell (Wash's 2.5x release gone; its tails are the reverb's own). PROJ_FORMAT
  25 (24's swells reset to these). Clean, when built, keeps the notes (no unison or sync).
- 2026-10-18: the player times the gestures (Evan: "not just on-off effects"). PRESETS is Macro time: every
  gesture's time x0.1 .. x10 (50: as set), live, kept in the project (not scenes; PROJ_FORMAT 26, in the byte that
  held the macro PRESETS played, which is gone). Let go after a hold: out over as long as it was held, x Macro time.
  Holding a gesture key: ALGORITHM its depth (Shift: its move), PRESETS its own time. A move's level glides over
  0.15 s at most (was 0.6).
- 2026-10-18: the Macros page goes, with the set macros Bloom, Weight, Tide and Tear (Evan). SEQ opens Partner. Their
  four values stay in the layout, unused (0); PROJ_FORMAT 27 zeroes them. Any new page of macros comes later, if
  Clean, Drifting pitch or Glitch earn one.
