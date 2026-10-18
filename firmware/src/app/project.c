/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift project (from FoMni's): the sound and bank A in one flash object (OBJ_PROJ), banks B and C in
 * one each (OBJ_BANKB, OBJ_BANKC); the Presets bank is the firmware's, each saved over in OBJ_BANKP (all but
 * PRESET_LOCKED, the clean sound: Evan, 2026-10-18). A project of another format, or none,
 * gives the defaults; 0.1's (19) and v0.2's first (20) are converted. Part of the unity build, after app.h. */
project_t proj;
static bank_t bank_bc[3];                  /* banks B, C, and the Presets saved over (used: which are) */
static uint8_t bank_dirty;                 /* bit 1 B, bit 2 C, bit 3 the Presets: to be written with the next save */
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

/* the Presets bank (Evan, 2026-10-18): P1 the starting sound, then sounds each showing a part of the range, P12 the
 * clean one (one oscillator, everything off; locked). Each a list of changes from the defaults, ended by -1; any
 * of them but P12 can be saved over (OBJ_BANKP) */
#define I_(p) DR_GP(0, p)
#define II_(p) DR_GP(1, p)
#define PRESET_N 80
static const int16_t PRESET_CHANGES[][PRESET_N + 1] = {
    /* P1 NIAGARA: the starting sound */
    {-1},
    /* P2 ABYSS (dark): low, a minor third apart, a saw and a narrow square, the subs a subharmonic (a third of
     * the pitch) down, closed resonant filters, character and a little feedback, a dark hall.
     * The LFOs on the resonance, very slowly: it swells and rings */
    {I_(GP_PITCH), 3600, II_(GP_PITCH), 3900, I_(GP_WAVE), 67, II_(GP_WAVE), 100, II_(GP_WIDTH), 70,
     I_(GP_INTERVAL), 0, II_(GP_DETUNE), 30, I_(GP_MIX), 30, I_(GP_SUBOCT), 3, I_(GP_CUTOFF), 650, II_(GP_CUTOFF), 610,
     I_(GP_RESO), 450, II_(GP_RESO), 450, I_(GP_DRIVE), 70, II_(GP_DRIVE), 60, P_CHAR, 40, P_FEEDBACK, 30,
     P_LFO_RATE, 3, P_REV_TYPE, 1, P_REV_LEN, 75, P_REV_DAMP, 80, P_REV_MIX, 40, I_(GP_NOISE), 8, I_(GP_LEVEL), 100,
     II_(GP_LEVEL), 100, P_DLY_TYPE, 1, P_DLY_TIME, 85, P_DLY_REPEATS, 45, P_DLY_MIX, 15, I_(GP_FLFO_DEST), DR_TO_RESO,
     I_(GP_FLFO_DEPTH), 55, II_(GP_FLFO_DEST), DR_TO_RESO, II_(GP_FLFO_DEPTH), -55, -1},
    /* P3 EERIE: a tritone apart, near-sines, drone II through a ringing band-pass with a breath of white noise,
     * its partner a just minor third; drone II wanders a whole-tone scale; the LFO's doubled curve, more Movement; the longest clean echoes, a chorus hall.
     * Drone II's pitch wavers a little (the oscillator LFO), the LFO's own wave bending its rate: uneven */
    {I_(GP_PITCH), 4500, II_(GP_PITCH), 5100, I_(GP_WAVE), 20, II_(GP_WAVE), 33, I_(GP_INTERVAL), 1,
     II_(GP_INTERVAL), 7, I_(GP_DETUNE), 35, II_(GP_DETUNE), 8, II_(GP_MODE), DR_BP, II_(GP_CUTOFF), 640,
     II_(GP_RESO), 720, I_(GP_CUTOFF), 600, II_(GP_NOISE), 12, II_(GP_NCOLOR), 0, P_WANDER, 25, P_WSCALE, 3,
     P_GLIDE, 45, P_LFO_W1, 6, P_MOVE, 70, P_PULSE, 0, P_REV_TYPE, 3, P_REV_MIX, 55, P_DLY_TYPE, 3, P_DLY_TIME, 100,
     P_DLY_REPEATS, 70, P_DLY_MIX, 30, II_(GP_OLFO), DR_LFO_II, II_(GP_OLFO_DEPTH), 12, P_LFO_RMOD, 40,
     P_LFO_RMOD_SRC, DR_LFO_I, -1},
    /* P4 GLASS: high sines an octave apart, beating a little, a long bright cathedral */
    {I_(GP_PITCH), 7200, II_(GP_PITCH), 7900, I_(GP_WAVE), 0, II_(GP_WAVE), 0, I_(GP_INTERVAL), 14, I_(GP_DETUNE), 20,
     II_(GP_DETUNE), 25, I_(GP_CUTOFF), 900, II_(GP_CUTOFF), 900, I_(GP_RESO), 100, II_(GP_RESO), 100, I_(GP_MIX), 0,
     II_(GP_MIX), 0, P_PULSE, 0, P_REV_LEN, 85, P_REV_MIX, 60, P_REV_DAMP, 15, P_DLY_TIME, 70, P_DLY_MIX, 30,
     P_DLY_REPEATS, 60, I_(GP_LEVEL), 33, II_(GP_LEVEL), 29, -1},
    /* P5 HEAVEN: a major tenth apart (G3 and B4), sines, their partners a just third and a just fifth, gently
     * beating, open and bright, a long light cathedral, the delay high and soft; no Swell.
     * The oscillator LFOs on X-Wave: the sines bloom slowly toward triangles and back */
    {I_(GP_PITCH), 5500, II_(GP_PITCH), 7100, I_(GP_WAVE), 0, II_(GP_WAVE), 10, I_(GP_INTERVAL), 9,
     II_(GP_INTERVAL), 13, I_(GP_DETUNE), 15, II_(GP_DETUNE), 15, I_(GP_MIX), 0, II_(GP_MIX), 0, I_(GP_CUTOFF), 950,
     II_(GP_CUTOFF), 950, I_(GP_RESO), 50, II_(GP_RESO), 50, P_PULSE, 0, P_MOVE, 35, P_REV_TYPE, 2, P_REV_LEN, 92,
     P_REV_DAMP, 20, P_REV_MIX, 65, P_DLY_TIME, 80, P_DLY_MIX, 25, P_DLY_REPEATS, 60, I_(GP_LEVEL), 38,
     II_(GP_LEVEL), 33, P_DLY_TYPE, 1, I_(GP_OLFO_DEST), DR_TO_XWAVE, I_(GP_OLFO_DEPTH), 65, II_(GP_OLFO), DR_LFO_II,
     II_(GP_OLFO_DEST), DR_TO_XWAVE, II_(GP_OLFO_DEPTH), 65, -1},
    /* P6 WANDERER: a minor seventh apart, warm saws with some resonance; drone II wanders Dorian over drone I,
     * gliding; no Swell.
     * A gentler sweep on a rounder wave */
    {I_(GP_PITCH), 5000, II_(GP_PITCH), 6000, I_(GP_WAVE), 55, II_(GP_WAVE), 55, I_(GP_CUTOFF), 620,
     II_(GP_CUTOFF), 600, I_(GP_RESO), 400, II_(GP_RESO), 400, P_WANDER, 45, P_WSCALE, 4, P_GLIDE, 35, P_PULSE, 0,
     P_DLY_MIX, 35, P_DLY_REPEATS, 55, I_(GP_LEVEL), 76, II_(GP_LEVEL), 67, P_DLY_TYPE, 1, P_DLY_TIME, 60,
     P_REV_TYPE, 1, P_REV_LEN, 50, P_REV_MIX, 30, P_LFO_W1, 4, P_LFO_RATE, 12, I_(GP_FLFO_DEPTH), 60,
     II_(GP_FLFO_DEPTH), -60, -1},
    /* P7 TAPE: worn: a minor third apart, a wide square and a saw, Age and a lot of Character, a tape delay,
     * hiss, darker, the sub a fifth's subharmonic.
     * Tape wow: the oscillator LFO on both pitches, slow and small (about 2 s, +-12 cents) */
    {I_(GP_PITCH), 4100, II_(GP_PITCH), 4400, I_(GP_WAVE), 100, I_(GP_WIDTH), 35, II_(GP_WAVE), 67, I_(GP_MIX), 30,
     I_(GP_SUBOCT), 4, P_AGE, 80, P_CHAR, 70, P_DLY_TYPE, 0, P_DLY_MIX, 35, P_DLY_REPEATS, 55, I_(GP_CUTOFF), 720,
     II_(GP_CUTOFF), 700, I_(GP_NOISE), 18, II_(GP_NOISE), 12, I_(GP_LEVEL), 100, II_(GP_LEVEL), 100, P_REV_TYPE, 0,
     P_REV_LEN, 35, P_REV_MIX, 25, I_(GP_OLFO_DEPTH), 7, II_(GP_OLFO), DR_LFO_I, II_(GP_OLFO_DEPTH), 6, P_LFO_RATE, 40,
     I_(GP_FLFO_DEPTH), 40, II_(GP_FLFO_DEPTH), -40, -1},
    /* P8 PULSE: a whole tone apart (a cluster), saws, the Swell in time, deep and swung, drone II high-passed and
     * resonant, ping-pong echoes, a hall.
     * The LFO locked to the Swell's bars, quicker */
    {II_(GP_PITCH), 5200, I_(GP_PITCH), 5000, II_(GP_MODE), DR_HP, II_(GP_CUTOFF), 560, II_(GP_RESO), 600,
     P_PULSE_RATE, 40, P_PULSE_DEPTH, 70, P_PULSE_SHAPE, 60, P_PULSE_SWING, 20, P_DLY_TYPE, 2, P_DLY_TIME, 45,
     P_DLY_MIX, 30, P_REV_TYPE, 1, P_REV_LEN, 45, P_REV_MIX, 25, P_LFO_LOCK, 1, P_LFO_RATE, 35, I_(GP_FLFO_DEPTH), 70,
     -1},
    /* P9 HOWL: low, a semitone apart (they grind), the feedback high, resonant, drone II band-passed, driven, dark;
     * a short tape slap in a small room.
     * The LFO in random steps on drone I's resonance: the howl flares unpredictably */
    {I_(GP_PITCH), 3800, II_(GP_PITCH), 3900, P_FEEDBACK, 85, P_CHAR, 30, II_(GP_MODE), DR_BP, I_(GP_CUTOFF), 740,
     II_(GP_CUTOFF), 780, I_(GP_RESO), 600, II_(GP_RESO), 600, I_(GP_DRIVE), 75, II_(GP_DRIVE), 75, P_DLY_TIME, 20,
     P_DLY_REPEATS, 45, P_DLY_MIX, 40, P_REV_TYPE, 0, P_REV_LEN, 40, P_REV_MIX, 50, P_MOVE, 40, I_(GP_LEVEL), 100,
     II_(GP_LEVEL), 100, P_LFO_W1, DR_LFO_RANDOM, P_LFO_RATE, 20, I_(GP_FLFO_DEST), DR_TO_RESO, I_(GP_FLFO_DEPTH), 50,
     -1},
    /* P10 CATHEDRAL: a major sixth apart, soft waves, drone I's sub a fifth's subharmonic far below, a long dark
     * reverb, almost a freeze.
     * Almost no LFO: still and huge */
    {I_(GP_PITCH), 4800, II_(GP_PITCH), 5700, I_(GP_WAVE), 20, II_(GP_WAVE), 20, I_(GP_MIX), 35, I_(GP_SUBOCT), 4,
     I_(GP_CUTOFF), 650, II_(GP_CUTOFF), 650, P_PULSE, 0, P_REV_TYPE, 2, P_REV_LEN, 98, P_REV_MIX, 75, P_REV_DAMP, 55,
     P_DLY_MIX, 10, I_(GP_LEVEL), 43, II_(GP_LEVEL), 38, I_(GP_FLFO_DEPTH), 15, II_(GP_FLFO_DEPTH), -15, P_MOVE, 25, -1},
    /* P11 METAL: cross-mod and hard sync, an inharmonic gap (a tritone and a bit), squares, drone I high-passed,
     * resonant, in a small room.
     * The oscillator LFOs on X-Wave and a quicker filter sweep: a moving, metallic shimmer */
    {I_(GP_PITCH), 4300, II_(GP_PITCH), 6230, P_XMOD, 1, P_XMOD_AMT, 60, P_SYNC, 1, I_(GP_WAVE), 100,
     II_(GP_WAVE), 100, I_(GP_WIDTH), 50, I_(GP_MODE), DR_HP, I_(GP_CUTOFF), 520, II_(GP_CUTOFF), 750,
     I_(GP_RESO), 650, II_(GP_RESO), 400, I_(GP_DRIVE), 55, II_(GP_DRIVE), 55, P_REV_TYPE, 0, P_REV_MIX, 25,
     I_(GP_LEVEL), 100, II_(GP_LEVEL), 100, P_DLY_TYPE, 3, P_DLY_TIME, 10, P_DLY_REPEATS, 30, P_DLY_MIX, 25,
     I_(GP_OLFO_DEST), DR_TO_XWAVE, I_(GP_OLFO_DEPTH), 45, II_(GP_OLFO), DR_LFO_II, II_(GP_OLFO_DEST), DR_TO_XWAVE,
     II_(GP_OLFO_DEPTH), 45, P_LFO_RATE, 45, I_(GP_FLFO_DEPTH), 50, II_(GP_FLFO_DEPTH), -50, -1},
    /* P12 CLEAN (locked): one oscillator, drone I's saw alone through an open filter, everything else off; drone II
     * out on its own (SEL + PLAY brings it in, as it is) */
    {P_II_OUT, 1, I_(GP_PARTNER), 0, II_(GP_PARTNER), 0, I_(GP_MIX), 0, I_(GP_CUTOFF), 1000, I_(GP_RESO), 0,
     I_(GP_DRIVE), 0, I_(GP_OLFO_DEPTH), 0, I_(GP_FLFO_DEPTH), 0, II_(GP_FLFO_DEPTH), 0, P_DRIFT, 0, P_PULSE, 0,
     P_DLY_MIX, 0, P_REV_MIX, 0, P_DLY_LFO_DEPTH, 0, P_MOVE, 0, I_(GP_LEVEL), 67, -1},
};
#define NPRESETS ((int)(sizeof PRESET_CHANGES / sizeof PRESET_CHANGES[0]))
_Static_assert(NPRESETS == NSCENES && PRESET_LOCKED < NPRESETS, "a Preset for every key, the clean one locked");
static int16_t preset[NPRESETS][DR_NPARAMS];

static void presets_init(void)
{
    int n, k;
    for (n = 0; n < NPRESETS; n++) {
        for (k = 0; k < DR_NPARAMS; k++)
            preset[n][k] = dr_param_info(k)->def;
        for (k = 0; k < PRESET_N && PRESET_CHANGES[n][k] >= 0; k += 2)
            preset[n][PRESET_CHANGES[n][k]] = PRESET_CHANGES[n][k + 1];
    }
}

static void bank_blank(bank_t *b)
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
    bank_blank(&bank_bc[0]);
    bank_blank(&bank_bc[1]);
    bank_blank(&bank_bc[2]);
    proj.bank = BANK_PRESETS;                  /* a new FM-1: the keys play the Presets */
}

const int16_t *scene_get(int bank, int n)
{
    if (n < 0 || n >= NSCENES)
        return 0;
    switch (bank) {
    case 0: return proj.scene_used >> n & 1u ? proj.scene[n] : 0;
    case 1: case 2: return bank_bc[bank - 1].used >> n & 1u ? bank_bc[bank - 1].scene[n] : 0;
    case BANK_PRESETS:                         /* saved over, or the firmware's */
        if (n != PRESET_LOCKED && (bank_bc[2].used >> n & 1u))
            return bank_bc[2].scene[n];
        return n < NPRESETS ? preset[n] : 0;
    default: return 0;
    }
}

int scene_put(int bank, int n)
{
    int16_t *d;
    int k;
    if (n < 0 || n >= NSCENES || bank < 0 || bank > BANK_PRESETS || (bank == BANK_PRESETS && n == PRESET_LOCKED))
        return -1;
    if (bank == 0) {
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
    if (from == 24u)                            /* the swells shorter (2-4 s, Evan 2026-10-18): to the new defaults */
        for (g = P_GR_0; g <= P_GR_3; g++)
            v[g] = dr_param_info(g)->def;
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
 * the effects' four). A value added since takes its default. */
#define NP21 78
#define NP22 94
_Static_assert(DR_NPARAMS >= NP22, "the values only grow");
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
static union {
    project_t p;
    proj_v20_t o20;
    proj_v21_t o21;
    bank_t b;
    bank_v21_t b21;
    proj_v22_t o22;
    bank_v22_t b22;
    uint8_t raw[PLAT_STORE_MAX];
} ld;

static void widen(int16_t *dst, const int16_t *src, uint32_t from)   /* an old sound: the new values' defaults */
{
    int k;
    const int np = from >= 22u ? NP22 : NP21;
    for (k = 0; k < DR_NPARAMS; k++)
        dst[k] = k < np ? src[k] : dr_param_info(k)->def;
    if (dst[P_LFO_RATE] < 0)                   /* slower than 50 s, as it went before 2026-10-14: the slowest */
        dst[P_LFO_RATE] = 0;
    convert(dst, from);
}

static void bank_load(int b)                   /* B (1), C (2) or the Presets (3): from its object, if it's sound */
{
    bank_t *k = &bank_bc[b - 1];
    int i;
    bank_blank(k);
    memset(ld.raw, 0, sizeof ld.raw);
    if (plat_store_load(BANK_OBJ[b - 1], ld.raw, sizeof ld.raw) < 8 || ld.b.magic != BANK_MAGIC)
        return;
    if (ld.b.format == PROJ_FORMAT || (ld.b.format >= 23u && ld.b.format <= 29u)) {
        uint32_t f = ld.b.format;
        *k = ld.b;
        if (f != PROJ_FORMAT) {                /* 23-29: the same layout */
            for (i = 0; i < NSCENES; i++)
                convert(k->scene[i], f);
            bank_dirty |= (uint8_t)(1u << b);
        }
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

int project_load(void)
{
    int n, i;
    uint32_t from;
    presets_init();
    bank_dirty = 0;
    bank_blank(&bank_bc[0]);
    bank_blank(&bank_bc[1]);
    bank_blank(&bank_bc[2]);
    memset(ld.raw, 0, sizeof ld.raw);
    n = plat_store_load(OBJ_PROJ, ld.raw, sizeof ld.raw);
    from = ld.p.format;
    if (n < 8 || ld.p.magic != PROJ_MAGIC || from < 19u || from > PROJ_FORMAT) {
        project_defaults();
        return -1;
    }
    if (from == PROJ_FORMAT || (from >= 23u && from <= 29u)) {   /* 23-29: the same layout */
        proj = ld.p;
        if (from != PROJ_FORMAT) {
            convert(proj.par, from);
            for (i = 0; i < NSCENES; i++)
                convert(proj.scene[i], from);
        }
    } else if (from == 22u) {
        static proj_v22_t o;
        o = ld.o22;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o.par, from);
        proj.leds = o.leds;
        proj.bank = o.bank;
        proj.mtime = MTIME_DEF;
        proj.scene_used = o.scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o.scene[i], from);
    } else if (from == 21u) {
        static proj_v21_t o;
        o = ld.o21;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o.par, from);
        proj.leds = o.leds;
        proj.bank = o.bank;
        proj.scene_used = o.scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o.scene[i], from);
    } else {                                   /* 19, 20 */
        static proj_v20_t o;
        o = ld.o20;
        memset(&proj, 0, sizeof proj);
        widen(proj.par, o.par, from);
        proj.leds = o.leds;
        proj.scene_used = o.scene_used & ((1u << NSCENES) - 1u);
        for (i = 0; i < NSCENES; i++)
            widen(proj.scene[i], o.scene[i], from);
        for (i = 0; i < 4; i++)                /* scenes 13-16 to B1-B4 */
            if (o.scene_used >> (12 + i) & 1u) {
                widen(bank_bc[0].scene[i], o.scene[12 + i], from);
                bank_bc[0].used |= (uint16_t)(1u << i);
            }
        bank_dirty |= 1u << 1;
    }
    if (from >= 21u) {
        bank_load(1);
        bank_load(2);
    }
    bank_load(3);
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
    proj_h = from == PROJ_FORMAT ? proj_hash() : 0u;   /* converted: not yet on the flash in this format */
    return 0;
}

int project_save(void)                         /* the banks first: a converted 0.1 project's B1-B4 are on
                                                * the flash before the project that no longer holds them */
{
    int rc = 0, b;
    uint32_t h;
    for (b = 1; b <= 3; b++)
        if (!rc && (bank_dirty >> b & 1u)) {
            rc = plat_store_save(BANK_OBJ[b - 1], &bank_bc[b - 1], sizeof bank_bc[b - 1]);
            if (!rc)
                bank_dirty &= (uint8_t)~(1u << b);
        }
    if (rc)
        return rc;
    h = proj_hash();
    if (h == proj_h)
        return 0;                              /* the flash has it already */
    rc = plat_store_save(OBJ_PROJ, &proj, sizeof proj);
    proj_h = rc ? 0u : h;
    return rc;
}

void project_apply(void)
{
    int i;
    for (i = 0; i < DR_NPARAMS; i++)
        dr_set(i, proj.par[i]);
}
