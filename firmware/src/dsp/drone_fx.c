/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift's effects, after the CPM DS-2's (its manual): the drones -> a DELAY -> a REVERB, with a
 * dry/wet mix for each. Included by drone.c (one compilation unit); uses its parameters, its LFO and its
 * helpers. Float only, no libm (fastmath.h). They run while the drone is stopped too, so tails ring out.
 *
 * DELAY: a PT2399-style echo. 70 .. 450 ms (exponential), mono. The time follows the knob slowly (~50 ms),
 * so turning it bends the repeats' pitch as tape does, and an LFO tap can move it (+-25 %, the warble).
 * Each repeat goes round a soft clip, a low-pass (~3.5 kHz: darker each time) and a low cut (~80 Hz);
 * Repeats past ~85 run away into self-oscillation, held by the clip ("space echo"). The wet out is
 * low-passed again (~5 kHz), as the chip's. Its line is int16 (64 KB) in the .pool RAM.
 *
 * REVERB: the plate of Jon Dattorro's "Effect Design, Part 1" (JAES, 1997): a pre-delay, a band-limit, four
 * input diffusers, then a figure-of-eight tank of two halves (a modulated allpass, a delay, damping, the
 * decay, an allpass, a delay), stereo taps from both halves. Our own code from the paper's structure and
 * delay lengths (scaled from its 29.8 kHz to 44.1 kHz). Four types set its size, decay, damping and
 * pre-delay; CHORUS HALL adds a slow stereo chorus on the wet. The sides are blended a little, for a
 * mono speaker. A type change fades the wet out, clears
 * the tank at its new size, and fades it back. Its lines are float (~150 KB) in the .pool RAM. */

#ifdef OM_HOST
#define DR_POOL
#else
#define DR_POOL __attribute__((section(".pool")))
#endif

static uint32_t fx_quiet;                /* blocks the effects' output has been silent (dr_render rests them) */

/* ------------------------------------------------------------- delay --- */
#define DLY_N 32768u                     /* 743 ms: 450 ms + the LFO's 25 % + room */
#define DLY_MASK (DLY_N - 1u)
static int16_t dly_buf[DLY_N] DR_POOL;
static uint32_t dly_w;
static float dly_t = 0.177f * DR_SR;    /* the time, in samples, smoothed */
static float dly_lp, dly_hp, dly_olp;    /* the loop's low-pass, its low cut's state, the output low-pass */

static float dly_ms(int v) { return 70.0f * fm_exp2f((float)v * 0.026845f); }   /* 70 * (450/70)^(v/100) */

static inline float dly_read(float d)    /* d samples back, linearly interpolated */
{
    float p = (float)dly_w - d, f;
    int32_t i;
    if (p < 0.0f)
        p += (float)DLY_N;
    i = (int32_t)p;
    f = p - (float)i;
    return ((float)dly_buf[(uint32_t)i & DLY_MASK] * (1.0f - f) + (float)dly_buf[(uint32_t)(i + 1) & DLY_MASK] * f) *
           (1.0f / 16384.0f);
}

/* ------------------------------------------------------------ reverb --- */
enum { RV_ROOM, RV_PLATE, RV_HALL, RV_CHORUS_HALL, RV_NTYPES };
typedef struct {
    float size, decay, damp, predelay_ms, bw;
} rv_type_t;
static const rv_type_t RV_TYPE[RV_NTYPES] = {
    {0.45f, 0.45f, 0.25f, 4.0f, 0.75f},     /* ROOM: small, short, bright */
    {0.75f, 0.65f, 0.12f, 8.0f, 0.85f},     /* PLATE: medium, long, bright */
    {1.00f, 0.82f, 0.45f, 25.0f, 0.70f},    /* HALL: the longest, warm */
    {1.00f, 0.86f, 0.50f, 25.0f, 0.65f},    /* CHORUS HALL: as the hall, a little longer and warmer, chorused */
};
/* Dattorro's delays at 29761 Hz, in order: 4 input diffusers; tank A: modulated allpass, delay, allpass,
 * delay; tank B: the same */
static const uint16_t RV_LEN[12] = {142, 107, 379, 277, 672, 4453, 1800, 3720, 908, 4217, 2656, 3163};
#define RV_SCALE (DR_SR / 29761.0f)
#define RV_EXC 24                        /* the modulated allpasses' swing, samples (Dattorro's 16, scaled) */
#define RV_PRE_N 2048u                   /* the pre-delay's line (46 ms) */
#define RV_CAP 33600u                    /* the twelve lines at full size (~33400), with the swings */
static float rv_mem[RV_CAP] DR_POOL;
static float rv_pre[RV_PRE_N] DR_POOL;
static struct {
    float *line[12];
    uint32_t len[12], pos[12];           /* each line's length and write position */
    uint32_t pre_w, pre_d;
    uint32_t td[14];                     /* the output taps' distances, at this size */
    float bw_lp, damp_a, damp_b, tank_a, tank_b;   /* the input band-limit, the dampings, the cross-feeds */
    float mod_ph, gain;                  /* the allpasses' modulation phase; the wet's fade (a type change) */
    float in_g;                          /* the tank's input, fading in after a rebuild */
    int type;                            /* the type running; -1 none yet */
    float cho_ph;
} rv = {.type = -1};
#define CHO_N 2048u                      /* the chorus's lines (46 ms), one a side */
static float cho_l[CHO_N] DR_POOL, cho_r[CHO_N] DR_POOL;
static uint32_t cho_w;
_Static_assert((RV_PRE_N & (RV_PRE_N - 1u)) == 0 && (CHO_N & (CHO_N - 1u)) == 0, "powers of two: masks");

static void rv_setup(int type)           /* the lines at this type's size, cleared */
{
    _Static_assert(RV_CAP >= 33400u, "the reverb's lines at full size");
    float *m = rv_mem;
    uint32_t i, k;
    for (i = 0; i < 12u; i++) {
        uint32_t n = (uint32_t)((float)RV_LEN[i] * RV_SCALE * (i < 4u ? 1.0f : RV_TYPE[type].size));
        if (i == 4u || i == 8u)
            n += RV_EXC + 2u;            /* the modulated allpasses: room to swing */
        rv.line[i] = m;
        rv.len[i] = n;
        rv.pos[i] = 0;
        for (k = 0; k < n; k++)
            m[k] = 0.0f;
        m += n;
    }
    rv.pre_d = (uint32_t)(RV_TYPE[type].predelay_ms * 0.001f * DR_SR);
    {   /* Dattorro's output taps (at 29761 Hz), scaled to the size; each inside its line */
        static const uint16_t AT[14] = {266, 2974, 1913, 1996, 1990, 187, 1066, 353, 3627, 1228, 2673, 2111, 335, 121};
        static const uint8_t LN[14] = {5, 5, 6, 7, 9, 10, 11, 9, 9, 10, 11, 5, 6, 7};
        for (i = 0; i < 14u; i++) {
            uint32_t d = (uint32_t)((float)AT[i] * RV_SCALE * RV_TYPE[type].size);
            rv.td[i] = d < rv.len[LN[i]] ? d : rv.len[LN[i]] - 1u;
        }
    }
    rv.bw_lp = rv.damp_a = rv.damp_b = rv.tank_a = rv.tank_b = 0.0f;
    rv.in_g = 0.0f;                      /* the sound fades into the empty lines: started at once, its edge rang
                                          * on in the tank (a crackle on a soft sound, after a type change) */
    rv.type = type;
}

static inline float rv_tap(int i, uint32_t back)     /* line i, back (< its length) behind its write position */
{
    uint32_t n = rv.len[i], p = rv.pos[i] + n - 1u - back;
    return rv.line[i][p >= n ? p - n : p];
}
static inline void rv_push(int i, float x)
{
    rv.line[i][rv.pos[i]] = x;
    if (++rv.pos[i] >= rv.len[i])
        rv.pos[i] = 0;
}
/* an allpass on line i: g the coefficient; its delay is the line's length */
static inline float rv_allpass(int i, float x, float g)
{
    float d = rv.line[i][rv.pos[i]];     /* the oldest sample: the line's full length ago */
    float v = x - g * d;
    rv_push(i, v);
    return d + g * v;
}
/* the modulated allpass: its delay swings by +-RV_EXC/2 around the line's length less the swing room */
static inline float rv_allpass_mod(int i, float x, float g, float swing)
{
    uint32_t n = rv.len[i];
    float back = (float)(n - RV_EXC - 2u) + swing, f, d;
    uint32_t b = (uint32_t)back;
    f = back - (float)b;
    d = rv_tap(i, b) * (1.0f - f) + rv_tap(i, b + 1u) * f;
    {
        float v = x - g * d;
        rv_push(i, v);
        return d + g * v;
    }
}

static float fx_g[5];                            /* the repeats' and mixes' gains where the last block ended */
/* REPEATS: the loop's gain, 1.15 at 100 before its losses (the soft clip, ~3.5 kHz, the 80 Hz cut); over 85,
 * where the screen says "runaway", a little more, so the top sings into runaway on a low drone too (it did only
 * at ~100) */
static float rep_g(float v) { return v * 0.0115f + fm_maxf(v - 85.0f, 0.0f) * 0.006f; }
static void fx_gains(void)                       /* ... where they're set: no ramp from nothing */
{
    fx_g[0] = rep_g(pfs[P_DLY_REPEATS]);
    fx_g[1] = fm_sqrtf(1.0f - pfs[P_DLY_MIX] * 0.01f);
    fx_g[2] = fm_sqrtf(pfs[P_DLY_MIX] * 0.01f);
    fx_g[3] = fm_sqrtf(1.0f - pfs[P_REV_MIX] * 0.01f);
    fx_g[4] = fm_sqrtf(pfs[P_REV_MIX] * 0.01f) * 0.7f;
}

static void fx_clear(void)
{
    uint32_t k;
    for (k = 0; k < DLY_N; k++)
        dly_buf[k] = 0;
    dly_lp = dly_hp = dly_olp = 0.0f;
    dly_t = dly_ms(par[P_DLY_TIME]) * 0.001f * DR_SR;    /* at its time, no glide from an old one */
    for (k = 0; k < CHO_N; k++)
        cho_l[k] = cho_r[k] = 0.0f;
    for (k = 0; k < RV_PRE_N; k++)
        rv_pre[k] = 0.0f;
    rv_setup(rv.type >= 0 ? rv.type : RV_HALL);
    rv.gain = 1.0f;
    rv.in_g = 1.0f;
    fx_gains();
    fx_quiet = 0;
}

/* the effects, in place on the stereo block l, r (n frames) */
static void fx_run(float *l, float *r, uint32_t n)
{
    /* the mixes and the repeats as they glide (pfs), and in a straight line across the block from where the
     * last one ended: read raw (par), each detent was a step, a click on a soft sound */
    const float ms = dly_ms(par[P_DLY_TIME]), fbg1 = rep_g(pfs[P_DLY_REPEATS]);
    const float dm = pfs[P_DLY_MIX] * 0.01f, dry1e = fm_sqrtf(1.0f - dm), wet1e = fm_sqrtf(dm);
    const float rm = pfs[P_REV_MIX] * 0.01f, dry2e = fm_sqrtf(1.0f - rm), wet2e = fm_sqrtf(rm) * 0.7f;
    const float dn = 1.0f / (float)(n ? n : 1u);
    float fbg = fx_g[0], dry1 = fx_g[1], wet1 = fx_g[2], dry2 = fx_g[3], wet2 = fx_g[4];
    const float dfb = (fbg1 - fbg) * dn, dd1 = (dry1e - dry1) * dn, dw1 = (wet1e - wet1) * dn;
    const float dd2 = (dry2e - dry2) * dn, dw2 = (wet2e - wet2) * dn;
    const float rstep = 1.0f / (morph_s() * DR_SR);    /* its LFO route switched: crossfade (Morph) */
    const float damt = depth_amt(par[P_DLY_LFO_DEPTH]) * 0.25f * pfs[P_MOVE] * 0.02f;   /* x the MOVEMENT */
    const int want = par[P_REV_TYPE] < RV_NTYPES ? par[P_REV_TYPE] : RV_HALL;
    float decay = RV_TYPE[rv.type].decay, damp = RV_TYPE[rv.type].damp, bw = RV_TYPE[rv.type].bw;
    int chorus = rv.type == RV_CHORUS_HALL;
    float sw = 0.0f, sw2 = 0.0f, cdl = 600.0f, cdr = 600.0f;
    uint32_t j;
    if ((uint8_t)par[P_DLY_LFO] != dly_rt) {
        dly_rp = dly_rt;
        dly_rt = (uint8_t)par[P_DLY_LFO];
        dly_rw = 0.0f;
    }
    for (j = 0; j < n; j++) {
        float x, y, w, rin, a, b, ol, or_;
        fbg += dfb;
        dry1 += dd1;
        wet1 += dw1;
        dry2 += dd2;
        wet2 += dw2;
        /* DELAY */
        x = (l[j] + r[j]) * 0.5f;
        {
            float lfo = lfo_at(dly_rt, j / CR);
            if (dly_rw < 1.0f) {
                dly_rw = fm_minf(dly_rw + rstep, 1.0f);
                lfo = lfo_at(dly_rp, j / CR) + (lfo - lfo_at(dly_rp, j / CR)) * dly_rw;
            }
            float t = ms * 0.001f * DR_SR * (1.0f + damt * lfo);
            dly_t += (t - dly_t) * 0.00045f;                       /* ~50 ms: tape-like */
        }
        y = dly_read(fm_clampf(dly_t, 4.0f, (float)(DLY_N - 4u)));
        dly_olp += (y - dly_olp) * 0.51f;                          /* the wet out: ~5 kHz */
        w = sat(y * fbg);                                          /* the loop: soft clip, darker, no rumble */
        dly_lp += (w - dly_lp) * 0.39f;                            /* ~3.5 kHz */
        dly_hp += (dly_lp - dly_hp) * 0.0114f;                     /* ~80 Hz, taken away */
        {
            float s = fm_clampf(x + dly_lp - dly_hp, -1.99f, 1.99f);
            dly_buf[dly_w] = (int16_t)(s * 16384.0f);
            dly_w = (dly_w + 1u) & DLY_MASK;
        }
        l[j] = l[j] * dry1 + dly_olp * wet1;
        r[j] = r[j] * dry1 + dly_olp * wet1;
        /* REVERB: the input, pre-delayed and band-limited, through the four diffusers */
        rv_pre[rv.pre_w] = (l[j] + r[j]) * 0.3f;
        rin = rv_pre[(rv.pre_w - rv.pre_d) & (RV_PRE_N - 1u)] * rv.in_g;
        if (rv.in_g < 1.0f)
            rv.in_g = fm_minf(rv.in_g + 1.0f / (0.05f * DR_SR), 1.0f);   /* ~50 ms */
        rv.pre_w = (rv.pre_w + 1u) & (RV_PRE_N - 1u);
        rv.bw_lp += (rin - rv.bw_lp) * bw;
        rin = rv_allpass(0, rv.bw_lp, 0.75f);
        rin = rv_allpass(1, rin, 0.75f);
        rin = rv_allpass(2, rin, 0.625f);
        rin = rv_allpass(3, rin, 0.625f);
        /* the tank: A takes B's last output, B takes A's (the figure of eight) */
        if (!(j % CR)) {                 /* the swings, once a control tick */
            sw = (fm_sin_turns(rv.mod_ph) + 1.0f) * (float)(RV_EXC / 2);
            sw2 = (fm_sin_turns(rv.mod_ph + 0.25f) + 1.0f) * (float)(RV_EXC / 2);
            if (chorus) {
                cdl = 600.0f + 180.0f * fm_sin_turns(rv.cho_ph);
                cdr = 600.0f + 180.0f * fm_sin_turns(rv.cho_ph + 0.25f);
            }
        }
        {
            a = rv_allpass_mod(4, rin + rv.tank_b, -0.7f, sw);
            rv_push(5, a);
            a = rv_tap(5, rv.len[5] - 1u);
            rv.damp_a += (a - rv.damp_a) * (1.0f - damp);
            a = rv_allpass(6, rv.damp_a * decay, 0.5f);
            rv_push(7, a);
            b = rv_allpass_mod(8, rin + rv.tank_a, -0.7f, sw2);
            rv_push(9, b);
            b = rv_tap(9, rv.len[9] - 1u);
            rv.damp_b += (b - rv.damp_b) * (1.0f - damp);
            b = rv_allpass(10, rv.damp_b * decay, 0.5f);
            rv_push(11, b);
            rv.tank_a = fm_flush(rv_tap(7, rv.len[7] - 1u) * decay);
            rv.tank_b = fm_flush(rv_tap(11, rv.len[11] - 1u) * decay);
        }
        /* the stereo taps (Dattorro's table) */
        ol = rv_tap(5, rv.td[0]) + rv_tap(5, rv.td[1]) - rv_tap(6, rv.td[2]) + rv_tap(7, rv.td[3]) -
             rv_tap(9, rv.td[4]) - rv_tap(10, rv.td[5]) - rv_tap(11, rv.td[6]);
        or_ = rv_tap(9, rv.td[7]) + rv_tap(9, rv.td[8]) - rv_tap(10, rv.td[9]) + rv_tap(11, rv.td[10]) -
              rv_tap(5, rv.td[11]) - rv_tap(6, rv.td[12]) - rv_tap(7, rv.td[13]);
        {   /* a quarter of each side into the other: the hall's two sides lean to mirror images, which a
             * mono speaker (the FM-1's) would partly cancel; this keeps every type within ~4 dB in mono */
            float t = ol;
            ol += 0.25f * or_;
            or_ += 0.25f * t;
        }
        if (chorus) {                    /* CHORUS HALL: each side's wet against a slowly swinging copy */
            float cl, cr;
            cho_l[cho_w] = ol;
            cho_r[cho_w] = or_;
            {
                uint32_t il = (uint32_t)cdl, ir = (uint32_t)cdr, m = CHO_N - 1u;
                float fl = cdl - (float)il, fr = cdr - (float)ir;
                cl = cho_l[(cho_w - il) & m] * (1.0f - fl) + cho_l[(cho_w - il - 1u) & m] * fl;
                cr = cho_r[(cho_w - ir) & m] * (1.0f - fr) + cho_r[(cho_w - ir - 1u) & m] * fr;
            }
            cho_w = (cho_w + 1u) & (CHO_N - 1u);
            ol = (ol + cl) * 0.7f;
            or_ = (or_ + cr) * 0.7f;
            rv.cho_ph += 0.35f / DR_SR;
            if (rv.cho_ph >= 1.0f)
                rv.cho_ph -= 1.0f;
        }
        rv.mod_ph += 1.0f / DR_SR;       /* the tank's swing: 1 Hz */
        if (rv.mod_ph >= 1.0f)
            rv.mod_ph -= 1.0f;
        /* a type change: the wet fades out (~20 ms), the tank is rebuilt, it fades back in over ~200 ms (in
         * 20 ms the sound's restart through the empty diffusers crackled, on a soft sound) */
        if (want != rv.type) {
            rv.gain -= 0.0012f;
            if (rv.gain <= 0.0f) {
                rv.gain = 0.0f;
                rv_setup(want);          /* the new tank starts empty; the wet comes back in from 0 */
                decay = RV_TYPE[rv.type].decay;
                damp = RV_TYPE[rv.type].damp;
                bw = RV_TYPE[rv.type].bw;
                chorus = rv.type == RV_CHORUS_HALL;
            }
        } else if (rv.gain < 1.0f) {
            rv.gain = fm_minf(rv.gain + 0.000113f, 1.0f);
        }
        l[j] = l[j] * dry2 + ol * wet2 * rv.gain;
        r[j] = r[j] * dry2 + or_ * wet2 * rv.gain;
    }
    fx_g[0] = fbg1;
    fx_g[1] = dry1e;
    fx_g[2] = wet1e;
    fx_g[3] = dry2e;
    fx_g[4] = wet2e;
}
