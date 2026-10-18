/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift project (from FoMni's): the sound and bank A in one flash object (OBJ_PROJ), banks B and C in
 * one each (OBJ_BANKB, OBJ_BANKC); the Presets bank is in the firmware. A project of another format, or none,
 * gives the defaults; 0.1's (19) and v0.2's first (20) are converted. Part of the unity build, after app.h. */
project_t proj;
static bank_t bank_bc[2];                  /* banks B, C */
static uint8_t bank_dirty;                 /* bit 1 B, bit 2 C: to be written with the next save */
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

/* the Presets bank: the starting sound, and the bass test (both drones an octave down, the filters darker,
 * resonance 100); the rest to be made by ear. Each a list of changes from the defaults, ended by -1 */
static const int16_t PRESET_CHANGES[][9] = {
    {-1},
    {DR_GP(0, GP_PITCH), 3800, DR_GP(1, GP_PITCH), 4500, DR_GP(0, GP_CUTOFF), 450, DR_GP(1, GP_CUTOFF), 430, -1},
};
#define NPRESETS ((int)(sizeof PRESET_CHANGES / sizeof PRESET_CHANGES[0]))
static int16_t preset[NPRESETS][DR_NPARAMS];

static void presets_init(void)
{
    int n, k;
    for (n = 0; n < NPRESETS; n++) {
        for (k = 0; k < DR_NPARAMS; k++)
            preset[n][k] = dr_param_info(k)->def;
        for (k = 0; PRESET_CHANGES[n][k] >= 0 && k < 8; k += 2)
            preset[n][PRESET_CHANGES[n][k]] = PRESET_CHANGES[n][k + 1];
    }
    for (n = 0; n < NPRESETS; n++) {           /* the bass test's resonance, low */
        if (n == 1) {
            preset[n][DR_GP(0, GP_RESO)] = 100;
            preset[n][DR_GP(1, GP_RESO)] = 100;
        }
    }
}

static void bank_blank(bank_t *b)
{
    memset(b, 0, sizeof *b);
    b->magic = BANK_MAGIC;
    b->format = PROJ_FORMAT;
}

static void gkeys_default(void)                  /* D5-G5: Weight, Shift, Converge, Wash */
{
    proj.gkey[0] = DR_G_WEIGHT;
    proj.gkey[1] = DR_G_SHIFT;
    proj.gkey[2] = DR_G_CONVERGE;
    proj.gkey[3] = DR_G_WASH;
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
    gkeys_default();
    bank_blank(&bank_bc[0]);
    bank_blank(&bank_bc[1]);
}

const int16_t *scene_get(int bank, int n)
{
    if (n < 0 || n >= NSCENES)
        return 0;
    switch (bank) {
    case 0: return proj.scene_used >> n & 1u ? proj.scene[n] : 0;
    case 1: case 2: return bank_bc[bank - 1].used >> n & 1u ? bank_bc[bank - 1].scene[n] : 0;
    case BANK_PRESETS: return n < NPRESETS ? preset[n] : 0;
    default: return 0;
    }
}

int scene_put(int bank, int n)
{
    int16_t *d;
    int k;
    if (n < 0 || n >= NSCENES || bank < 0 || bank > 2)
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
        for (g = P_WANDER; g <= P_OLD_M3; g++)
            if (g != P_SHIFT_MOVE)
                v[g] = 0;
        v[P_GD_1] = dr_param_info(P_GD_1)->def;
    }
    if (from <= 27u)                            /* NOISE's top was 6 dB lower: the same level is half the value now */
        for (g = 0; g < DR_NGROUPS; g++)
            v[DR_GP(g, GP_NOISE)] = (int16_t)((v[DR_GP(g, GP_NOISE)] + 1) / 2);
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

static void bank_load(int b)                   /* B (1) or C (2): from its object, if it's sound */
{
    bank_t *k = &bank_bc[b - 1];
    int i;
    bank_blank(k);
    memset(ld.raw, 0, sizeof ld.raw);
    if (plat_store_load(b == 1 ? OBJ_BANKB : OBJ_BANKC, ld.raw, sizeof ld.raw) < 8 || ld.b.magic != BANK_MAGIC)
        return;
    if (ld.b.format == PROJ_FORMAT || (ld.b.format >= 23u && ld.b.format <= 27u)) {
        uint32_t f = ld.b.format;
        *k = ld.b;
        if (f != PROJ_FORMAT) {                /* 23-27: the same layout */
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
    memset(ld.raw, 0, sizeof ld.raw);
    n = plat_store_load(OBJ_PROJ, ld.raw, sizeof ld.raw);
    from = ld.p.format;
    if (n < 8 || ld.p.magic != PROJ_MAGIC || from < 19u || from > PROJ_FORMAT) {
        project_defaults();
        return -1;
    }
    if (from == PROJ_FORMAT || (from >= 23u && from <= 27u)) {   /* 23-27: the same layout */
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
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    if (proj.bank >= NBANKS)
        proj.bank = 0;
    if (from <= 25u || proj.mtime > 100u)    /* the byte was the macro PRESETS played */
        proj.mtime = MTIME_DEF;
    for (i = 0; i < DR_NGEST; i++)
        if (from <= 26u || proj.gkey[i] >= DR_NGTYPE) {   /* these bytes were spare: the keys as they were */
            gkeys_default();
            break;
        }
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
    for (b = 1; b <= 2; b++)
        if (!rc && (bank_dirty >> b & 1u)) {
            rc = plat_store_save(b == 1 ? OBJ_BANKB : OBJ_BANKC, &bank_bc[b - 1], sizeof bank_bc[b - 1]);
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
    for (i = 0; i < DR_NGEST; i++)
        dr_gesture_type(i, proj.gkey[i]);
    for (i = 0; i < DR_NPARAMS; i++)
        dr_set(i, proj.par[i]);
}
