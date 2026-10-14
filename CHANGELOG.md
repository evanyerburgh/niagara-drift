# Changelog

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
