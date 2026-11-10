/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift project (from FoMni's): the sound and bank A in one flash object (OBJ_PROJ), banks B and C in
 * one each (OBJ_BANKB, OBJ_BANKC); the Presets bank is the firmware's, fixed (Evan, 2026-10-19; OBJ_BANKP held
 * the ones saved over before, moved to the banks at load). A project of another format, or none,
 * gives the defaults; 0.1's (19) and v0.2's first (20) are converted. Part of the unity build, after app.h. */
project_t proj;
static bank_t bank_bc[2];                  /* banks B and C */
static bank8_t bank_x[3];                  /* banks D, E, F (v0.3; 8 scenes each, 5.4 KB of RAM) */
static uint8_t bank_dirty;                 /* bit 1 B, bit 2 C, bit 3 the old Presets object, bits 4-6 D-F: to be written
                                            * with the next save */
static const uint8_t BANKX_OBJ[3] = {OBJ_BANKD, OBJ_BANKE, OBJ_BANKF};
#define BX(b) ((b) - BANK_D)               /* bank D, E, F (4-6) -> 0..2 */
#define BANK_X(b) ((b) >= BANK_D && (b) < BANK_D + 3)
static uint16_t p_moved;                   /* the old Presets object's scenes moved to the banks, to be cleared there */
static const uint8_t BANK_OBJ[3] = {OBJ_BANKB, OBJ_BANKC, OBJ_BANKP};
static uint32_t proj_h;                    /* a hash of the project as on the flash, 0 = unknown: a save that
                                            * would write the same bytes skips it (each write is a late block) */
static uint32_t proj_hash(void)
{
    const uint8_t *p = (const uint8_t *)&proj;
    uint32_t h = 2166136261u, i;               /* FNV-1a */
    for (i = 0; i < sizeof proj; i++)
        h = (h ^ p[i]) * 16777619u;
    return h | 1u;                             /* never 0 */
}

/* the Presets (0.3, Evan, 2026-11-09, after eight audition rounds): two banks of eight, P1 the starting sound, P16 the
 * bare one (one oscillator, everything off); between them sounds that move and show what the synth can do, in
 * different moods. Each a list of changes from the defaults, ended by -1; all fixed. The drones' levels set so each
 * one's loud moments meet P1's (the mean of A-weighted and 200 Hz high-passed power, 95th percentile of 0.4 s) */
#define I_(p) DR_GP(0, p)
#define II_(p) DR_GP(1, p)
#define PRESET_N 100
static const int16_t PRESET_CHANGES[][PRESET_N + 1] = {
    /* P1 NIAGARA: the starting sound (0.2's P1, unchanged) */
    {-1},
    /* P2 EMBERS: triangles blooming toward saws and back, the two drones in turn, a pure-third partner each, a chorus
     * hall */
    {I_(GP_WAVE), 40, I_(GP_INTERVAL), 13, I_(GP_DETUNE), 10, I_(GP_CUTOFF), 700, I_(GP_RESO), 300,
     I_(GP_OLFO_DEPTH), 71, I_(GP_OLFO_DEST), 1, II_(GP_PITCH), 5386, II_(GP_WAVE), 40, II_(GP_INTERVAL), 13,
     II_(GP_DETUNE), 10, II_(GP_CUTOFF), 700, II_(GP_RESO), 300, II_(GP_LEVEL), 80, II_(GP_OLFO), 1,
     II_(GP_OLFO_DEPTH), -71, II_(GP_OLFO_DEST), 1, P_LFO_RATE, 78, P_PULSE_DEPTH, 30, P_DLY_MIX, 20, P_REV_MIX, 50,
     P_REV_TYPE, 3, P_CHAR, 15, P_DLY_TYPE, 1, -1},
    /* P3 VESPERS: saws through band-passes near a sung "ah", the waves morphing, a bright Cathedral */
    {I_(GP_INTERVAL), 9, I_(GP_MIX), 10, I_(GP_CUTOFF), 520, I_(GP_RESO), 450, I_(GP_MODE), 1, I_(GP_LEVEL), 100,
     I_(GP_OLFO_DEPTH), 45, I_(GP_OLFO_DEST), 1, I_(GP_FLFO_DEPTH), 18, II_(GP_WAVE), 67, II_(GP_INTERVAL), 13,
     II_(GP_MIX), 0, II_(GP_CUTOFF), 595, II_(GP_RESO), 450, II_(GP_MODE), 1, II_(GP_LEVEL), 100,
     II_(GP_OLFO_DEPTH), 55, II_(GP_OLFO_DEST), 1, II_(GP_FLFO_DEPTH), -18, P_LFO_RATE, 45, P_PULSE_RATE, 20,
     P_PULSE_DEPTH, 30, P_DLY_MIX, 15, P_REV_MIX, 55, P_MOVE, 60, P_REV_LEN, 85, P_REV_DAMP, 20, -1},
    /* P4 DREAMHOUSE: on Harmonics: A1 and its 7th harmonic, sines, partners pure fifths, the resonance swelling, a
     * little Feedback */
    {I_(GP_PITCH), 3300, I_(GP_WAVE), 20, I_(GP_INTERVAL), 13, I_(GP_DETUNE), 0, I_(GP_MIX), 0, I_(GP_CUTOFF), 700,
     I_(GP_RESO), 350, I_(GP_LEVEL), 66, I_(GP_FLFO_DEPTH), 70, I_(GP_FLFO_DEST), 1, II_(GP_PITCH), 6669,
     II_(GP_WAVE), 0, II_(GP_INTERVAL), 13, II_(GP_DETUNE), 0, II_(GP_MIX), 0, II_(GP_CUTOFF), 680,
     II_(GP_RESO), 300, II_(GP_LEVEL), 48, II_(GP_FLFO_DEPTH), -60, II_(GP_FLFO_DEST), 1, P_LFO_RATE, 50,
     P_GLIDE, 16, P_PULSE, 0, P_DLY_MIX, 15, P_REV_MIX, 55, P_FEEDBACK, 25, P_WSCALE, 5, P_REV_LEN, 85, P_TUNING, 6,
     -1},
    /* P5 ABYSS: dark: D3 over B2, a third's subharmonic, closed resonant filters swept slowly on their resonance
     * (0.2's P2) */
    {I_(GP_INTERVAL), 0, I_(GP_MIX), 45, I_(GP_SUBOCT), 3, I_(GP_CUTOFF), 650, I_(GP_RESO), 450, I_(GP_DRIVE), 70,
     I_(GP_LEVEL), 100, I_(GP_FLFO_DEPTH), 100, I_(GP_FLFO_DEST), 1, I_(GP_NOISE), 8, II_(GP_PITCH), 4700,
     II_(GP_WAVE), 100, II_(GP_WIDTH), 70, II_(GP_DETUNE), 30, II_(GP_CUTOFF), 610, II_(GP_RESO), 450,
     II_(GP_DRIVE), 60, II_(GP_LEVEL), 100, II_(GP_FLFO_DEPTH), -100, II_(GP_FLFO_DEST), 1, P_LFO_RATE, 61,
     P_DLY_TIME, 85, P_DLY_REPEATS, 45, P_DLY_MIX, 15, P_REV_MIX, 40, P_REV_TYPE, 1, P_FEEDBACK, 30, P_CHAR, 27,
     P_DLY_TYPE, 1, P_REV_LEN, 75, P_REV_DAMP, 80, -1},
    /* P6 SEANCE: eerie: Eerie's hollow bass (D3, a fifth under it) under a high band-pass voice (A4 -19) on Harrison;
     * cross-mod, long swinging echoes, a chorus hall; Wander at its slowest */
    {I_(GP_WAVE), 20, I_(GP_INTERVAL), 1, I_(GP_DETUNE), 35, I_(GP_CUTOFF), 600, II_(GP_PITCH), 6881,
     II_(GP_WAVE), 27, II_(GP_INTERVAL), 8, II_(GP_DETUNE), 8, II_(GP_MIX), 20, II_(GP_CUTOFF), 615,
     II_(GP_RESO), 688, II_(GP_MODE), 1, II_(GP_DRIVE), 26, II_(GP_LEVEL), 62, II_(GP_OLFO_DEPTH), 10,
     II_(GP_NOISE), 10, II_(GP_NCOLOR), 18, P_XMOD, 1, P_LFO_RATE, 73, P_LFO_W1, 6, P_GLIDE, 37, P_PULSE, 0,
     P_DLY_TIME, 111, P_DLY_REPEATS, 68, P_DLY_MIX, 33, P_REV_MIX, 56, P_REV_TYPE, 3, P_LFO_RMOD, 33,
     P_LFO_RMOD_SRC, 1, P_MOVE, 66, P_WANDER, 1, P_WSCALE, 3, P_DLY_TYPE, 0, P_REV_LEN, 66, P_REV_DAMP, 39,
     P_DLY_DRIVE, 7, P_TUNING, 8, -1},
    /* P7 SONAR: drone I at 2.7 Hz: a click each cycle through the delay, drone II's resonant filter singing as it
     * sweeps */
    {I_(GP_PITCH), -1900, I_(GP_MIX), 0, I_(GP_CUTOFF), 900, I_(GP_LEVEL), 64, I_(GP_PARTNER), 0,
     II_(GP_PITCH), 4500, II_(GP_WAVE), 67, II_(GP_CUTOFF), 620, II_(GP_RESO), 850, II_(GP_LEVEL), 88,
     P_LFO_RATE, 94, P_PULSE, 0, P_DLY_TIME, 45, P_DLY_MIX, 40, P_REV_TYPE, 1, P_DLY_TYPE, 0, P_REV_LEN, 55,
     P_DLY_DRIVE, 25, -1},
    /* P8 SIBYL: a voice: D4 and F#5, sines, the LFO morphing each wave (the "ah"), a gentle Swell, a bright Cathedral
     * (0.2's Heaven, moving more) */
    {I_(GP_PITCH), 6200, I_(GP_WAVE), 0, I_(GP_INTERVAL), 9, I_(GP_DETUNE), 15, I_(GP_MIX), 0, I_(GP_CUTOFF), 950,
     I_(GP_RESO), 50, I_(GP_LEVEL), 65, I_(GP_OLFO_DEPTH), 65, I_(GP_OLFO_DEST), 1, II_(GP_PITCH), 7800,
     II_(GP_WAVE), 10, II_(GP_INTERVAL), 13, II_(GP_DETUNE), 15, II_(GP_MIX), 0, II_(GP_CUTOFF), 950,
     II_(GP_RESO), 50, II_(GP_LEVEL), 61, II_(GP_OLFO_DEPTH), 75, II_(GP_OLFO_DEST), 1, P_DRIFT, 40, P_PULSE_RATE, 8,
     P_PULSE_DEPTH, 30, P_PULSE_SHAPE, 0, P_DLY_TIME, 80, P_DLY_REPEATS, 60, P_REV_MIX, 65, P_MOVE, 55,
     P_DLY_TYPE, 1, P_REV_LEN, 92, P_REV_DAMP, 20, -1},
    /* P9 HOWL: D2 and Eb2 grinding, Feedback 85, a random LFO on the filters, quick enough
     * that the grind comes back however Howl is started (Evan, 2026-11-10; 0.2's P9) */
    {I_(GP_PITCH), 3800, I_(GP_CUTOFF), 740, I_(GP_RESO), 600, I_(GP_DRIVE), 75, I_(GP_LEVEL), 92,
     I_(GP_FLFO_DEPTH), 100, I_(GP_FLFO_DEST), 1, II_(GP_PITCH), 3900, II_(GP_CUTOFF), 780, II_(GP_RESO), 600,
     II_(GP_MODE), 1, II_(GP_DRIVE), 75, II_(GP_LEVEL), 92, P_LFO_RATE, 100, P_LFO_W1, 8, P_DLY_TIME, 20,
     P_DLY_REPEATS, 45, P_DLY_MIX, 40, P_REV_MIX, 50, P_REV_TYPE, 0, P_MOVE, 40, P_FEEDBACK, 85, P_CHAR, 21,
     P_REV_LEN, 40, -1},
    /* P10 CISTERN: reeds (Deep Listening's cistern): saw-square drones, drone I rounder with a sine sub, a bellows
     * Swell, a Tape delay a second long, the Shimmer reverb */
    {I_(GP_WAVE), 45, I_(GP_WIDTH), 0, I_(GP_MIX), 35, I_(GP_CUTOFF), 600, I_(GP_RESO), 150, I_(GP_DRIVE), 10,
     I_(GP_LEVEL), 87, I_(GP_FLFO_DEPTH), 45, II_(GP_PITCH), 5702, II_(GP_WAVE), 65, II_(GP_DETUNE), 6,
     II_(GP_MIX), 0, II_(GP_CUTOFF), 690, II_(GP_RESO), 300, II_(GP_DRIVE), 25, II_(GP_LEVEL), 77,
     II_(GP_FLFO_DEPTH), -45, P_LFO_RATE, 30, P_PULSE_RATE, 18, P_PULSE_SHAPE, 0, P_DLY_TIME, 145, P_DLY_REPEATS, 55,
     P_DLY_MIX, 28, P_REV_MIX, 60, P_REV_TYPE, 4, P_DLY_LFO_DEPTH, 22, P_MOVE, 40, P_REV_LEN, 99, P_REV_DAMP, 35,
     P_SUBW_0, 1, -1},
    /* P11 PISTON: the low D stepping a whole tone up and back about once a second (AND 3), driving drone II's cutoff;
     * a slow Swell */
    {I_(GP_PITCH), 3800, I_(GP_CUTOFF), 620, I_(GP_RESO), 600, I_(GP_DRIVE), 40, I_(GP_LEVEL), 86, I_(GP_PARTNER), 0,
     I_(GP_OLFO_DEPTH), 29, I_(GP_FLFO_DEPTH), 0, II_(GP_CUTOFF), 640, II_(GP_RESO), 400, II_(GP_LEVEL), 74,
     II_(GP_FLFO_DEPTH), 0, P_XMOD, 1, P_XMOD_AMT, 45, P_LFO_RATE, 112, P_LFO_W1, 9, P_LFO_W2, 0, P_PULSE_RATE, 10,
     P_PULSE_DEPTH, 45, P_PULSE_SHAPE, 0, P_DLY_TIME, 55, P_DLY_MIX, 30, P_REV_MIX, 25, P_FEEDBACK, 20,
     P_DLY_TYPE, 2, -1},
    /* P12 ASHRAM: a warm organ on Eb: a filtered pulse with a sine sub, a second voice ~24 c sharp, the bass wandering
     * in Dorian on the Shruti Tuning, a chorus hall */
    {I_(GP_PITCH), 5100, I_(GP_WAVE), 85, I_(GP_WIDTH), 20, I_(GP_DETUNE), 80, I_(GP_MIX), 25, I_(GP_CUTOFF), 500,
     I_(GP_RESO), 100, I_(GP_DRIVE), 10, I_(GP_LEVEL), 100, I_(GP_FLFO_DEPTH), 20, II_(GP_PITCH), 3900,
     II_(GP_WAVE), 50, II_(GP_INTERVAL), 13, II_(GP_DETUNE), 6, II_(GP_MIX), 30, II_(GP_CUTOFF), 480,
     II_(GP_RESO), 60, II_(GP_DRIVE), 0, II_(GP_LEVEL), 100, II_(GP_FLFO_DEPTH), -15, P_LFO_RATE, 40, P_GLIDE, 20,
     P_DRIFT, 35, P_PULSE_RATE, 25, P_PULSE_DEPTH, 25, P_PULSE_SHAPE, 0, P_DLY_TIME, 120, P_DLY_REPEATS, 35,
     P_DLY_MIX, 10, P_REV_TYPE, 3, P_DLY_LFO_DEPTH, 0, P_CHAR, 20, P_WANDER, 35, P_AGE, 20, P_WSCALE, 4,
     P_REV_LEN, 75, P_REV_DAMP, 50, P_SUBW_0, 1, P_SUBW_1, 1, P_TUNING, 9, -1},
    /* P13 TELEGRAPH: a stepped figure (OR 3) on drone II's pitch and filter over a low D; Wander moves the whole
     * figure every ~20 s */
    {I_(GP_PITCH), 3800, I_(GP_CUTOFF), 600, I_(GP_LEVEL), 88, I_(GP_FLFO_DEPTH), 0, II_(GP_WAVE), 100,
     II_(GP_RESO), 720, II_(GP_DRIVE), 50, II_(GP_LEVEL), 100, II_(GP_PARTNER), 0, II_(GP_OLFO), 1,
     II_(GP_OLFO_DEPTH), -70, II_(GP_FLFO), 1, II_(GP_FLFO_DEPTH), 40, P_LFO_RATE, 113, P_LFO_W1, 10, P_GLIDE, 0,
     P_PULSE_RATE, 9, P_PULSE_DEPTH, 35, P_PULSE_SHAPE, 0, P_DLY_TIME, 40, P_DLY_REPEATS, 45, P_DLY_MIX, 30,
     P_REV_MIX, 20, P_WANDER, 10, P_WSCALE, 2, P_DLY_TYPE, 2, -1},
    /* P14 TAPE: worn: D3 and F3, a wide square and a saw, Age and Character, a 0.54 Hz wow (0.2's P7, more worn) */
    {I_(GP_WAVE), 100, I_(GP_WIDTH), 35, I_(GP_CUTOFF), 720, I_(GP_LEVEL), 87, I_(GP_OLFO_DEPTH), 7,
     I_(GP_FLFO_DEPTH), 40, I_(GP_NOISE), 18, II_(GP_PITCH), 5300, II_(GP_WAVE), 67, II_(GP_CUTOFF), 700,
     II_(GP_LEVEL), 87, II_(GP_OLFO), 1, II_(GP_OLFO_DEPTH), 6, II_(GP_FLFO_DEPTH), -40, II_(GP_NOISE), 12,
     P_LFO_RATE, 106, P_DLY_REPEATS, 55, P_DLY_MIX, 35, P_REV_MIX, 25, P_REV_TYPE, 0, P_FEEDBACK, 15, P_CHAR, 67,
     P_AGE, 45, P_REV_LEN, 35, -1},
    /* P15 FALLING: on LMY: sines on E4 and G3, the filters opening, a slow falling top, Feedback 64, Wander slow in
     * the bass */
    {I_(GP_PITCH), 6429, I_(GP_WAVE), 0, I_(GP_INTERVAL), 9, I_(GP_DETUNE), 15, I_(GP_MIX), 33, I_(GP_CUTOFF), 300,
     I_(GP_RESO), 50, I_(GP_DRIVE), 21, I_(GP_LEVEL), 94, I_(GP_OLFO), 3, I_(GP_OLFO_DEPTH), 65, I_(GP_FLFO), 2,
     II_(GP_PITCH), 5469, II_(GP_WAVE), 10, II_(GP_INTERVAL), 13, II_(GP_DETUNE), 15, II_(GP_MIX), 39,
     II_(GP_CUTOFF), 380, II_(GP_RESO), 50, II_(GP_DRIVE), 38, II_(GP_LEVEL), 88, II_(GP_OLFO), 3,
     II_(GP_OLFO_DEPTH), 70, II_(GP_OLFO_DEST), 1, P_XMOD, 1, P_XMOD_AMT, 41, P_LFO_RATE, 50, P_GLIDE, 0,
     P_DRIFT, 60, P_PULSE_DEPTH, 21, P_PULSE_SHAPE, 76, P_DLY_TIME, 80, P_DLY_REPEATS, 60, P_REV_MIX, 45,
     P_DLY_LFO_DEPTH, 28, P_LFO_RMOD, -100, P_LFO_RMOD_SRC, 3, P_MOVE, 35, P_FEEDBACK, 64, P_WANDER, 8,
     P_DLY_TYPE, 1, P_REV_LEN, 92, P_REV_DAMP, 20, P_SUBW_0, 1, P_SUBW_1, 1, P_TUNING, 7, -1},
    /* P16 BARE: one oscillator: drone I's saw alone through an open filter, everything else off; drone II out (locked) */
    {I_(GP_MIX), 0, I_(GP_CUTOFF), 1000, I_(GP_RESO), 0, I_(GP_DRIVE), 0, I_(GP_LEVEL), 80, I_(GP_PARTNER), 0,
     I_(GP_FLFO_DEPTH), 0, II_(GP_LEVEL), 100, II_(GP_PARTNER), 0, II_(GP_FLFO_DEPTH), 0, P_DRIFT, 0, P_PULSE, 0,
     P_DLY_MIX, 0, P_REV_MIX, 0, P_DLY_LFO_DEPTH, 0, P_II_OUT, 1, P_MOVE, 0, -1},
};
#define NPRESETS ((int)(sizeof PRESET_CHANGES / sizeof PRESET_CHANGES[0]))
static const char *const PRESET_NAME[] = {"NIAGARA", "EMBERS", "VESPERS", "DREAMHOUSE", "ABYSS", "SEANCE", "SONAR", "SIBYL",
                                          "HOWL", "CISTERN", "PISTON", "ASHRAM", "TELEGRAPH", "TAPE", "FALLING", "BARE"};
_Static_assert(sizeof PRESET_NAME / sizeof PRESET_NAME[0] == NPRESETS, "a name for each Preset");
const char *preset_name(int n)             /* a Preset's name */
{
    return n >= 0 && n < NPRESETS ? PRESET_NAME[n] : 0;
}
/* 0.2'S PRESETS THAT 0.3 DROPS (Evan, 2026-11-10): for a player updating from 0.2, into bank F (F1-F6; F is new in
 * 0.3, so empty), as they sound on 0.3's engine (main's lists before the new sixteen: #84's cutoffs, #78's LFO scale).
 * Niagara, Abyss, Tape and Howl live on as 0.3's Presets; Heaven is Sibyl, moving more; Clean is Bare. Nobody
 * else gets them */
#define OLD02_N 64
static const int16_t OLD02_CHANGES[][OLD02_N + 1] = {
    /* F1 EERIE (0.2's P3): a tritone apart, near-sines, drone II through a ringing band-pass with a breath of white
     * noise, its partner a just minor third; drone II wanders a whole-tone scale; the LFO's doubled curve, more
     * Movement; the longest clean echoes, a chorus hall. Drone II's pitch wavers a little (the oscillator LFO), the
     * LFO's own wave bending its rate: uneven */
    {I_(GP_PITCH), 5000, II_(GP_PITCH), 5600, I_(GP_LEVEL), 95, II_(GP_LEVEL), 76, I_(GP_WAVE), 20, II_(GP_WAVE), 33, I_(GP_INTERVAL), 1,
     II_(GP_INTERVAL), 7, I_(GP_DETUNE), 35, II_(GP_DETUNE), 8, II_(GP_MODE), DR_BP, II_(GP_CUTOFF), 640,
     II_(GP_RESO), 720, I_(GP_CUTOFF), 600, II_(GP_NOISE), 12, II_(GP_NCOLOR), 0, P_WANDER, 25, P_WSCALE, 3,
     P_GLIDE, 45, P_LFO_W1, 6, P_MOVE, 70, P_PULSE, 0, P_REV_TYPE, 3, P_REV_MIX, 55, P_DLY_TYPE, 0, P_DLY_TIME, 100,
     P_DLY_REPEATS, 70, P_DLY_MIX, 30, II_(GP_OLFO), DR_LFO_II, II_(GP_OLFO_DEPTH), 12, P_LFO_RMOD, 40,
     P_LFO_RMOD_SRC, DR_LFO_I, -1},
    /* F2 GLASS (0.2's P4): high sines a fifth apart (D5, A5), drone I's partner an octave up, beating a little, a long
     * bright cathedral */
    {I_(GP_PITCH), 7400, II_(GP_PITCH), 8100, I_(GP_WAVE), 0, II_(GP_WAVE), 0, I_(GP_INTERVAL), 14, I_(GP_DETUNE), 20,
     II_(GP_DETUNE), 25, I_(GP_CUTOFF), 900, II_(GP_CUTOFF), 900, I_(GP_RESO), 100, II_(GP_RESO), 100, I_(GP_MIX), 0,
     II_(GP_MIX), 0, P_PULSE, 0, P_REV_LEN, 85, P_REV_MIX, 60, P_REV_DAMP, 15, P_DLY_TIME, 70, P_DLY_MIX, 30,
     P_DLY_REPEATS, 60, I_(GP_LEVEL), 46, II_(GP_LEVEL), 42, -1},
    /* F3 TANPURA (0.2's P6): Sa and Pa: drone I on D3 with its high Sa (the partner an octave up), drone II on the Pa
     * below (A2), bright buzzing strings (the jawari: character, drive, an open resonant filter), plucked: the Swell
     * about once a second, swung; a hall. The LFO slow and gentle */
    {II_(GP_PITCH), 4500, I_(GP_WAVE), 67, II_(GP_WAVE), 67, I_(GP_INTERVAL), 14, II_(GP_INTERVAL), 14,
     I_(GP_DETUNE), 6, II_(GP_DETUNE), 6, I_(GP_MIX), 0, II_(GP_MIX), 0, I_(GP_CUTOFF), 820, II_(GP_CUTOFF), 780,
     I_(GP_RESO), 320, II_(GP_RESO), 320, I_(GP_DRIVE), 40, II_(GP_DRIVE), 40, P_CHAR, 27, P_PULSE_RATE, 50,
     P_PULSE_DEPTH, 45, P_PULSE_SHAPE, -40, P_PULSE_SWING, 15, P_MOVE, 30, P_REV_TYPE, 1, P_REV_LEN, 55,
     P_REV_MIX, 30, P_DLY_MIX, 10, I_(GP_LEVEL), 73, II_(GP_LEVEL), 68, -1},
    /* F4 WHISTLE (0.2's P8): the filters' resonance at its edge, so they sing as sines of their own: drone I's at D5
     * and drone II's at A5, over soft low drones (D2, A2) that feed them; the filter LFO drifts the whistles a little;
     * long clean echoes in a cathedral; no Swell */
    {I_(GP_PITCH), 3800, II_(GP_PITCH), 4500, I_(GP_WAVE), 20, II_(GP_WAVE), 20, I_(GP_CUTOFF), 497,
     II_(GP_CUTOFF), 556, I_(GP_RESO), 960, II_(GP_RESO), 960, I_(GP_DRIVE), 10, II_(GP_DRIVE), 10, I_(GP_MIX), 0,
     II_(GP_MIX), 0, I_(GP_FLFO_DEPTH), 12, II_(GP_FLFO_DEPTH), -12, P_PULSE, 0, P_MOVE, 40, P_DLY_TYPE, 0,
     P_DLY_TIME, 90, P_DLY_REPEATS, 55, P_DLY_MIX, 25, P_REV_TYPE, 2, P_REV_LEN, 80, P_REV_MIX, 50, I_(GP_LEVEL), 54, II_(GP_LEVEL), 51, -1},
    /* F5 CATHEDRAL (0.2's P10): a major sixth apart (D3, B3), soft waves, drone I's sub a third's subharmonic below
     * (49 Hz), a long dark reverb, almost a freeze. Almost no LFO: still and huge */
    {I_(GP_PITCH), 5000, II_(GP_PITCH), 5900, I_(GP_WAVE), 20, II_(GP_WAVE), 20, I_(GP_MIX), 35, I_(GP_SUBOCT), 3,
     I_(GP_CUTOFF), 650, II_(GP_CUTOFF), 650, P_PULSE, 0, P_REV_TYPE, 2, P_REV_LEN, 98, P_REV_MIX, 75, P_REV_DAMP, 55,
     P_DLY_MIX, 10, I_(GP_LEVEL), 50, II_(GP_LEVEL), 47, I_(GP_FLFO_DEPTH), 15, II_(GP_FLFO_DEPTH), -15, P_MOVE, 25, -1},
    /* F6 METAL (0.2's P11): cross-mod and hard sync, an inharmonic gap (a tritone and a bit), squares, drone I high-
     * passed, resonant, in a small room. The oscillator LFOs on X-Wave and a quicker filter sweep: a moving, metallic
     * shimmer */
    {I_(GP_PITCH), 3800, II_(GP_PITCH), 5730, P_XMOD, 1, P_XMOD_AMT, 60, P_SYNC, 1, I_(GP_WAVE), 100,
     II_(GP_WAVE), 100, I_(GP_WIDTH), 50, I_(GP_MODE), DR_HP, I_(GP_CUTOFF), 520, II_(GP_CUTOFF), 750,
     I_(GP_RESO), 650, II_(GP_RESO), 400, I_(GP_DRIVE), 55, II_(GP_DRIVE), 55, P_REV_TYPE, 0, P_REV_MIX, 25,
     I_(GP_LEVEL), 73, II_(GP_LEVEL), 73, P_DLY_TYPE, 0, P_DLY_TIME, 10, P_DLY_REPEATS, 30, P_DLY_MIX, 25,
     I_(GP_OLFO_DEST), DR_TO_XWAVE, I_(GP_OLFO_DEPTH), 45, II_(GP_OLFO), DR_LFO_II, II_(GP_OLFO_DEST), DR_TO_XWAVE,
     II_(GP_OLFO_DEPTH), 45, P_LFO_RATE, 110, I_(GP_FLFO_DEPTH), 50, II_(GP_FLFO_DEPTH), -50, -1},
};
#define NOLD02 ((int)(sizeof OLD02_CHANGES / sizeof OLD02_CHANGES[0]))
_Static_assert(NOLD02 == 6 && NOLD02 <= NPLAY, "0.2's six dropped Presets fit bank F");

_Static_assert(NPRESETS == 2 * NPLAY && PRESET_LOCKED < NPRESETS, "a Preset for every key of the two banks, the bare one locked");
static int16_t preset[DR_NPARAMS];         /* one Preset, built when it's asked for (all twelve were 2.4 KB of RAM) */
static int preset_n = -1;                  /* ... which */

static void changes_build(int16_t *d, const int16_t *c, int max)   /* the defaults, then a list of changes */
{
    int k;
    for (k = 0; k < DR_NPARAMS; k++)
        d[k] = dr_param_info(k)->def;
    for (k = 0; k < max && c[k] >= 0; k += 2)
        d[c[k]] = c[k + 1];
}

static const int16_t *preset_get(int n)
{
    if (n != preset_n) {
        changes_build(preset, PRESET_CHANGES[n], PRESET_N);
        preset_n = n;
    }
    return preset;
}

static void bank_blank(bank_t *b)
{
    memset(b, 0, sizeof *b);
    b->magic = BANK_MAGIC;
    b->format = PROJ_FORMAT;
}
static void bankx_blank(bank8_t *b)
{
    memset(b, 0, sizeof *b);
    b->magic = BANK_MAGIC;
    b->format = PROJ_FORMAT;
}

void project_defaults(void)
{
    int i;
    proj_h = 0;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    for (i = 0; i < DR_NPARAMS; i++)
        proj.par[i] = dr_param_info(i)->def;
    proj.leds = 1;
    proj.mtime = MTIME_DEF;
    proj.seen[1] = SEEN1_MOVED;
    bank_blank(&bank_bc[0]);
    bank_blank(&bank_bc[1]);
    for (i = 0; i < 3; i++)
        bankx_blank(&bank_x[i]);
    p_moved = 0;
    proj.bank = BANK_PRESETS;                  /* a new FM-1: the keys play the Presets (P9-P16 in their second bank) */
}

int scene_has(int bank, int n)                 /* scene n of a bank holds a sound (builds nothing) */
{
    if (n < 0 || n >= NSCENES)
        return 0;
    switch (bank) {
    case 0: return proj.scene_used >> n & 1u;
    case 1: case 2: return bank_bc[bank - 1].used >> n & 1u;
    case BANK_D: case BANK_D + 1: case BANK_D + 2: return n < NPLAY && (bank_x[BX(bank)].used >> n & 1u);
    case BANK_PRESETS: return n < NPLAY;   /* P1-P8 on keys */
    case BANK_PRESETS2: return n < NPRESETS - NPLAY;   /* P9 .. on keys 1 .. */
    default: return 0;
    }
}

const int16_t *scene_get(int bank, int n)      /* a Preset's stays as given until another Preset is asked for */
{
    if (n < 0 || n >= NSCENES)
        return 0;
    switch (bank) {
    case 0: return proj.scene_used >> n & 1u ? proj.scene[n] : 0;
    case 1: case 2: return bank_bc[bank - 1].used >> n & 1u ? bank_bc[bank - 1].scene[n] : 0;
    case BANK_D: case BANK_D + 1: case BANK_D + 2:
        return n < NPLAY && (bank_x[BX(bank)].used >> n & 1u) ? bank_x[BX(bank)].scene[n] : 0;
    case BANK_PRESETS:                         /* the firmware's, fixed (2026-10-19) */
        return n < NPRESETS ? preset_get(n) : 0;
    case BANK_PRESETS2:
        return n < NPRESETS - NPLAY ? preset_get(NPLAY + n) : 0;
    default: return 0;
    }
}

int scene_put(int bank, int n)
{
    int16_t *d;
    int k;
    if (n < 0 || n >= NSCENES || bank < 0 || bank >= NBANKS || BANK_IS_PRESETS(bank) ||   /* the Presets are fixed */
        (BANK_X(bank) && n >= NPLAY))
        return -1;
    if (BANK_X(bank)) {
        d = bank_x[BX(bank)].scene[n];
        bank_x[BX(bank)].used |= (uint16_t)(1u << n);
        bank_dirty |= (uint8_t)(1u << (bank - BANK_D + 4));
    } else if (bank == 0) {
        d = proj.scene[n];
        proj.scene_used |= (uint16_t)(1u << n);
    } else {
        d = bank_bc[bank - 1].scene[n];
        bank_bc[bank - 1].used |= (uint16_t)(1u << n);
        bank_dirty |= (uint8_t)(1u << bank);
    }
    for (k = 0; k < DR_NPARAMS; k++)
        d[k] = proj.par[k];
    return 0;
}

/* format 43 to 44: an LFO depth on X-WAVE (sq: squared) or the resonance (linear), whose swing was cut off at the
 * setting's ends (b the knob, 0..1), as the depth that reaches the same range now (drone.c: lfo_fit, half the scale) */
static int16_t fit_depth(const int16_t *v, int g, int tap, int dp, float b, int sq)
{
    /* Movement at 0 reached nothing: converted as at 50, so raising it later reaches what it would have (#84's review) */
    const float mv = v[P_MOVE] > 0 ? (float)v[P_MOVE] * 0.02f : 1.0f, n = v[DR_GP(g, tap)] == DR_LFO_BOTH ? 2.0f : 1.0f;
    const int d = v[DR_GP(g, dp)];
    const float a = (float)(d < 0 ? -d : d) * 0.01f, s = (sq ? a * a : a) * mv * n;
    float w, x;
    if (d == 0 || v[DR_GP(g, tap)] == DR_LFO_OFF)
        return (int16_t)d;
    w = 0.5f * (fm_minf(b + s, 1.0f) - fm_maxf(b - s, 0.0f));   /* the half-width it reached */
    x = w / (0.5f * mv * n);
    if (sq)
        x = fm_sqrtf(x);
    x = fm_minf(x * 100.0f + 0.5f, 100.0f);
    return (int16_t)(d < 0 ? -(int)x : (int)x);
}

/* one scene (or the sound) from an older format to this one */
static void convert(int16_t *v, uint32_t from)
{
    int g;
    if (from <= 19u)                            /* 0.1: the noise Colour was 0 / 1, now 0..100 (white .. pink) */
        for (g = 0; g < DR_NGROUPS; g++)
            v[DR_GP(g, GP_NCOLOR)] = v[DR_GP(g, GP_NCOLOR)] ? 100 : 0;
    if (from <= 23u)                            /* the gestures were Beat, Heat, Converge, Wash: theirs to defaults */
        for (g = P_GD_0; g <= P_GL_3; g++)
            v[g] = dr_param_info(g)->def;
    if (from <= 26u) {                          /* the set macros (Bloom, Weight, Tide, Tear) gone: their values 0 (now
                                                 * Wander, Age, Shift's move: 0 is their start); Shift's move was E5's depth */
        v[P_SHIFT_MOVE] = (unsigned)v[P_GD_1] < 4u ? v[P_GD_1] : 0;   /* 0..3 then, as in 27 */
        for (g = P_WANDER; g <= P_WSCALE; g++)
            if (g != P_SHIFT_MOVE)
                v[g] = 0;
        v[P_GD_1] = dr_param_info(P_GD_1)->def;
    }
    if (from <= 27u)                            /* NOISE's top was 6 dB lower: the same level is half the value now */
        for (g = 0; g < DR_NGROUPS; g++)
            v[DR_GP(g, GP_NOISE)] = (int16_t)((v[DR_GP(g, GP_NOISE)] + 1) / 2);
    if (from <= 29u) {                          /* Tuning into each drone's Interval: tempered, then just */
        static const int8_t EQ[10] = {0, 1, 3, 5, 6, 8, 10, 12, 14, 15}, JU[10] = {0, 2, 4, 5, 7, 9, 11, 13, 14, 16};
        for (g = 0; g < DR_NGROUPS; g++) {
            int iv = v[DR_GP(g, GP_INTERVAL)];
            iv = (unsigned)iv < 10u ? iv : 3;
            v[DR_GP(g, GP_INTERVAL)] = v[P_II_OUT] ? JU[iv] : EQ[iv];   /* the slot was Tuning then */
        }
        v[P_II_OUT] = 0;
    }
    if (from <= 28u)                            /* the keys fixed: a latch may have been another gesture's */
        for (g = P_GL_0; g <= P_GL_3; g++)
            v[g] = 0;
    if (from <= 27u) {                          /* Shift's move was one of four: now in cents, 700 none */
        static const int16_t M[4] = {1400, 1200, 200, 0};   /* a 5th up, a 4th up, a 4th down, a 5th down */
        v[P_SHIFT_MOVE] = M[(unsigned)v[P_SHIFT_MOVE] < 4u ? v[P_SHIFT_MOVE] : 0];
    }
    if (from <= 30u)                            /* Level an audio taper (squared): the same loudness is 10 sqrt(v) */
        for (g = 0; g < DR_NGROUPS; g++) {
            int lv = v[DR_GP(g, GP_LEVEL)], x = 0;
            while (x < 100 && (2 * x + 1) * (2 * x + 1) < 400 * lv)   /* rounded */
                x++;
            v[DR_GP(g, GP_LEVEL)] = (int16_t)x;
        }
    if (from >= 23u && from <= 32u) {           /* the delay types reordered, plain to most alive (Evan, 2026-10-25):
                                                 * Tape, Wide, Ping-pong, Clean, ... then; Clean, Wide, Ping-pong, Tape now
                                                 * (22 and before had no type: the default, Tape, already) */
        int t = v[P_DLY_TYPE];
        v[P_DLY_TYPE] = (int16_t)(t == 0 ? 3 : t == 3 ? 0 : t);
    }
    if (from == 24u)                            /* the swells shorter (2-4 s, Evan 2026-10-18): to the new defaults */
        for (g = P_GR_0; g <= P_GR_3; g++)
            v[g] = dr_param_info(g)->def;
    if (from <= 37u)                            /* the Swell's Shape turned round (Evan, 2026-10-30): the ramp to the
                                                 * right now, the gate to the left; the same sound */
        v[P_PULSE_SHAPE] = (int16_t)-v[P_PULSE_SHAPE];
    if (from == 39u) {                          /* the Tunings reordered, simplest to richest (Evan, 2026-10-30): 39's
                                                 * LMY, Harmonics, Shruti, Harrison now Harmonics, LMY, Harrison, Shruti */
        static const int8_t T39[DR_NTUNING] = {DR_TUN_12, DR_TUN_24, DR_TUN_19, DR_TUN_31, DR_TUN_53, DR_TUN_JUST,
                                               DR_TUN_LMY, DR_TUN_HARM, DR_TUN_SHRUTI, DR_TUN_HARRISON, DR_TUN_PARTCH};
        int t = v[P_TUNING];
        v[P_TUNING] = (int16_t)((unsigned)t < DR_NTUNING ? T39[t] : DR_TUN_12);
    }
    if (from <= 40u) {                          /* Tune in Hz (Evan, 2026-11-01): cents then, tenths of a Hz from 440
                                                 * now, the nearest */
        const float d = 4400.0f * fm_exp2f((float)v[P_TUNE] * (1.0f / 1200.0f)) - 4400.0f;
        v[P_TUNE] = (int16_t)fm_clampf(d + (d < 0.0f ? -0.5f : 0.5f), -130.0f, 130.0f);
        {   /* the time knobs a power curve now (Evan, 2026-11-07): the same time, the nearest step */
            static const uint8_t TP[3] = {P_FADE, P_MORPH, P_SCENE_T};
            static const float MN[3] = {0.05f, 0.02f, 0.1f}, K[3] = {0.102288f, 0.105507f, 0.115507f},
                               E[3] = {1.0f / 3.0f, 0.4f, 0.25f};
            int j;
            for (j = 0; j < 3; j++) {
                const float t = MN[j] * fm_exp2f((float)v[TP[j]] * K[j]), lo = dr_time_s(TP[j], 0),
                            hi = dr_time_s(TP[j], 100), y = (t - lo) / (hi - lo);
                v[TP[j]] = (int16_t)fm_clampf(y > 0.0f ? 100.0f * fm_exp2f(fm_log2f(y) * E[j]) + 0.5f : 0.0f, 0.0f, 100.0f);
            }
        }
    }
    if (from <= 42u) {                          /* the LFO rate to 100 s, every second on the dial (Evan, 2026-11-07):
                                                 * 41 and before 0.02 * 3500^(v/100) Hz, 42 (a test build) 0.01 *
                                                 * 7000^((v/100)^1.3), now 0..152 (lfo_hz_f in drone.c); the same rate,
                                                 * the nearest step */
        const float x = (float)v[P_LFO_RATE] * 0.01f,
                    hz = from <= 41u ? 0.02f * fm_exp2f((float)v[P_LFO_RATE] * 0.117733931f)
                                     : 0.01f * fm_exp2f(x > 0.0f ? 12.773139f * fm_exp2f(1.3f * fm_log2f(x)) : 0.0f),
                    t = 1.0f / hz,
                    n = t >= 10.0f ? 100.0f - t : 90.0f + fm_log2f(hz * 10.0f) * (1.0f / 0.152438f);
        v[P_LFO_RATE] = (int16_t)fm_clampf(n + 0.5f, 0.0f, 152.0f);
    }
    if (from <= 43u)                            /* the LFO on X-WAVE and the resonance turns at their ends now, and
                                                 * its depth 100 is their whole range (Evan, 2026-11-08): each sound
                                                 * keeps the range it reached */
        for (g = 0; g < DR_NGROUPS; g++) {
            if (v[DR_GP(g, GP_OLFO_DEST)] == DR_TO_XWAVE)
                v[DR_GP(g, GP_OLFO_DEPTH)] = fit_depth(v, g, GP_OLFO, GP_OLFO_DEPTH, v[DR_GP(g, GP_WAVE)] * 0.01f, 1);
            if (v[DR_GP(g, GP_FLFO_DEST)] == DR_TO_RESO)
                v[DR_GP(g, GP_FLFO_DEPTH)] = fit_depth(v, g, GP_FLFO, GP_FLFO_DEPTH, v[DR_GP(g, GP_RESO)] * 0.001f, 0);
        }
}

static int in_range(const int16_t *v)
{
    int k;
    for (k = 0; k < DR_NPARAMS; k++) {
        const dr_param_t *p = dr_param_info(k);
        if (v[k] < p->lo || v[k] > p->hi)
            return 0;
    }
    return 1;
}

/* the older layouts: 19 and 20 had 16 scenes in the project (scenes 13-16 become B1-B4), 21 had bank A there and
 * B, C in their own objects, each with 78 values a sound (DR_NPARAMS before the macros); 22 as 21 with 94 (before
 * the effects' four); 23-31 as now with 98 (before the delay's drive), 32 with 99 (before the reverb's route), 33 with 100
 * (before the Swell's division), 34 with 101 (before the LFO's, the delay's and the scenes' clock values), 35 with 104
 * (before the pressure keys' gestures' latches), 36 with 108 (before the sub waves), 37 and 38 with 110 (before the
 * Tuning), 39 and 40 with 111 (before Sync). A value added since takes its default. */
#define NP21 78
#define NP22 94
#define NP31 98
#define NP32 99
#define NP33 100
#define NP34 101
#define NP35 104
#define NP36 108
#define NP38 110                               /* 37 and 38 */
#define NP40 111                               /* 39 and 40 */
_Static_assert(DR_NPARAMS >= NP36, "the values only grow");
typedef struct {
    uint32_t magic, format;
    int16_t par[NP21];
    uint8_t leds;
    uint8_t rsv[15];
    uint16_t scene_used;
    int16_t scene[16][NP21];
} proj_v20_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP21];
    uint8_t leds, bank;
    uint8_t rsv[14];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP21];
} proj_v21_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP21];
} bank_v21_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP22];
    uint8_t leds, bank, mlive;
    uint8_t rsv[13];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP22];
} proj_v22_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP22];
} bank_v22_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP31];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t rsv[9];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP31];
} proj_v31_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP31];
} bank_v31_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP32];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t rsv[9];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP32];
} proj_v32_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP32];
} bank_v32_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP33];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t rsv[9];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP33];
} proj_v33_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP33];
} bank_v33_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP34];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t rsv[9];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP34];
} proj_v34_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP34];
} bank_v34_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP35];
    uint8_t leds, bank, mtime;
    uint8_t seen[2];
    uint8_t rsv[7];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP35];
} proj_v35_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP35];
} bank_v35_t;
typedef struct {
    uint32_t magic, format;
    int16_t par[NP36];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t seen[2];
    uint8_t rsv[7];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP36];
} proj_v36_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP36];
} bank_v36_t;
typedef struct {                               /* 37 and 38 (as 36, 110 values a sound) */
    uint32_t magic, format;
    int16_t par[NP38];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t seen[2];
    uint8_t rsv[7];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP38];
} proj_v38_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP38];
} bank_v38_t;
typedef struct {                               /* 39 and 40 (as now, 111 values a sound) */
    uint32_t magic, format;
    int16_t par[NP40];
    uint8_t leds, bank, mtime;
    uint8_t gkey_old[4];
    uint8_t seen[2];
    int8_t tun_off[3];
    uint8_t rsv[4];
    uint16_t scene_used;
    int16_t scene[NSCENES][NP40];
} proj_v40_t;
typedef struct {
    uint32_t magic, format;
    uint16_t used, rsv;
    int16_t scene[NSCENES][NP40];
} bank_v40_t;
static union {
    project_t p;
    proj_v31_t o31;
    bank_v31_t b31;
    proj_v32_t o32;
    bank_v32_t b32;
    proj_v33_t o33;
    bank_v33_t b33;
    proj_v34_t o34;
    bank_v34_t b34;
    proj_v35_t o35;
    bank_v35_t b35;
    proj_v36_t o36;
    bank_v36_t b36;
    proj_v38_t o38;
    bank_v38_t b38;
    proj_v40_t o40;
    bank_v40_t b40;
    proj_v20_t o20;
    proj_v21_t o21;
    bank_t b;
    bank8_t x;
    bank_v21_t b21;
    proj_v22_t o22;
    bank_v22_t b22;
    uint8_t raw[PLAT_STORE_MAX];
} ld;

static void widen(int16_t *dst, const int16_t *src, uint32_t from)   /* an old sound: the new values' defaults */
{
    int k;
    const int np = from >= 41u ? DR_NPARAMS : from >= 39u ? NP40 : from >= 37u ? NP38 : from >= 36u ? NP36 : from >= 35u ? NP35 : from >= 34u ? NP34 : from >= 33u ? NP33 : from >= 32u ? NP32 : from >= 23u ? NP31 : from >= 22u ? NP22 : NP21;
    for (k = 0; k < DR_NPARAMS; k++)
        dst[k] = k < np ? src[k] : dr_param_info(k)->def;
    if (dst[P_LFO_RATE] < 0)                   /* slower than 50 s, as it went before 2026-10-14: the slowest */
        dst[P_LFO_RATE] = 0;
    convert(dst, from);
}

static uint32_t bank_fmt[2];                   /* B's and C's format as stored (0: none) */
static void bank_load(int b)                   /* B (1) or C (2): from its object, if it's sound */
{
    bank_t *k = &bank_bc[b - 1];
    int i;
    bank_blank(k);
    bank_fmt[b - 1] = 0;
    memset(ld.raw, 0, sizeof ld.raw);
    if (plat_store_load(BANK_OBJ[b - 1], ld.raw, sizeof ld.raw) < 8 || ld.b.magic != BANK_MAGIC)
        return;
    bank_fmt[b - 1] = ld.b.format;
    if (ld.b.format == PROJ_FORMAT) {
        *k = ld.b;
    } else if (ld.b.format >= 41u && ld.b.format <= 43u) {   /* 41-43: as now, the LFO rate on an old scale (41,
                                                                * 42) or the X-WAVE and resonance depths (43) */
        *k = ld.b;
        for (i = 0; i < NSCENES; i++)
            convert(k->scene[i], ld.b.format);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 39u || ld.b.format == 40u) {   /* 39, 40: 111 values a sound (Tune in cents; 39 the
                                                               * Tunings in their old order) */
        const bank_v40_t *o = &ld.b40;
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], ld.b.format);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 37u || ld.b.format == 38u) {   /* 37, 38: 110 values a sound (37 the Shape the other
                                                               * way round) */
        const bank_v38_t *o = &ld.b38;
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], ld.b.format);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 36u) {           /* 36: 108 values a sound */
        const bank_v36_t *o = &ld.b36;
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], 36u);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 35u) {           /* 35: 104 values a sound */
        const bank_v35_t *o = &ld.b35;
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], 35u);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 34u) {           /* 34: 101 values a sound */
        const bank_v34_t *o = &ld.b34;
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], 34u);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 33u) {           /* 33: 100 values a sound */
        const bank_v33_t *o = &ld.b33;
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], 33u);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 32u) {           /* 32: 99 values a sound */
        const bank_v32_t *o = &ld.b32;         /* read in place (a copy was 2.4 KB of RAM) */
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], 32u);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format >= 23u && ld.b.format < 32u) {   /* 23-31: 98 values a sound */
        const bank_v31_t *o = &ld.b31;         /* read in place, as all of them (copies were 12 KB of RAM) */
        k->used = o->used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], o->scene[i], ld.b.format);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 22u) {
        k->used = ld.b22.used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], ld.b22.scene[i], 22u);
        bank_dirty |= (uint8_t)(1u << b);
    } else if (ld.b.format == 21u) {
        k->used = ld.b21.used;
        for (i = 0; i < NSCENES; i++)
            widen(k->scene[i], ld.b21.scene[i], 21u);
        bank_dirty |= (uint8_t)(1u << b);
    } else {
        return;
    }
    k->format = PROJ_FORMAT;
    for (i = 0; i < NSCENES; i++)
        if ((k->used >> i & 1u) && !in_range(k->scene[i]))
            k->used &= (uint16_t)~(1u << i);
}

/* D, E or F (j 0..2) from its object: new in format 43, read in 43 (converted) and 44 (a later format converts it
 * here too, as bank_load does B and C) */
_Static_assert(PROJ_FORMAT == 44, "bankx_load reads D-F only in formats 43 and 44: add their conversion (as bank_load "
                                  "does B and C) before raising PROJ_FORMAT, or everyone's D, E and F are dropped");
static void bankx_load(int j)
{
    bank8_t *k = &bank_x[j];
    int i;
    bankx_blank(k);
    memset(ld.raw, 0, sizeof ld.raw);
    if (plat_store_load(BANKX_OBJ[j], ld.raw, sizeof ld.raw) < (int)sizeof *k || ld.x.magic != BANK_MAGIC ||
        (ld.x.format != PROJ_FORMAT && ld.x.format != 43u))
        return;
    *k = ld.x;
    if (k->format != PROJ_FORMAT) {            /* 43: the X-WAVE and resonance depths */
        for (i = 0; i < NPLAY; i++)
            convert(k->scene[i], k->format);
        k->format = PROJ_FORMAT;
        bank_dirty |= (uint8_t)(1u << (j + 4));     /* (bits 4-6: D-F) */
    }
    k->used &= (uint16_t)((1u << NPLAY) - 1u);
    for (i = 0; i < NPLAY; i++)
        if ((k->used >> i & 1u) && !in_range(k->scene[i]))
            k->used &= (uint16_t)~(1u << i);
}

/* The Presets are fixed since 2026-10-19: any saved over before (OBJ_BANKP) move to the first empty slots of C,
 * then B, then A, so nothing is lost; one with no room left stays in OBJ_BANKP (the installer backs it up). Read from
 * the load buffer, scene by scene, in whatever format it was saved; the object is rewritten without the moved ones by
 * the next save, after the banks and the project that now hold them */
static int moved_a;
static const int16_t *old_scene(int n, uint32_t *from)   /* scene n of the bank in ld, and its format; 0: none */
{
    uint32_t f = ld.b.format;
    *from = f;
    if (f == PROJ_FORMAT || (f >= 41u && f <= 43u)) return ld.b.scene[n];
    if (f == 39u || f == 40u) return ld.b40.scene[n];
    if (f == 37u || f == 38u) return ld.b38.scene[n];
    if (f == 36u) return ld.b36.scene[n];
    if (f == 35u) return ld.b35.scene[n];
    if (f == 34u) return ld.b34.scene[n];
    if (f == 33u) return ld.b33.scene[n];
    if (f == 32u) return ld.b32.scene[n];
    if (f >= 23u && f < 32u) return ld.b31.scene[n];
    if (f == 22u) return ld.b22.scene[n];
    if (f == 21u) return ld.b21.scene[n];
    return 0;
}
static void presets_move(void)
{
    int n, b, i;
    uint16_t used;
    memset(ld.raw, 0, sizeof ld.raw);
    if (plat_store_load(OBJ_BANKP, ld.raw, sizeof ld.raw) < 8 || ld.b.magic != BANK_MAGIC)
        return;
    used = ld.b.used;
    for (n = 0; n < NSCENES; n++) {
        uint32_t from;
        const int16_t *src = old_scene(n, &from);
        if (!(used >> n & 1u) || !src)
            continue;
        for (b = 2; b >= 0; b--) {             /* C, B, A */
            uint16_t u = b ? bank_bc[b - 1].used : proj.scene_used;
            int16_t *d;
            for (i = 0; i < NSCENES && (u >> i & 1u); i++)
                ;
            if (i == NSCENES)
                continue;
            d = b ? bank_bc[b - 1].scene[i] : proj.scene[i];
            widen(d, src, from);
            if (in_range(d)) {
                if (b) {
                    bank_bc[b - 1].used |= (uint16_t)(1u << i);
                    bank_dirty |= (uint8_t)(1u << b);
                } else {
                    proj.scene_used |= (uint16_t)(1u << i);
                    moved_a = 1;               /* not yet on the flash (proj_h below) */
                }
            }
            p_moved |= (uint16_t)(1u << n);    /* (one out of range is dropped, as at any load) */
            bank_dirty |= 1u << 3;
            break;
        }
    }
}

/* KEYS 9-12 BECOME GESTURES (format 36, Evan 2026-10-28): a bank plays keys 1-8. On the first start, a scene of
 * the player's on keys 9-12 moves to the first empty key 1-8 of its own bank, then of another (A, B, C); one with no
 * room stays stored there, unplayed, not lost. Then 0.2's Presets that 0.3 dropped go into bank F (OLD02_CHANGES).
 * proj_note says what happened, for the screen */
int proj_note_moved, proj_note_stored, proj_note_presets;
static uint16_t *bank_used(int b) { return b ? &bank_bc[b - 1].used : &proj.scene_used; }
static int16_t *bank_scene(int b, int n) { return b ? bank_bc[b - 1].scene[n] : proj.scene[n]; }
static int free_key(int b)                     /* the first empty key 1-8 of bank b, or -1 */
{
    int i;
    for (i = 0; i < NPLAY; i++)
        if (!(*bank_used(b) >> i & 1u))
            return i;
    return -1;
}
static void keys_to_gestures(void)
{
    static const int8_t ORDER[3][3] = {{0, 1, 2}, {1, 0, 2}, {2, 1, 0}};   /* its own bank first */
    /* a first save cut short: B or C already written in this format (the banks go first), the project not. Those
     * moves and copies are on the flash already: only A's own scenes move again, within A, so nothing doubles */
    const int again = bank_fmt[0] >= 36u || bank_fmt[1] >= 36u;
    int b, n, k, i, d;
    for (b = 0; b < 3; b++)
        for (n = NPLAY; n < NSCENES; n++) {
            if (!(*bank_used(b) >> n & 1u))
                continue;
            for (k = 0; k < (again ? 1 : 3); k++) {
                d = ORDER[b][k];
                if ((i = free_key(d)) >= 0) {
                    memcpy(bank_scene(d, i), bank_scene(b, n), sizeof(int16_t) * DR_NPARAMS);
                    *bank_used(d) |= (uint16_t)(1u << i);
                    *bank_used(b) &= (uint16_t)~(1u << n);
                    proj_note_moved++;
                    break;
                }
            }
            if (k == (again ? 1 : 3))
                proj_note_stored++;
        }
    if (!bank_x[2].used) {                     /* 0.2's dropped Presets into F (a first save cut short may have
                                                * written it already: then it's loaded, not empty) */
        for (n = 0; n < NOLD02; n++)
            changes_build(bank_x[2].scene[n], OLD02_CHANGES[n], OLD02_N);
        bank_x[2].used = (uint16_t)((1u << NOLD02) - 1u);
        bank_dirty |= 1u << 6;
        proj_note_presets = NOLD02;
    }
    for (b = 1; b < 3; b++)
        bank_dirty |= (uint8_t)(1u << b);
}

int project_load(void)
{
    int n, i;
    uint32_t from;
    preset_n = -1;
    bank_dirty = 0;
    p_moved = 0;
    bank_blank(&bank_bc[0]);
    bank_blank(&bank_bc[1]);
    for (i = 0; i < 3; i++)
        bankx_blank(&bank_x[i]);
    memset(ld.raw, 0, sizeof ld.raw);
    n = plat_store_load(OBJ_PROJ, ld.raw, sizeof ld.raw);
    from = ld.p.format;
    if (n < 8 || ld.p.magic != PROJ_MAGIC || from < 19u || from > PROJ_FORMAT) {
        project_defaults();
        return -1;
    }
    if (from == PROJ_FORMAT) {
        proj = ld.p;
    } else if (from >= 41u && from <= 43u) {   /* 41-43: as now, the LFO rate on an old scale (41, 42) or the
                                                * X-WAVE and resonance depths (43) */
        proj = ld.p;
        convert(proj.par, from);
        for (i = 0; i < NSCENES; i++)
            convert(proj.scene[i], from);
    } else if (from == 39u || from == 40u) {   /* 39, 40: 111 values a sound (Tune in cents; 39 the Tunings in
                                                * their old order) */
        const proj_v40_t *o = &ld.o40;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        memcpy(proj.seen, o->seen, sizeof proj.seen);
        memcpy(proj.tun_off, o->tun_off, sizeof proj.tun_off);
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 37u || from == 38u) {   /* 37, 38: 110 values a sound (37 the Shape the other way round) */
        const proj_v38_t *o = &ld.o38;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        memcpy(proj.seen, o->seen, sizeof proj.seen);
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 36u) {                  /* 36: 108 values a sound */
        const proj_v36_t *o = &ld.o36;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        memcpy(proj.seen, o->seen, sizeof proj.seen);
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 35u) {                  /* 35: 104 values a sound */
        const proj_v35_t *o = &ld.o35;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        memcpy(proj.seen, o->seen, sizeof proj.seen);
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 34u) {                  /* 34: 101 values a sound */
        const proj_v34_t *o = &ld.o34;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 33u) {                  /* 33: 100 values a sound */
        const proj_v33_t *o = &ld.o33;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 32u) {                  /* 32: 99 values a sound */
        const proj_v32_t *o = &ld.o32;         /* read in place (a copy was 2.4 KB of RAM) */
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from >= 23u) {                  /* 23-31: 98 values a sound */
        const proj_v31_t *o = &ld.o31;   /* read in place */
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = o->mtime;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 22u) {
        const proj_v22_t *o = &ld.o22;   /* read in place */
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.mtime = MTIME_DEF;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else if (from == 21u) {
        const proj_v21_t *o = &ld.o21;   /* read in place */
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.bank = o->bank;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
    } else {                                   /* 19, 20 */
        const proj_v20_t *o = &ld.o20;   /* read in place */
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o->par, from);
        proj.leds = o->leds;
        proj.scene_used = o->scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o->scene[i], from);
        for (i = 0; i < 4; i++)                /* scenes 13-16 to B1-B4 */
            if (o->scene_used >> (12 + i) & 1u) {
                widen(bank_bc[0].scene[i], o->scene[12 + i], from);
                bank_bc[0].used |= (uint16_t)(1u << i);
            }
        bank_dirty |= 1u << 1;
    }
    if (from >= 21u) {
        bank_load(1);
        bank_load(2);
    }
    for (i = 0; i < 3; i++)                    /* D, E, F (v0.3): none before */
        bankx_load(i);
    moved_a = 0;
    presets_move();
    proj_note_moved = proj_note_stored = proj_note_presets = 0;
    if (from < 36u)                            /* 0.3: keys 9-12 are gestures now */
        keys_to_gestures();
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    if (proj.bank >= NBANKS)
        proj.bank = 0;
    if (from <= 25u || proj.mtime > 100u)    /* the byte was the macro PRESETS played */
        proj.mtime = MTIME_DEF;
    memset(proj.gkey_old, 0, sizeof proj.gkey_old);
    if (!in_range(proj.par)) {
        project_defaults();
        return -1;
    }
    for (i = 0; i < NSCENES; i++)              /* a scene out of range is dropped, not the project */
        if ((proj.scene_used >> i & 1u) && !in_range(proj.scene[i]))
            proj.scene_used &= (uint16_t)~(1u << i);
    proj_h = from == PROJ_FORMAT && !moved_a ? proj_hash() : 0u;   /* converted: not yet on the flash in this format */
    return 0;
}

int project_save(void)                         /* the banks first, then the project: a converted 0.1 project's
                                                * B1-B4 are on the flash before the project that no longer holds them;
                                                * the old Presets object last, once the scenes moved from it are saved */
{
    int rc = 0, b, n;
    uint32_t h;
    for (b = 1; b <= 2; b++)
        if (!rc && (bank_dirty >> b & 1u)) {
            rc = plat_store_save(BANK_OBJ[b - 1], &bank_bc[b - 1], sizeof bank_bc[b - 1]);
            if (!rc)
                bank_dirty &= (uint8_t)~(1u << b);
        }
    for (b = 0; b < 3; b++)                    /* D, E, F */
        if (!rc && (bank_dirty >> (b + 4) & 1u)) {
            rc = plat_store_save(BANKX_OBJ[b], &bank_x[b], sizeof bank_x[b]);
            if (!rc)
                bank_dirty &= (uint8_t)~(1u << (b + 4));
        }
    if (rc)
        return rc;
    h = proj_hash();
    if (h != proj_h) {                         /* (the flash has it already otherwise) */
        rc = plat_store_save(OBJ_PROJ, &proj, sizeof proj);
        proj_h = rc ? 0u : h;
        if (rc)
            return rc;
    }
    if (bank_dirty >> 3 & 1u) {                /* the old Presets object, without the scenes moved from it */
        memset(ld.raw, 0, sizeof ld.raw);
        n = plat_store_load(OBJ_BANKP, ld.raw, sizeof ld.raw);
        if (n >= 8 && ld.b.magic == BANK_MAGIC) {
            ld.b.used &= (uint16_t)~p_moved;   /* (the same place in every format) */
            rc = plat_store_save(OBJ_BANKP, ld.raw, (uint32_t)n);
        }
        if (!rc) {
            bank_dirty &= (uint8_t)~(1u << 3);
            p_moved = 0;
        }
    }
    return rc;
}

void project_apply(void)
{
    int i;
    for (i = 0; i < DR_NPARAMS; i++)
        dr_set(i, proj.par[i]);
}
