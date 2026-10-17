/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift project (from FoMni's): the sound and bank A in one flash object (OBJ_PROJ), banks B and C in
 * one each (OBJ_BANKB, OBJ_BANKC); the Presets bank is in the firmware. A project of another format, or none,
 * gives the defaults; 0.1's (19) and v0.2's first (20) are converted. Part of the unity build, after app.h. */
project_t proj;
static bank_t bank_bc[2];                  /* banks B, C */
static uint8_t bank_dirty;                 /* bit 1 B, bit 2 C: to be written with the next save */

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

void project_defaults(void)
{
    int i;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    for (i = 0; i < DR_NPARAMS; i++)
        proj.par[i] = dr_param_info(i)->def;
    proj.leds = 1;
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

/* 19 and 20 had 16 scenes in the project, at the same place: the first 12 are bank A as they lie, 13-16
 * become B1-B4 (moved there by project_load, which has the bytes) */
#define OLD_NSCENES 16
typedef struct {
    uint32_t magic, format;
    int16_t par[DR_NPARAMS];
    uint8_t leds;
    uint8_t rsv[15];
    uint16_t scene_used;
    int16_t scene[OLD_NSCENES][DR_NPARAMS];
} project_old_t;
static union {
    project_t p;
    project_old_t o;
    uint8_t raw[PLAT_STORE_MAX];
} ld;

static int project_valid(void)
{
    int i;
    uint32_t from = proj.format;
    if (proj.magic != PROJ_MAGIC || from < 19u || from > PROJ_FORMAT)
        return 0;
    if (proj.par[P_LFO_RATE] < 0)              /* slower than 50 s, as it went before 2026-10-14: the slowest */
        proj.par[P_LFO_RATE] = 0;
    convert(proj.par, from);
    for (i = 0; i < NSCENES; i++) {
        if (proj.scene[i][P_LFO_RATE] < 0)
            proj.scene[i][P_LFO_RATE] = 0;
        convert(proj.scene[i], from);
    }
    if (from < 21u) {                          /* 19, 20: scenes 13-16 to B1-B4 */
        bank_blank(&bank_bc[0]);
        for (i = 0; i < 4; i++)
            if (ld.o.scene_used >> (12 + i) & 1u) {
                memcpy(bank_bc[0].scene[i], ld.o.scene[12 + i], sizeof bank_bc[0].scene[i]);
                if (bank_bc[0].scene[i][P_LFO_RATE] < 0)
                    bank_bc[0].scene[i][P_LFO_RATE] = 0;
                convert(bank_bc[0].scene[i], from);
                bank_bc[0].used |= (uint16_t)(1u << i);
            }
        proj.scene_used &= (1u << NSCENES) - 1u;
        proj.bank = 0;
        memset(proj.rsv, 0, sizeof proj.rsv);
        bank_dirty |= 1u << 1;                 /* B written with the next save */
    }
    proj.format = PROJ_FORMAT;
    if (proj.bank >= NBANKS)
        proj.bank = 0;
    if (!in_range(proj.par))
        return 0;
    for (i = 0; i < NSCENES; i++)              /* a scene out of range is dropped, not the project */
        if ((proj.scene_used >> i & 1u) && !in_range(proj.scene[i]))
            proj.scene_used &= (uint16_t)~(1u << i);
    return 1;
}

static void bank_load(int b)                   /* B (1) or C (2): from its object, if it's sound */
{
    bank_t *k = &bank_bc[b - 1];
    int i;
    memset(ld.raw, 0, sizeof ld.raw);
    if (plat_store_load(b == 1 ? OBJ_BANKB : OBJ_BANKC, ld.raw, sizeof ld.raw) < 8)
        return;
    memcpy(k, ld.raw, sizeof *k);
    if (k->magic != BANK_MAGIC || k->format < 21u || k->format > PROJ_FORMAT) {
        bank_blank(k);
        return;
    }
    for (i = 0; i < NSCENES; i++) {
        convert(k->scene[i], k->format);
        if ((k->used >> i & 1u) && !in_range(k->scene[i]))
            k->used &= (uint16_t)~(1u << i);
    }
    k->format = PROJ_FORMAT;
}

int project_load(void)
{
    int n;
    presets_init();
    bank_dirty = 0;
    bank_blank(&bank_bc[0]);
    bank_blank(&bank_bc[1]);
    memset(ld.raw, 0, sizeof ld.raw);
    n = plat_store_load(OBJ_PROJ, ld.raw, sizeof ld.raw);
    memcpy(&proj, ld.raw, sizeof proj);
    if (n < 8 || proj.magic != PROJ_MAGIC) {
        project_defaults();
        return -1;
    }
    if (proj.format >= 21u) {                  /* B and C first: a converted 0.1 project fills B itself */
        bank_load(1);
        bank_load(2);
    }
    if (project_valid())
        return 0;
    project_defaults();
    return -1;
}

int project_save(void)                         /* the banks first: a converted 0.1 project's B1-B4 are on
                                                * the flash before the project that no longer holds them */
{
    int rc = 0, b;
    for (b = 1; b <= 2; b++)
        if (!rc && (bank_dirty >> b & 1u)) {
            rc = plat_store_save(b == 1 ? OBJ_BANKB : OBJ_BANKC, &bank_bc[b - 1], sizeof bank_bc[b - 1]);
            if (!rc)
                bank_dirty &= (uint8_t)~(1u << b);
        }
    return rc ? rc : plat_store_save(OBJ_PROJ, &proj, sizeof proj);
}

void project_apply(void)
{
    int i;
    for (i = 0; i < DR_NPARAMS; i++)
        dr_set(i, proj.par[i]);
}
