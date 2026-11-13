# Changelog

## 0.3 (2026-11)

A new look on every page, sixteen new presets, eight gestures and eleven Tunings, with a manual that is now the
site's own pages. An update from 0.2 or 0.2.1 keeps your sound and scenes. Tested on an older FM-1; it saves on the
newer FM-1s' memory chip too, as 0.2.1 does.

- **A new look:** every page redrawn, dark and glowing, with a picture of the sound at the top and what you're turning
  under it. Whatever moves shows its value, and an LFO moving a setting moves its picture. Keys and buttons are named
  in boxes by their note. The panel's lights follow the screen: dark always means off, and when stopped only PLAY
  moves (it breathes).
- **Sixteen presets** in two banks, each its own mood and each moving over a minute: Niagara, Embers, Vespers,
  Dreamhouse, Abyss, Seance, Sonar, Sibyl, Howl, Cistern, Piston, Ashram, Telegraph, Tape, Falling and Bare. After an
  update, 0.2's presets that 0.3 doesn't keep (Eerie, Glass, Tanpura, Whistle, Cathedral, Metal) are in bank 6.
- **Eight gestures:** Weight, Shift, Clean and Wash on the top keys; Tick, Chord, Snarl and Stutter on keys 9–12, as
  deep as you press. Tap one in to stay; a scene saved with a gesture latched brings it back.
- **Eleven Tunings:** 12-tone, quarter tones, 19-, 31- and 53-tone, just, the harmonics, La Monte Young's, Lou
  Harrison's, the shrutis and Partch's 43 notes. The pitch knobs land on the Tuning's notes, the screen names the
  interval between the drones in the Tuning's own terms, and Partner, Shift, Wander and Chord follow it. A sound
  carries its Tuning. Tune A from 427 to 453 Hz.
- **Banks by number:** six of your own and two of presets, picked with SEL and a gesture key: 48 scenes of yours. A
  scene reads as 3.5 (bank 3, key 5).
- **New sounds:** a Shimmer reverb (one sparkling octave that washes in and out); Grain and Self delays whose echoes
  move their own time; a drive after the delay; cross-FM and Snarl, the drones bending each other's pitch; a Feedback
  that sings; each drone's own sub wave; pitch down to 2 Hz and the sub to 4 octaves down.
- **The LFO:** the stepped AND and OR waves; its slowest rate a cycle in 100 s; X-Wave and resonance turn
  at their ends, so every part of a sweep is heard.
- **MIDI clock:** the Swell, the LFO, the delay, Wander and Scene time follow it; Sync on Setup 1 lets the drone run
  free of it.
- **Pages:** HOME opens Drones (the levels and cutoffs), FX opens Space (the effects' amounts). Feedback moved to
  KNOB 1 on Life, Cross-mod to KNOB 4 on Mods, Tune to KNOB 2 on Setup 2 (a tip says so, once).
- **The motion loop** records gestures and scenes too.
- **Screen reader:** the screen as words, for Charles Vestal's FM-1 Screen Reader.
- **The installer** saves your sound and scenes before it installs, and its "Your sounds and scenes" card puts them
  back; a restore that is cut short finishes at the next start.
- **The manual** is now the site's own pages: Play it, Recipes, Reference, Shortcuts and Help, and feedback without an
  account.

## 0.2.1 (2026-11)

Getting on and off Niagara Drift, on every FM-1. The sound is the same as 0.2's, and an update from 0.2 keeps your
sound and scenes. Tested on an older FM-1; the newer FM-1s' memory chip hasn't been tried here yet.

- **Newer FM-1s:** 0.2 didn't recognize the memory chip of some newer FM-1s, so on those it couldn't save and ignored
  every install (it just started again). 0.2.1 works with any standard memory chip, and never ignores an install. An
  FM-1 like that still on 0.2 needs [M-Flasher](https://github.com/jvitkauskas/m-flasher) once to leave it: the
  install page's "Trouble installing?" shows how.
- **Back to M-VAVE's V16 or V15,** whichever bootloader the FM-1 has (0.2 took back only the version whose bootloader
  it already had, so an FM-1 with V15's refused V16). Charles Vestal's fix, from felucca-bsp.
- **The installer takes M-VAVE's V16** as well as V15 (M-VAVE's link now gives V16; V15 is linked on the Internet
  Archive). From 0.2, "Back to M-VAVE's firmware" updates Niagara Drift to 0.2.1 first, then writes M-VAVE's: one
  button.
- **"Trouble installing?"** on the install page, and in the manual: what to do when an install doesn't take.

## 0.2 (2026-10)

A calmer, deeper instrument: slow movement by default, gestures and presets to play, scenes in banks you can
crossfade with pressure. Tested on one FM-1. An update from 0.1 keeps your sound and scenes (scenes 1–12 go to
bank A, 13–16 to B1–B4).

- **A new starting sound** (Preset P1, Niagara): slow and breathing, the filters swept by the LFO, the Swell on,
  delay and reverb in.
- **Twelve Presets** (SEL + G5), fixed: Niagara, Abyss, Eerie, Glass, Heaven, Tanpura, Tape, Whistle, Howl,
  Cathedral, Metal, Clean; each its own sound, movement and effects, levelled to each other. With Pitch lock on they
  keep your pitch. A new FM-1 starts on them.
- **Gestures on D5–G5:** Weight (a full, warm bass), Shift (both drones move, up to a 5th either way), Clean (the
  dirt out, as loud), Wash (into the effects). Hold: in while held, out as slowly as you held it; tap: all the way,
  tap again to let go; SEL as you let go keeps it where it is. PRESETS sets **Gesture time** (x0.1–x10).
- **Scenes in banks:** 12 a bank on F3–C5; banks A, B, C and the Presets (SEL + D5..G5). A tap goes there over the
  Scene time.
- **Crossfade by pressure:** hold a scene key and press harder to move further into its scene; let go and the sound
  glides back. Press all the way to land in the scene (the key flashes); SEL while holding keeps the blend between
  the two. Pressure sensitivity is on Setup 2. The crossfade was Quixotic7's idea; the key pressure is read with
  MONK-1's method (Charles Vestal), from Velo's sensing.
- **Pitch lock** (black key 4), on at start: the pitch knobs and scenes leave the pitch alone.
- **The left encoders, always live:** SELECT the LFO rate, ALGORITHM Movement, PRESETS Gesture time; OCT− / OCT+
  pick LFO Wave I / II. Two pages on most page buttons (a button opens the one you last used).
- **New sounds and moves:** Age (tape: darker, hiss, wow and flutter), Wander (drone II's slow random melody on a
  scale), Pitch glide, Shift's move, the 17 partner intervals (each tempered, then just), a noise color blend.
- **New effects,** after ChoralRoot's (Quixotic7), written anew: a stereo delay (Tape, Wide, Ping-pong, Clean) and
  an eight-line reverb (Room, Hall, Cathedral, Chorus hall; length to Inf, a freeze). Feedback rings in key and holds
  its loudness.
- **The speaker and headphones:** the FM-1 hears which is in use; the speaker gets a low cut, headphones the full
  range. MASTER's top fifth drives the output harder at the same loudness.
- **Level** is an audio taper (saved sounds keep their loudness; the numbers read higher). Band-pass and high-pass
  keep their loudness at high resonance.
- **The screen** in the site's colors: what's turning shown large; SEL held draws the black keys as on the
  keyboard, and touching one names it. Lights at several brightnesses: the OCT lights pulse with the LFO waves, ARP
  breathes with the Swell.
- **Knobs** catch every click on slow, paused turns.
- **Saving and backups:** autosave 4 s after the last change, at most every 30 s. `fm1_install.py --backup` saves
  your sound, scenes and settings to a file and `--restore FILE` puts them back; installing over Niagara Drift from
  the terminal saves a backup first, in `~/niagara-drift-backups/` (the web installer doesn't yet).
- **The website:** the emulator's FM-1 labeled with what every control does, the manual in the order you meet it
  (presets, gestures, scenes first), a link preview and an icon.

Thanks to [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1) (Quixotic7) for the effects' design, the crossfade
idea and the idea of a backup before installing, and to [MONK-1](https://github.com/charlesvestal/fm1-monk) (Charles Vestal) for
reading key pressure.

## 0.1 beta (2026-10)

The first public version, tested on one FM-1.

- Two drones, each with X-Wave (sine → triangle → saw → square), pulse width, a partner oscillator (interval,
  detune, equal or just tuning), a sub (−1, −2 octaves, ⅓, ⅕), white or pink noise, a 12 dB low-, band- or
  high-pass filter and drive.
- Links: pitch link, hard sync, cross-mod (drone I moves drone II's cutoff).
- The Ultra LFO: one rate (a cycle in 50 s to 70 Hz), two waves of eight shapes, routed to pitch / X-Wave
  and cutoff / resonance; rate mod; lock to the Swell. Movement (OCT− / OCT+) scales every depth 0–200 %.
- The Swell: both drones breathing in time, with swing.
- Effects: a PT2399-style delay into a reverb (Room, Plate, Hall, Chorus hall); Feedback; Character; a Speaker
  low cut.
- Drift, Morph, 16 scenes on the white keys (the current one's key blinks), a 5-minute motion loop.
- SAVE writes at once, even while playing (the sound dips for ~50 ms); autosave when stopped.
- A browser emulator running the same code, and a web installer that can also put M-VAVE's firmware back.

Built on FoMni and X0X (Charles Vestal) and Felucca's FM-1 platform (Leo Kuroshita, Hügelton Instruments).
FoMni's own versions are kept as the tags `fomni-v0.1` … `fomni-v0.3.2`.
