---
paths:
  - "firmware/src/dsp/**"
  - "tests/host/**"
  - "host/**"
---

# Audio code (the engine, firmware/src/dsp)

The aim: the best sound the FM-1 can make, inside its budget (`.claude/CLAUDE.md`). None of these rules
costs sound. They're how this code gets quality and speed at once.

## Every sample is expensive

- Per sample: multiply, add, compare, the `fm_*` helpers in `fastmath.h`. Not libm (`sinf`, `expf`,
  `powf` are slow and not bit-identical across host and device), not a divide (multiply by a reciprocal
  computed once a block), no branches that depend on the signal when a min / max or a blend will do.
- Per block (256 samples): anything derived from a knob, an LFO's slow parts, filter coefficients,
  gains. Ramp them across the block (a linear step a sample) so a change never steps audibly.
- Never in the audio path: allocation, logging, flash reads or writes, waits.
- Skip what isn't heard: a stage at zero depth or mix costs nothing (check before computing it), a
  stopped drone renders silence cheaply ("drone stopped" in drone_cost).
- Look at the cost before and after with `drone_cost` (the Mac) or CI's valgrind table. Add a case to
  drone_cost when a new stage can be on in the worst case.

## Single precision, used carefully

A float holds 24 bits, more than the FM-1's 24-bit output, so precision is fine where values stay near 1.
It runs out where a value grows or a step is tiny next to it:

- Phase: wrap every sample (turns in 0..1); slow phases (LFOs under ~0.1 Hz, anything that runs for
  minutes) in a `uint32_t` (as `lfo_q`): a float phase drifted ~2 % at the slowest LFO rate.
- Long indexes (the delay line) as integers plus a fractional part, never one float.
- Very low filter cutoffs (the pitch reaches 2 Hz, the speaker's low cut) need a form that stays
  accurate there; check by rendering, not by reasoning.
- Write constants as `1.0f`, never `1.0` (a double constant pulls in soft-double: the build fails).
- `fm_flush` feedback paths and filter states that decay, so denormals don't stall the FPU.

## Quality

- **Aliasing:** oscillators stay band-limited (polyBLEP); a new non-linearity (drive, fold, FM, a hard
  clip) is a new source of aliasing: soft curves, and measure it (a sweep, the spectrum above 10 kHz).
- **Clicks:** every jump (a type change, a scene, a gesture, a restart) is crossfaded or ramped; zipper
  noise from a knob is a bug.
- **Level:** a change that adds energy takes it back off (Feedback, the drive after the delay and the
  MASTER drive hold loudness); loudness is the mean of A-weighted and 200 Hz high-pass power.
- **Noise floor:** the int16 delay line and the half-rate reverb are the known weak spots: a change near
  them measures the noise, not just the tone.
- Prove a sound claim with a render and numpy, as the existing tests do; the device is the final judge.
