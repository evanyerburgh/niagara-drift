# Niagara Drift: decisions

The dated log of decisions, Evan's (design) and Claude's (marked "engineering"), oldest first. Add new
ones at the end. The current state is in the [README](../README.md) and the code; this explains how it got there. The
repo and folder were called fm1-drone until 2026-10-11.

- 2026-10-07: name stays `fm1-drone` for now.
- 2026-10-07: a fresh GitHub repo (not a GitHub fork) that keeps FoMni's git history; FoMni is the
  `upstream` remote.
- 2026-10-07 (engineering): the README now describes fm1-drone; BUILDING.md's install, rescue and
  publishing steps still point at FoMni's URLs and are rewritten when we first publish.
- 2026-10-07 (engineering): the install test runs Node with `--no-warnings`; any Node warning on
  stderr (here, from a `package.json` in a parent folder) was read as the package's product name.

- 2026-10-07: open PRs against `main`, never stacked on another PR's branch: PR #2 was opened on
  `setup`, merged there after #1 and never reached `main` (carried over by the drone-engine PR).
- 2026-10-07 (engineering): the drone engine is its own module, `dsp/drone.{c,h}` (`dr_*`), beside
  FoMni's `omni.c`. The app and UI keep running OMNI until step 5 swaps them, so the tree builds and
  tests the whole way. Voice count is a runtime parameter (P_VOICES: 2 = main oscillators, 4 = main +
  partner) so both can be heard in the same build.
- 2026-10-07 (engineering): 12 dB/oct trapezoidal SVF (LP/BP/HP) with a soft clip on its band-pass
  state for stable self-oscillation, and input that drops as resonance rises (like a ladder / OTA)
  so the resonance sings over the oscillators. A 24 dB/oct 3320-style low-pass is a later option.
- 2026-10-07 (engineering): cutoff is smoothed over ~60 ms. Its 100 knob steps are 1.2 semitones each,
  which a self-oscillating filter makes audible; finer steps (encoder acceleration) are a UI-step question.
- 2026-10-07 (engineering): CPU is measured as Felucca does: host instructions per sample from the macOS
  kernel counter (`tests/host/drone_cost.c`), converted with Felucca's device ratio (~1.7 % of the FM-1's
  audio budget per 100, from its PHYS engine measured on the device). Engine budget: 40 % (2,350); the
  test fails above it. Measured: 2 voices ~570 (~9.7 %), 4 voices ~685 (~11.6 %), FoMni at its busiest
  ~1,875 (~32 %). Felucca's chorus + delay + reverb bus: ~150-175 (~3 %). Unconfirmed until the device
  reports `plat_cpu_pct`; the counts don't see cycle costs such as the 4-6 FP divides a sample (soft clips,
  master tanh). If the device reads well above the estimate, make `sat()` division-free first.
- 2026-10-07 (engineering): step 5 replaces FoMni's UI and project with a first, temporary control map
  (`firmware/src/app/ui.c` header lists it) and removes FoMni's engine (omni.c, Chordian data, its test
  and scenarios). File names `omni.*` / `omni_host` stay for now; renaming is a separate change.
- 2026-10-07 (engineering): autosave runs only while the drone is stopped (a flash write stops the audio
  for a moment, and a drone is never silent while playing). SAVE always works.
- 2026-10-07: the emulator is played in the MacBook's browser (localhost), not on a phone: a private
  repo can't be served on GitHub Pages for free.
- 2026-10-07: before pushing to a PR's branch, check the PR is still open (`gh pr view N`). A fix pushed
  after PR #5 merged never reached `main` (re-done in its own PR).
- If the emulator is ever opened in the browser pane, stop the drone and close the pane: Evan hears it
  otherwise (2026-10-07: it was left droning).
- Testing the emulator with automated clicks: a click must be held ~100 ms or the device can miss it
  (the worklet reads the button state between 128-frame renders). Real clicks are long enough.
- 2026-10-07 (Evan, after comparing with the DS-2 panel): pitch is set by knobs, free between notes, not
  by keys. SELECT = group 1 pitch, ALGORITHM = group 2 pitch, continuous, never semitone steps
  (Evan). A detent is 60 cents (a slow full turn of 20 detents = an octave), up to 4x when spun quickly;
  hold SEL for 1 cent a detent. Each detent's change slides linearly over the time since the last detent
  (`dr_slide`, 15..250 ms), so turns at any speed are one continuous slide (tested: never under ~90 % of
  its speed). Plain turns rest in 60-cent hops from where they started; SEL gives any cent.
  2026-10-09: each glide lasts up to 600 ms (was 250): slow or uneven turns (an emulator drag) climbed in
  60-cent stairs. A first detent after a pause glides 300 ms.
  **Revisit the 60 cents a detent on the real FM-1** (Evan): its clicks per turn are unknown (the
  emulator draws 20); 20 cents a detent was the alternative. Range 27 Hz .. 1.6 kHz, the DS-2's oscillator range (2026-10-09; was 40 Hz .. 1 kHz). Waves: the classic set picked from a list (sine,
  triangle, saw, square + width). Keys will become DS-2-style latching switches (assignment: step 6).
- 2026-10-07 (engineering): pitch is a parameter in cents (GP_PITCH, replacing GP_FINE), so it saves and
  restores exactly; `dr_note` is gone. Glide defaults to 0 so a pitch knob answers at once.
  Project format 2 (an older saved project loads the defaults).
- 2026-10-07 (Evan): **four independent voices**, each with its own partner oscillator riding along, its own
  filter, pitch, wave and level (not two groups of a main + partner, which the first build was).
  KNOB n = voice n on every page; the Pitch page (HOME) holds the four pitches; the screen shows the four
  as columns over their knobs. The partner: an interval list + detune. SELECT = Brightness (all cutoffs),
  ALGORITHM = Crossfade (voices 1+2 <-> 3+4). Partner on/off, sub octave and filter mode are switches,
  so they wait for the key map; Key lights is gone.
- 2026-10-07 (engineering): 4 voices with partners cost ~17.5 % of the device (estimate). Level and
  Crossfade changes are smoothed (~6 ms) so they never click (tested). Project format 3.
- 2026-10-07 (Evan, after the DS-2 manual, `docs/ds2-notes.md`): the X-WAVE blends sine -> triangle ->
  saw -> square (the DS-2's square/triangle/saw plus a sine); OSC MIX crossfades main <-> sub; a DS-2-style
  Ultra LFO (one rate 0.02-70 Hz, two taps of Off + 7 shapes) routed per voice to its oscillator (pitch
  or X-WAVE) and its filter, with bipolar depths. No sync / pitch link between voices for now.
- 2026-10-07 (engineering): the four waves are phase-aligned so X-WAVE blends don't cancel (within 3 dB
  all along, tested); the X-WAVE snaps to a pure wave within 2/300 of it (33 = triangle, 67 = saw), and its
  label agrees. The LFO's 7+7 shapes follow the DS-2's names; their exact curves are ours. LFO routing
  and wave picks are switches: they wait for the key map (defaults: routed to a tap, depth 0). PRESETS is
  the LFO rate for now. ~18.7 % of the device with the defaults, ~22 % worst case. Project format 4.
- 2026-10-09 (Evan): back to **two drones** (DS-2-like): four voices sounded good but needed menu diving.
  The control map is in Figma: https://www.figma.com/design/CSTqDAhSGsHZbTHgYR9rfG/Untitled?node-id=3-2
  ("fm1-drone control map: draft 1"), built as drawn; its open questions come after. In short: KNOB 1-2
  drone I, 3-4 drone II on every page (Play: pitch, cutoff); SELECT / ALGORITHM / PRESETS = LFO rate /
  Wave I / Wave II; page buttons tap to stay, hold to peek (let go: back); the 11 black keys are lit
  switches in groups 3+2 (drone I osc / filter), 3+2 (drone II), 1 (both), following the page (LFO page:
  the routing). Principle (Evan): what's used most in performance is always live; the rest one tap away.
- 2026-10-09 (Evan): the lone key is drone I -> drone II cutoff (the DS-2 "Ultra Analog" patch); hard sync
  and pitch link sit on drone II's free keys. With pitch link on, KNOB 3 moves drone II's offset (it
  slides like a pitch).
- 2026-10-09 (engineering): cross-mod updates drone II's filter every sample (audio rate); hard sync is
  a plain phase reset (not band-limited yet); drone I renders first and leaves its sound, restarts and
  pitch for drone II. ~10 % of the device with the defaults, ~14 % with everything on. Project format 5.
- 2026-10-09 (Evan, the map's open questions): OCT-/+ move both drones an octave; ARP = the Pulse (tap
  on/off, hold for its page: Rate, Depth, Shape, Swing; own clock) and REC = the Motion loop (next PR);
  the white keys are 16 scenes (tap: recall, morphing over the Fade time; SAVE held + key: store); the
  LFO shapes stay on ALGORITHM / PRESETS (selectors, like the DS-2's); HOME again = a Mix page (the
  levels; KNOB 2/4 free for the effect sends); drone I's free filter key = LFO reset.
- 2026-10-09 (engineering): scenes live in the project (16 x every parameter, ~1.6 KB of the 3.8 KB
  object); storing one writes the flash at once. A morph glides smooth values and flips steps (ranges of
  10 or fewer) half way; a knob turned stops it; the Fade isn't recalled. While stopped the engine
  settles pitches at once (the screen showed old ones). Project format 6.
- 2026-10-09 (engineering): the PULSE is in the engine (a gain on both drones before the master clip):
  two beats a cycle for swing, a raised cosine blended with a gate by Shape, a 1 ms slew; its own clock,
  0.25-16 a second. ARP: a tap toggles it, a hold peeks at its page (the page never stays). The MOTION
  LOOP is in the UI: knob moves (KNOB 1-4, SELECT / ALGORITHM / PRESETS) recorded with their times and a
  pitch's slide, up to 1024 events / 30 s; each pass starts from the values the recording began with.
  Switches and scenes aren't recorded. Project format 7.
- 2026-10-09 (Evan): the EFFECTS as the DS-2's: a few fixed reverb types (Room, Plate, Hall, Chorus hall),
  the LFO on the delay's time (depth knob + a tap key: the LFO page's free key in drone II's filter group),
  one mix for both drones (no per-drone sends; Mix page KNOB 2/4 stay free), a dark PT2399-style delay
  (70-450 ms, soft-clipped repeats up to runaway). FX: Time, Repeats, Delay mix, Reverb mix; FX again:
  Reverb type, Delay LFO depth. Defaults: reverb 25 % (Hall), delay off.
- 2026-10-09 (engineering): `dsp/drone_fx.c`, included by drone.c. Delay: int16 line, 64 KB. Reverb: our
  own code of Dattorro's 1997 plate (paper structure and lengths, scaled to size per type), float lines,
  ~150 KB; its sides are blended 25 % so a mono speaker keeps it (mid within ~4 dB of the sides, tested).
  Both in .pool (300 of 344 KB used). Chain: drones -> pulse -> PLAY fade -> delay -> reverb -> MASTER;
  the tails ring on after PLAY stops; stopped and silent for 160 blocks (~0.93 s, longer than the delay's
  line) the effects rest. Cost ~7 % of the device (~18 % default total, ~25 % everything on). Format 8.
- The FM-1 renders stereo (the code's I2S frames, USB audio, Felucca's pans); its built-in speaker is
  one speaker (Felucca's SPEAKER EQ), so the effects stay mono-safe. Confirm the headphone jack on the unit.
- 2026-10-09 (engineering): concept D built in two PRs: first the layout, dark style, per-knob highlight
  timers (two hands), the top-bar label for SELECT/ALGORITHM/PRESETS, switch banners across the picture
  and live waveforms (`dr_scope`: ~2 cycles from a cycle's start, after filter and level; flat when
  silent); then each page's own picture. The picture redraws ~20 times a second (the LCD's SPI is slow).
- When work is done, leave ~/fm1-drone on `main`: `emulator.command` builds whatever is checked out.
- Not in the engine yet: hard sync isn't band-limited.

- 2026-10-10 (Evan): the starting sound an octave higher and brighter (drone I D3, drone II A3, cutoff
  70 / 68, resonance 35, partner detune 12): at D2 / A2 and cutoff 55, ~75 % of the sound was under 200 Hz,
  which laptop (and small) speakers barely play, so pitch and cutoff turns were hard to hear. The range
  is unchanged (27 Hz .. 1.6 kHz); only where it starts.

- 2026-10-10 (engineering): low-pass "no bass loss": the filter's input still drops as resonance rises (room
  for the peak and self-oscillation), and in low-pass (1 - rin) of the input through a one-pole an octave
  under the cutoff is added to the output, so the passband stays level (+0.0 dB at 55 Hz, resonance 90)
  while the peak and the self-oscillation are as before. Cost ~+1 % of the device. BP / HP unchanged.

- 2026-10-10 (engineering): the filter LFO's destination (GP_FLFO_DEST: cutoff or resonance) is a drone
  parameter, so GP_N is 19 and every later index moved (scenarios renumbered; PROJ_FORMAT 9: saved
  projects reset once). At depth 100 the LFO sweeps the whole resonance range (clamped 0..1).

- 2026-10-10 (Evan): no interval between the drones on the screen: people tune by ear.

- 2026-10-10 (Evan): MORPH: a switch (button or key) never jumps; it moves the sound to its new setting
  over the Morph time (Setup 2, KNOB 1: 0.02 .. 10 s, default 0.6 s), separate from Fade (PLAY, scenes).
  Knobs follow the hand. Filter mode crossfades; partner, cross-mod, pulse fade; LFO routes crossfade wave
  to wave and destination to destination; the sub's octave and hard sync dip out and back; OCT slides.
  Stopped, everything snaps to its setting. Setup 2's KNOB 2 is Tune (it had no knob). PROJ_FORMAT 12.

- 2026-10-10 (review, Evan: do the clear fixes): every continuous setting glides in the engine (pfs: each
  knob detent over the time since the last, as pitch; anything else over 25 ms), so slow turns sweep (the
  cutoff had 1.2-semitone stairs with 60 ms smoothing). Continuous knobs use pitch's speed curve (up to 4x).
  The motion loop glides back to its start over the Morph time (it jumped) and records up to 5 minutes
  (events in 5 ms units, 4096 of them). Fade reaches 60 s, Morph 30 s, and scenes have their own Scene
  time (0.1 s .. 5 min, Setup 2 KNOB 3). PROJ_FORMAT 13.

- 2026-10-10 (Evan, from the review): pages merged, 14 to 12, none half-empty: HOME Play / Play 2 (reso,
  level); ENV Tone (drive, width); EDIT Osc; LFO / LFO 2 (rate mod, mod by, X-mod amount, delay LFO
  depth); SEQ Partner; GLO Setup / Setup 2; FX Effects / Effects 2 (reverb type); ARP Pulse. Mix, Filter,
  Filter 2 and Shape are gone. The top bar's page name is bold, with a dot per page of its button; the
  pictures' text is 16 px (was 12).

- 2026-10-10 (Evan, from the review): just tuning (P_JUST, Setup 2 KNOB 4: the partner's intervals as
  pure ratios); subharmonics on the sub key (round: -1, -2 oct, 1/3, 1/5); the Pulse from 0.03 Hz (a 33 s
  breath); hard sync band-limited (a polyBLEP at the exact restart: as clean as a plain saw); MASTER's top
  fifth drives the output clip at about the same loudness (the clean top is the old full volume; the tests
  render at 3277, 4/5); a SPEAKER low cut (Effects 2 KNOB 2, 40..240 Hz, off by default: no headphone
  detection known yet); PITCH LOCK on drone I's third key. PROJ_FORMAT 14.

- 2026-10-10 (Evan): the Pulse is called the Swell on screen and in the docs (it now goes down to a 33 s
  breath). Code names (P_PULSE, pulse_run...) stay.

- 2026-10-11 (Evan): OCT- / OCT+ no longer jump octaves (too dramatic, little used in drone): they step the
  MOVEMENT (P_MOVE), every LFO depth scaled together 0..200 %, 10 % a tap, held on and on, gliding; lit
  while off 100 %. Pitch lock now guards only the pitch knobs. PROJ_FORMAT 15.

- 2026-10-11 (Evan): PRESETS (left) is Wave I, ALGORITHM (right) Wave II (was the other way). The Tone
  page shows the live waveforms (its own drawing and drive words didn't add anything).

- 2026-10-11 (Evan): the Swell fades in and out evenly to the ear: its dips deepen in dB in a straight line
  over the Morph time (they deepened in gain, so the first seconds were nearly silent); ARP flashes with
  the beat from the moment it's on (it lit by the gain, so only once the dips were deep).

- 2026-10-11 (Evan): LFO 2's picture shows all four knobs: the rate-modded wave over its plain shape and the
  rate's swing (top), cross-mod's moving filter curve (bottom left), the delay time's wobble (bottom
  right). Rate mod is linear now (-25 swings the rate x0.6..x1.7; it was x0.9..x1.1), +-100 unchanged.

- 2026-10-11 (Evan): the default sound breathes a little (filter LFO depth +25 on drone I, -25 on drone II,
  the LFO at ~0.23 Hz), so the Movement is heard at once (its depths were all 0: OCT-/+ scaled nothing).
  Measured: the cutoffs swing 7.4 semitones at 100 %, 0 at 0 %, 14.7 at 200 %. dr_cut now includes the
  LFO, so Play 2's filter curves move with it. The engine tests zero the filter depths in dry_init.

- 2026-10-11 (Evan): the Swell goes in and out like a breath: an exponential approach (~98 % in the Morph
  time: most of the change first, easing into place), not a straight line (which hung on, then seemed to
  snap off when its dips fell under the drone's own ~5 dB flutter).

- 2026-10-11 (Evan): FEEDBACK (Effects 2 KNOB 3): the finished mono sound (after the effects) fed back
  into both drones' filters through their drive, after exactly 256 samples (a ring: the same loop
  whatever the block), its lows cut (80 Hz); gain linear to 1.6 (dry, it sustains past ~60; with the
  delay, ~50), the heard sound turned down as it rises (not the loop's copy); held by the soft clips
  (worst case tested inside full scale). CHARACTER (KNOB 4, global): a one-pole to ~1.6 kHz, x^2 (DC
  blocked), +-6 cents of ~20 Hz jitter, drone II up to 6 cents sharp, -44 dB noise; skipped at 0.
  PROJ_FORMAT 16.

- 2026-10-11 (Evan: it got out of control at 50): Feedback never sustains itself now: the loop's gain at 100
  is 0.85 (FB_MAX; it was 1.6 and ran away past ~50) and what it adds is capped by a tanh at 0.35
  (FB_CEIL); the heard sound is turned down less (0.4x). Tested: stopped, the tail at 100 is the
  effects' own, as at 0.

- 2026-10-11 (second review; Evan: do the clear fixes): scenes leave the room's settings alone (Fade, Tune,
  Speaker, Pitch lock, Morph, Scene time: scene_room); a sub under 20 Hz goes up an octave (a 20 Hz output
  low cut was tried: its ringing upset the delay and panic tests); the filter key goes round LP, BP, HP
  (BP winks); PW / Stereo instead of two "Width"s; cutoff and resonance run 0..1000 (a detent 10, SEL held
  1: fine), shown as 0..100 with a tenth. PROJ_FORMAT 17. Not doing (Evan): intervals between the drones
  (tune by ear), SEL + OCT.

- 2026-10-11 (second review; Evan): Osc = X-Wave + PW, Tone = Drive + Osc mix; LFO 2 renamed Mod (every
  page is full, so X-mod amount and the delay LFO depth stay: as "modulation amounts" they belong there).
  Two quick ARP taps (< 350 ms) open the Swell page to stay (the first tap's toggle undone). The normal
  black keys by meaning: drone I (partner, sub, filter) | pitch lock, LFO reset | drone II | pitch link,
  hard sync | cross-mod (the LFO layer unchanged).

- 2026-10-11 (second review; Evan): SAVE and storing a scene while playing don't write the flash (a write
  stops the sound for a moment): they mark it dirty, and the autosave writes it after PLAY stops (the top
  bar's dot). SEL + PLAY fades drone II alone out / in (dr_voice; "II OUT" in the top bar). The Swell's
  default rate is ~0.5 a second (45); the LFO rate goes down to -30 (0.0017 Hz, a cycle in ~10 min; shown
  as a period, "9m48"). A Stage view (large values on a third HOME page) was tried and dropped (Evan).

- 2026-10-11 (Evan): a white key held is a visit: it morphs to the scene and, let go after HOLD_MS,
  morphs back to the sound before it (visit_back, over the Scene time; a knob turned during it makes it a
  stay). A tap stays, as before. The keys keep one job (scenes): no notes or sequencer.

- 2026-10-11 (Evan): the name is **Niagara Drift** (the falls' endless roar + La Monte Young's Drift
  Studies; the drones drifting against each other). Tagline: "a drone synth for the M-VAVE FM-1". In the
  README, LICENSING, BUILDING, the emulator page and code headers; the repo, the ~/fm1-drone folder, the
  build files (omni.*) and the emulator's storage key keep their old names for now.

- 2026-10-11 (Evan): DRIFT widened ("never fully in control", the name's idea): besides the pitch drift,
  four slow random wanders per drone (cutoff +-0.5 oct, X-WAVE +-0.12, level +-2.4 dB, partner detune
  +-10 cents, at 100), each a new aim every 10..40 s glided to over ~8 s, around the setting. Drift 0:
  still. Measured over 60 s: cutoff still at 0, about +-0.25 unit at 30, about +-4 at 100.

- 2026-10-11 (Evan): NOISE (the name is just a name: nothing literal, no waterfall): per drone, into its
  filter like the oscillators, on a new Tone 2 page (ENV again: KNOB 1-2 drone I's Noise and Colour, 3-4
  drone II's; every other page was full). Colour: white or pink only (Evan: no brown), a two-step knob (0
  white, 1 pink; every key is taken) that crossfades over the Morph as a switch.
- 2026-10-11 (Evan, as his Moog Grandmother's mixer): Noise is a LEVEL added beside the oscillators, which
  stay as they are (a first build crossfaded them: too much noise, and not how synths work). Never louder
  than them, yet heard all along the knob: a straight level, 100 is 6 dB under them, 50 -12, 25 -18
  (a squared curve with 100 as loud as them was too loud at the top and too faint below the middle).
- 2026-10-11 (engineering): Colour: white through six one-pole shelves two octaves apart (10 Hz .. 10 kHz),
  each adding g of its input back, g = 2^(-2 c), c 0 white .. 1/2 pink (between only while the switch
  crossfades): slopes 0, -3 dB/oct (pink measured -9.5 dB a decade against white over 250 Hz .. 2.5 kHz),
  then a 25 Hz low cut; the loudness made up so white and pink are about as loud (within ~1 dB). Each drone its own generator. The tests measure the noise alone as the difference of two renders,
  with and without it. Cost ~71 instructions a sample a drone (valgrind, x86), ~1.2 % of the device; ~0 at
  Noise 0, where the sound is bit-identical to before. dr_init now seeds the drift and LFO generators, so
  every init sounds the same (the first is unchanged). GP_N 21: scenarios renumbered. PROJ_FORMAT 18.
- 2026-10-11 (Evan): the screen's page names match the web manual: a page with a second page is numbered
  (Play 1 / Play 2, Tone 1 / 2, LFO 1 / 2, Setup 1 / 2, Effects 1 / 2), since "1" says there's a "2";
  Osc, Partner and Swell stay unnumbered. LFO 2 was "Mod".
- 2026-10-11 (Evan): "Scene time" renamed "Scene switch" (screen, manual, README); in the manual Morph and
  Scene switch are both described as a "switch time" (Morph: buttons and keys; Scene switch: scenes).
- Linux (a cloud session): the tests need `CC="cc -D_DEFAULT_SOURCE -Wl,--no-as-needed -lm"` (macOS needs
  neither); no instruction counter, so measure cost with valgrind's callgrind, against `main`.

## Milestones

- [x] 1. Repo started from FoMni; host simulator, scenarios and browser emulator build and pass unchanged.
- [x] 2. JieLi toolchain (jieli-linux-toolchains-20250324.1) + AC79 SDK installed; the unchanged
  package builds in Docker in ~15 s (`build/omni.fwsc`, identity FM-1_800) and all tests pass.
- [x] 3. Minimal drone engine (`dsp/drone.c`), tested (`tests/host/drone_test.c`), scenes rendered to
  `build/drone/*.wav` by `host/drone_render.c`. Cross-compiles for pi32v2 with no soft-double calls.
- [x] 4. CPU measured: ~12 % of the device estimated for 4 voices (`tests/host/drone_cost.c`).
- [x] 5. The app, simulator and browser emulator play the drone (`tests/scenarios/drone.omni`, 44 checks).
  Evan runs it with `emulator.command`.
- [x] 6. Control map built, its open questions answered; effects (delay, reverb); screen part 1 (PR 17).
- [ ] 7. Hardware bring-up (FM-1 due ~2026-10-21).

- 2026-10-11 (QA batch 1; Evan: fix the issues): gain staging (Drive's soft clip starts clean, its make-up
  keeps the level; the output is straight up to 0.7 of full scale, then soft, 3 dB less into it; the sine at
  0.95), Unison with no Detune phase-locks the partner, MASTER's turn maps the same on device and emulator
  (square law to 4/5, then the drive), the page pictures without a stray pitch link. PRs #85-88.
- 2026-10-11 (Evan): the Swell keeps fading in and out over the Morph time (at the default 0.6 s it is
  nearly a switch; a longer Morph gives a breath).
- 2026-10-11: Felucca is still GPL-3.0-only (checked on GitHub: its licence unchanged since its first commit).
  What's published under the GPL stays usable; a future licence change would only stop us copying newer code.
- 2026-10-11 (engineering): the unused Terminus fonts removed (PR #90); CLAUDE.md is the handover, this log
  moved out of it (PR #91).
- 2026-10-11 (QA batch 2; Evan: fix them all, and four suggestions): Random draws a new level at every
  restart (it froze locked to the Swell) and is seeded at the first PLAY (the same every power-up before;
  an init alone still repeats, for the tests); rate mod keeps the tempo (the step scaled by the cycle's mean
  of 2^(-rm * wave)); the LFO phase in 32 bits (the slowest rate ran 2 % fast) (#93). The Swell: Shape firms
  a sine breath (a gate only at 100, no steps), Depth in dB (0 .. -30 at 99, 100 silence), a 3 ms slew,
  defaults Shape 25 / Depth 48, '33s' at the slowest rate; PROJ_FORMAT 19 (#94). The filter LFO's depth
  linear on the resonance (#95). A locked pitch card stays dark (#96). A new LFO shape glides over the Morph;
  every shape centred (Roundtooth, Exp saw, Log tri: their swing a little smaller); 2x / 4x Exp at their
  rates (#97). docs/ds2-notes.md removed (Evan: after QA batch 2).
- 2026-10-11 (QA batch 3; Evan: fix them all, and his picks): Delay mix, Reverb mix, Repeats and Stereo read
  as they glide, ramped across each block (each detent clicked); the Speaker cut crossfades on and off; a
  reverb type change fades the sound into the rebuilt tank (it crackled); Repeats gets more loop gain over 85
  (#99). Glide follows a knob's slide, so it smooths knob turns, scenes and the motion loop, not only MIDI
  (#100). The motion loop joins a quick turn's detents (100 ms), says FULL and loops if the 4096 events fill;
  REC shows its time (#101). Character's jitter about +-6 cents (it was ~1) and its softening corner in even
  steps, 8 kHz .. 1.2 kHz (#102). Stereo is a room setting: scenes leave it alone (#103). A Speaker picture,
  the reverb lengths as measured, an amber unsaved dot, VISIT while a scene key is held (#104). The emulator
  page's wording, and the switch-on pill over the keys on a phone (#105).
- 2026-10-14 (first install on Evan's FM-1): stock V15 (FM-1_015) worked; V15 checked (SHA-256) and the rescue
  tool set up; `fm1_install.py` wrote FM-1_800 and it started: the animation smooth, two drones, knobs, pages
  and black keys fine. Found: FoMni's boot splash ("A chord harp for the FM-1") showed before ours; now the
  platform clears to black (our animation fades in from black), and the crash and safe-mode screens drop FoMni's name.
  The USB names still say FoMni (left for the release prep: the installers match on them).
- 2026-10-14 (Evan, on the FM-1: the screen text is too small; all of it, about a quarter bigger): the
  fonts 12 / 15 / 15 / 23 px become 15 / 19 / 19 / 28 (the startup title unchanged). A knob name or value
  too wide for its card has its letters a pixel closer; a value that still doesn't fit steps down a face.
  Two names are shorter on their cards only: Scenes (Scene switch), X-mod (X-mod amt). The wave names on
  Osc and the second lines on Setup 1 / 2 stay at the old body size, to fit.
- 2026-10-14 (on the FM-1, after #110-#112): no FoMni screen; the bigger text is good (Evan). CPU measured
  with `tools/fm1_cpu.py`: 3.5 % stopped, ~39 % with the default sound playing (38.7-41.4 %), the longest
  block 2763 us of 5804 (48 %), no late blocks. The host's 21 % estimate was low by nearly half. Headphones
  work; the speaker is a little tinny (Evan); Speaker low cut stays off by default for now.
- 2026-10-14 (Evan, on the FM-1: 20 detents a turn; a slow click (60 cents) is right, a fast spin went too
  far; about half as far): the pitch knobs' speed-up tops out at 2x (it was 4x), on the same curve, so a quick
  full turn is about 1.7 octaves, not 3. The other knobs keep theirs (a cutoff sweep wants it; Evan's call).
- 2026-10-14 (Evan, on the FM-1: lit scene keys don't say which one you're on; a slow blink, until you change
  the sound): the scene stored or recalled last blinks slowly (0.6 s on, 0.4 off); turning a knob, SELECT /
  ALGORITHM / PRESETS or OCT, or flipping a black-key switch clears it (the LFO reset key doesn't, nor do
  Drift, the LFOs, the motion loop or MIDI). A visit blinks the visited key, and letting go restores the mark.
- 2026-10-14 (measured on the FM-1, then Evan: save while playing, with a quick dip): three saves while stopped
  lost no audio block (15493 in 89.9 s, as due), so the flash erase stops the audio for under one 5.8 ms block;
  the write clears the audio buffer first, which would click. SAVE and a scene store now write at once while
  playing: the output dips out (20 ms, a straight line, after the clip), the write, back (30 ms). The autosave
  still waits for the stop (it would dip by itself).
- 2026-10-14 (Evan: the review's ~10-minute LFO is too long; the DS-2's slowest, 50 s): the LFO rate runs 0..100
  again (0.02 Hz, a cycle in 50 s, to 70 Hz), the DS-2's knob range (its CV reaches 0.006 Hz; the
  Subharmonicon's slowest pulse is ~48 s; the Lyra-8's manual gives no figures). A saved rate below it loads as
  the slowest (no project reset). Longer arcs: Scene switch (to 5 min) and the motion loop.
- 2026-10-14 (Evan: release prep; 0.1 beta; prepared as PRs, public only when he says go; FoMni's tags renamed
  fomni-v0.x; the USB name Niagara Drift FM-1): over USB the FM-1 is "Niagara Drift FM-1" (manufacturer
  "Niagara Drift"); the installers still find it by "FM-1". A release is `./build.sh --release 0.1-beta`:
  build/niagara-drift-0.1-beta.fwsc, identity FM-1_8000100, the startup line "niagara-drift 0.1 beta".
- 2026-10-14 (release prep 2): the site is the emulator page (build/emu) at the root, with a row of four
  actions (Install, Download, Full manual, Source) and a version / risk line that `tools/make_pages.py` puts in
  at `<!--ACTIONS-->` (the local emulator has none); FoMni's landing page is gone. The web installer
  (web/installer.html, was omni_installer.html) is in the site's look, English only (FoMni's Japanese text
  dropped: the site is English), the same logic: install, resume a cut-off install, M-VAVE's V15 back (SHA-256
  checked). Served at https://evanyerburgh.github.io/niagara-drift/ once public.
- 2026-10-14 (Evan: before going public): CLAUDE.md (Claude's working notes) leaves the repo: ignored, kept
  only on Evan's Mac. Its earlier versions stay in the git history.
- 2026-10-14 (Evan, on the FM-1: Feedback could be stronger; B of three renders): the loop's gain at 100 is
  1.05 (was 0.85) and its ceiling 0.6 of an oscillator (was 0.35): from ~95 it sustains itself and sings, about
  8 dB louder at 100 than before, bounded (worst peak 0.989); stopped, it fades as at 0.
- 2026-10-14 (Evan, on the FM-1: B, singing, too strong; E of three renders): Feedback's loop gain at 100 0.9,
  its ceiling 0.45: much more texture than the first (0.85 / 0.35), no louder at 100 than without it (-10.5 dB
  on the default sound, as with none).
- 2026-10-16 (v0.2; Evan: the crossfade will be played by key pressure, so read it first): key pressure ported
  from MONK-1 (Charles Vestal; Velo's sensing): the scan tick samples the two analog key rows (the 20 lower keys,
  F3 .. C5) and owns the converter, so MASTER is read there too; the seven top keys (C#5 .. G5) have no
  converter and no pressure. MONK-1's settings (DRIVE 2, WAIT 2, no calibration) and its pressure curve
  (counted from where the key rests, Sensitive 50). Nothing plays from it yet: a held key shows its pressure on
  the screen (a test readout), and it goes out as polyphonic aftertouch over USB for measuring. To try on the
  FM-1 before the crossfade is designed on it.
