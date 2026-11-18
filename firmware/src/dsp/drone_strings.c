/* SPDX-License-Identifier: GPL-3.0-only */
/* THE STRINGS (0.4, FX's third page): eight strings ringing in sympathy with the drones. Mutable Instruments Rings'
 * sympathetic-string model, ported to C from Emilie Gillet's code (MIT; github.com/pichenettes/eurorack f4cfa76,
 * stmlib d18def8: rings/dsp/string.cc and .h, part.cc and .h, note_filter.h; stmlib/dsp/filter.h, delay_line.h,
 * parameter_interpolator.h; rings/resources.cc's svf_shift table; LICENSING.md, its notice in
 * LICENSES/MIT-Mutable-Instruments.txt). As Rings plays it with polyphony 1,
 * no internal exciter, tonic 0 and its note drone I's. The knobs (P_ST_*): Mix, Chord (Rings' Structure, its jumps
 * kept), Tone (Brightness), Length (Damping), Hollow (Position, its whole turn from the nut to the middle: Rings folds
 * it at 0.5, Evan 2026-11-18).
 * Changed from Rings: 44.1 kHz; each string's line 16-bit (+-64), rounded up or down at random in proportion, and
 * 1024 long (notes down to 43 Hz; below that Rings' own upsampler, as it does under 11.7 Hz); SemitonesToRatio from
 * fm_exp2f (its steps kept); the tangents' constants rounded to float once; the strings' dispersion (off in this
 * model) left out; the chord's fifths the Tuning's (dr_tun_over), its octaves pure.
 * WHERE IT RUNS: the second core (CPU1), one block behind. Each block the platform hands the job over (dr_st_job_on,
 * then fm1_cpu1_run) before the drones; the job reads st_job_in (the drones the last block left, mono, and the knobs)
 * and writes st_out; st_mix, after the PLAY fade and before the effects, waits for it (dr_st_wait), checks st_out,
 * adds it, then publishes this block's drones into st_job_in for the next. Who writes what:
 *   st_job_in: CPU0, only in st_mix after the wait (no job is out then); the job reads it
 *   st_js, st_line, st_out: the job alone; CPU0 reads st_out after the wait, the picture the lines (read-only)
 *   st_ramp, st_lvl_s, st_hold (on / off and the level): CPU0 alone, from the parameters (dr_set's queue)
 * The host and the emulator (no second core) run the job at the block's start on the same st_job_in: the same
 * samples, wherever in the block it runs (drone_test tries it at the wait too). Included in drone.c. */

#define ST_N 8u
#define ST_LINE 1024u                      /* a power of two: a mask */
#define ST_CH 24u                          /* Rings' block (its parameters glide over it) */
#define ST_SR 44100.0f
#define ST_GAIN 0.25f                      /* Rings' 1 / sqrt(strings x 2) */
#define ST_A3 (440.0f / ST_SR)
#define ST_QK (32767.0f / 64.0f)           /* the lines' 16 bits: +-64 */
#define ST_QI (64.0f / 32767.0f)
#define ST_FADE_S 4.0f                     /* Mix from 0, or to 0: they wash in and out over 4 s */
#define ST_OUT_MAX 4.0f                    /* st_out's check: every sample finite and within this */

typedef struct { float g, r, h, s1, s2; } st_svf_t;
typedef struct {
    float freq, delay, cpos, dcomp;        /* frequency_, delay_, clamped_position_, previous_damping_compensation_ */
    float src, o0, o1, a0, a1;             /* the upsampler's phase, out_sample_, aux_sample_ */
    float fx1, fx2, fbr, fbri, fdm, fdmi;  /* the FIR damping filter */
    st_svf_t iir;                          /* the IIR damping filter */
    uint32_t w;                            /* the line's write pointer (it counts down) */
} st_str_t;

static struct {                            /* CPU0 -> the job (above) */
    float in[BLK];                         /* the drones, mono, the last block's */
    uint32_t k;                            /* its frames (0: none yet) */
    float note, f5;                        /* drone I's pitch (MIDI, gliding); the Tuning's fifth over its set note */
    float tgt[4];                          /* Chord, Tone, Length, Hollow as Rings' Structure, Brightness, Damping,
                                            * Position, 0..1 */
    uint32_t clear;                        /* counts up: the job first clears the lines and its state (faded out) */
} st_job_in DR_POOL;
static int16_t st_line[ST_N][ST_LINE];     /* 16 KB of main RAM (the pool can't hold them: the plan's gate G1) */
static float st_out[2][BLK] DR_POOL;       /* the strings for the block (the job writes, CPU0 reads after its wait) */
static struct {                            /* the job's own */
    st_str_t s[ST_N];
    struct {                               /* Rings' Part's */
        float nv[4], note, nc;             /* the note filter: its last 4, note_, coefficient_ */
        st_svf_t exc;                      /* the excitation filter */
        float dcx, dcy;                    /* the input's DC blocker */
        float peak;                        /* the limiter's */
    } p;
    float cur[4];                          /* the knobs as heard, glided per 24-sample chunk (~25 ms) */
    uint32_t rng;                          /* the lines' rounding (the same on the host and the FM-1) */
    uint32_t cleared;                      /* st_job_in.clear as last done */
} st_js;
#define ST_STRUCT st_js.cur[0]
#define ST_BRIGHT st_js.cur[1]
#define ST_DAMP st_js.cur[2]
#define ST_POS st_js.cur[3]
#ifdef ST_MEASURE
#include <stdio.h>
float st_xmax;
__attribute__((destructor)) static void st_pr(void) { fprintf(stderr, "strings: line peak %.3f\n", (double)st_xmax); }
#endif

static float st_ramp;                      /* CPU0's: on / off, 0..1 (heard as its square) */
static float st_lvl_s, st_hold;            /* ... the level glided a sample; the level held while Mix is 0 (washing out) */
static int st_two;                         /* this block: 1 the job is out (wait), 2 it ran here, 0 none */
static uint32_t st_clears;                 /* CPU0's count for st_job_in.clear */
volatile uint8_t dr_st_avail = 1;          /* the platform: 0 the second core can't run them (none, or retired) */
int (*dr_st_wait)(void);                   /* the platform's wait for the second core (0 done, -1 the job dropped);
                                            * NULL on the host, which runs the job in place */
volatile int dr_st_two;                    /* the platform: 1 it handed this block's job to the second core */
volatile uint8_t dr_st_failed;             /* st_mix: the job's output failed its check (the platform retires CPU1) */
volatile uint8_t dr_st_poison;             /* a debug query: the next output spoiled (a NaN) to try the check */
int dr_st_late;                            /* the host's test: the job run at the wait, not the block's start */

static const float st_svf_shift[132] = {   /* Rings' lut_svf_shift (2 atan(2^-(i/12)) / 2 pi), the part it reads */
2.500000000e-01f, 2.408119579e-01f, 2.316544611e-01f, 2.225575501e-01f, 2.135502761e-01f, 2.046602549e-01f,
1.959132760e-01f, 1.873329789e-01f, 1.789406032e-01f, 1.707548172e-01f, 1.627916233e-01f, 1.550643347e-01f,
1.475836177e-01f, 1.403575876e-01f, 1.333919506e-01f, 1.266901772e-01f, 1.202537001e-01f, 1.140821254e-01f,
1.081734480e-01f, 1.025242668e-01f, 9.712999179e-02f, 9.198504051e-02f, 8.708302003e-02f, 8.241689360e-02f,
7.797913038e-02f, 7.376183852e-02f, 6.975688172e-02f, 6.595598018e-02f, 6.235079694e-02f, 5.893301078e-02f,
5.569437701e-02f, 5.262677742e-02f, 4.972226058e-02f, 4.697307381e-02f, 4.437168789e-02f, 4.191081545e-02f,
3.958342416e-02f, 3.738274529e-02f, 3.530227864e-02f, 3.333579426e-02f, 3.147733169e-02f, 2.972119704e-02f,
2.806195849e-02f, 2.649444041e-02f, 2.501371653e-02f, 2.361510230e-02f, 2.229414676e-02f, 2.104662398e-02f,
1.986852431e-02f, 1.875604550e-02f, 1.770558386e-02f, 1.671372543e-02f, 1.577723728e-02f, 1.489305906e-02f,
1.405829467e-02f, 1.327020425e-02f, 1.252619642e-02f, 1.182382076e-02f, 1.116076060e-02f, 1.053482614e-02f,
9.943947824e-03f, 9.386169992e-03f, 8.859644866e-03f, 8.362626781e-03f, 7.893466717e-03f, 7.450607078e-03f,
7.032576744e-03f, 6.637986365e-03f, 6.265523903e-03f, 5.913950392e-03f, 5.582095932e-03f, 5.268855886e-03f,
4.973187279e-03f, 4.694105394e-03f, 4.430680542e-03f, 4.182035018e-03f, 3.947340207e-03f, 3.725813861e-03f,
3.516717519e-03f, 3.319354065e-03f, 3.133065427e-03f, 2.957230396e-03f, 2.791262569e-03f, 2.634608406e-03f,
2.486745394e-03f, 2.347180309e-03f, 2.215447582e-03f, 2.091107747e-03f, 1.973745986e-03f, 1.862970740e-03f,
1.758412418e-03f, 1.659722154e-03f, 1.566570656e-03f, 1.478647104e-03f, 1.395658114e-03f, 1.317326764e-03f,
1.243391669e-03f, 1.173606108e-03f, 1.107737206e-03f, 1.045565155e-03f, 9.868824789e-04f, 9.314933471e-04f,
8.792129165e-04f, 8.298667176e-04f, 7.832900713e-04f, 7.393275405e-04f, 6.978324110e-04f, 6.586662024e-04f,
6.216982059e-04f, 5.868050482e-04f, 5.538702800e-04f, 5.227839874e-04f, 4.934424252e-04f, 4.657476707e-04f,
4.396072968e-04f, 4.149340639e-04f, 3.916456285e-04f, 3.696642688e-04f, 3.489166247e-04f, 3.293334538e-04f,
3.108493994e-04f, 2.934027734e-04f, 2.769353496e-04f, 2.613921700e-04f, 2.467213608e-04f, 2.328739600e-04f,
2.198037532e-04f, 2.074671201e-04f, 1.958228884e-04f, 1.848321967e-04f, 1.744583648e-04f, 1.646667709e-04f,
};

static inline float st_softlimit(float x) { return x * (27.0f + x * x) / (27.0f + 9.0f * x * x); }
static inline float st_s2r(float semis)   /* SemitonesToRatio: as Rings' tables, in steps of 1/256 semitone (so this
                                            * gives Rings' samples: 144 dB under the strings on a noise test) */
{
    float p = semis + 128.0f;
    int32_t i = (int32_t)p, f = (int32_t)((p - (float)i) * 256.0f);
    return fm_exp2f((float)(i - 128) * (1.0f / 12.0f)) * fm_exp2f((float)f * (1.0f / 3072.0f));
}

static inline void st_svf_set(st_svf_t *f, float g, float q)
{
    f->g = g;
    f->r = 1.0f / q;
    f->h = 1.0f / (1.0f + f->r * f->g + f->g * f->g);
}
static inline float st_tan_dirty(float f) { return f * (3.14159274f + 11.5839453f * f * f); }
static inline float st_tan_acc(float f)
{
    float f2 = f * f;
    return f * (3.14159274f + f2 * (10.3353662f + f2 * (40.8206978f + f2 * (161.205307f + f2 * (86.4620361f +
                f2 * 2799.88354f)))));
}
static inline float st_svf_lp(st_svf_t *f, float in)
{
    float hp, bp, lp;
    hp = (in - f->r * f->s1 - f->g * f->s1 - f->s2) * f->h;
    bp = f->g * hp + f->s1;
    f->s1 = f->g * hp + bp;
    lp = f->g * bp + f->s2;
    f->s2 = f->g * bp + lp;
    return lp;
}

static void st_reset(void)                 /* the job's: the lines and the strings still, the knobs where they're set */
{
    uint32_t i, j;
    for (i = 0; i < ST_N; i++) {
        st_str_t *s = &st_js.s[i];
        for (j = 0; j < ST_LINE; j++)
            st_line[i][j] = 0;
        s->freq = 220.0f / ST_SR;
        s->delay = 1.0f / s->freq;
        s->cpos = s->dcomp = 0.0f;
        s->src = s->o0 = s->o1 = s->a0 = s->a1 = 0.0f;
        s->fx1 = s->fx2 = s->fbr = s->fbri = s->fdm = s->fdmi = 0.0f;
        st_svf_set(&s->iir, st_tan_dirty(0.01f), 100.0f);
        s->iir.s1 = s->iir.s2 = 0.0f;
        s->w = 0;
    }
    for (i = 0; i < 4u; i++) {
        st_js.p.nv[i] = 69.0f;
        st_js.cur[i] = st_job_in.tgt[i];
    }
    st_js.p.note = 69.0f;
    st_js.p.nc = 1.0f / (0.001f * (ST_SR / (float)ST_CH));
    st_svf_set(&st_js.p.exc, st_tan_dirty(0.01f), 100.0f);
    st_js.p.exc.s1 = st_js.p.exc.s2 = 0.0f;
    st_js.p.dcx = st_js.p.dcy = 0.0f;
    st_js.p.peak = 0.5f;
    st_js.rng = 0x2545F491u;
}

static void st_publish_knobs(void);

static void st_init(void)                  /* dr_init (no job is out) */
{
    uint32_t i;
    for (i = 0; i < BLK; i++)
        st_job_in.in[i] = st_out[0][i] = st_out[1][i] = 0.0f;
    st_job_in.k = 0;
    st_job_in.note = 69.0f;
    st_job_in.f5 = 7.0f;
    st_publish_knobs();
    st_reset();
    st_js.cleared = st_job_in.clear = st_clears = 0;
    st_ramp = st_lvl_s = st_hold = 0.0f;
    st_two = 0;
    dr_st_failed = dr_st_poison = 0;
}

static float st_note_filter(float note)    /* Rings' NoteFilter, its note() (strum never comes) */
{
    const float fast = 1.0f / (0.001f * (ST_SR / (float)ST_CH)), slow = 1.0f / (0.010f * (ST_SR / (float)ST_CH));
    const float lag = 1.0f / (0.050f * (ST_SR / (float)ST_CH));
    if (fm_fabsf(note - st_js.p.note) > 0.4f) {
        st_js.p.note = note;
        st_js.p.nc = fast;
        st_js.p.nv[0] = st_js.p.nv[1] = st_js.p.nv[2] = st_js.p.nv[3] = note;
    } else {
        float a, b, c, d, t, med;
        st_js.p.nv[0] = st_js.p.nv[1];
        st_js.p.nv[1] = st_js.p.nv[2];
        st_js.p.nv[2] = st_js.p.nv[3];
        st_js.p.nv[3] = note;
        a = st_js.p.nv[0], b = st_js.p.nv[1], c = st_js.p.nv[2], d = st_js.p.nv[3];   /* the median of 4: the middle two's mean */
        if (a > b) t = a, a = b, b = t;
        if (c > d) t = c, c = d, d = t;
        if (a > c) t = a, a = c, c = t;
        if (b > d) t = b, b = d, d = t;
        if (b > c) t = b, b = c, c = t;
        med = 0.5f * (b + c);
        st_js.p.note += st_js.p.nc * (med - st_js.p.note);
        st_js.p.nc += lag * (slow - st_js.p.nc);
    }
    return st_js.p.note;
}

static inline float st_squash(float x)
{
    if (x < 0.5f) {
        x *= 2.0f;
        x *= x; x *= x; x *= x; x *= x;
        return x * 0.5f;
    }
    x = 2.0f - 2.0f * x;
    x *= x; x *= x; x *= x; x *= x;
    return 1.0f - 0.5f * x;
}

static void st_notes(float note, float f5, float chord, float *dst)   /* ComputeSympatheticStringsNotes, 8
                                            * strings, tonic 0; its fifths f5 (the Tuning's), its octaves pure */
{
    const float notes[9] = { 0.0f, note - 12.0f, note - f5, note, note + f5, note + 12.0f, note + 12.0f + f5,
                             note + 24.0f, note + 24.0f };
    const float detune[4] = { 0.013f, 0.011f, 0.007f, 0.017f };
    const uint32_t first_detuned = ST_N - ((ST_N - 1u) >> 1);   /* 5 */
    float parameter = chord;
    uint32_t i;
    for (i = 0; i < first_detuned; i++) {
        float n = 3.0f, nf;
        int32_t ni;
        if (i) {
            n = parameter * 7.0f;
            parameter += (1.0f - parameter) * 0.2f;
        }
        ni = (int32_t)n;
        nf = st_squash(n - (float)ni);
        dst[i] = notes[ni] + (notes[ni + 1] - notes[ni]) * nf;
        if (i + first_detuned < ST_N)
            dst[i + first_detuned] = dst[i] + detune[i & 3u];
    }
}

static inline float st_rd(const int16_t *l, uint32_t i) { return (float)l[i & (ST_LINE - 1u)] * ST_QI; }

static void st_string(st_str_t *s, int16_t *line, float f, float br, float dm, float po,
                      const float *in, float *out, float *aux, uint32_t k)
{
    float delay, src_ratio, cp, lf, rt60, rt, dcoef, bright, dcut, df, sh, step = 1.0f / (float)k;
    float dv, di, pv, pi, cv, ci;          /* the delay's, position's and damping compensation's interpolators */
    int32_t si;
    uint32_t wp = s->w & (ST_LINE - 1u);   /* (masked as it's loaded: a misread can't write outside the line) */
    s->freq += 1.0f * (f - s->freq);       /* set_frequency(f, glide 1) */
    delay = fm_clampf(1.0f / s->freq, 4.0f, (float)ST_LINE - 4.0f);
    src_ratio = delay * s->freq;
    if (src_ratio >= 0.9999f) {            /* above the line's lowest note: no upsampling */
        s->src = 1.0f;
        src_ratio = 1.0f;
    }
    cp = 0.5f - 0.98f * fm_fabsf(po - 0.5f);
    dv = s->delay, di = (delay - s->delay) / (float)k;
    pv = s->cpos, pi = (cp - s->cpos) / (float)k;
    lf = dm * (2.0f - dm);
    rt60 = 0.07f * st_s2r(lf * 96.0f) * ST_SR;
    rt = fm_maxf(-120.0f * delay / src_ratio / rt60, -127.0f);
    dcoef = st_s2r(rt);
    bright = br * br;
    dcut = fm_minf(24.0f + dm * dm * 48.0f + br * br * 24.0f, 84.0f);
    df = fm_minf(s->freq * st_s2r(dcut), 0.499f);
    if (dm >= 0.95f) {                     /* towards an endless ring */
        float inf = 20.0f * (dm - 0.95f);
        dcoef += inf * (1.0f - dcoef);
        bright += inf * (1.0f - bright);
        df += inf * (0.4999f - df);
        dcut += inf * (128.0f - dcut);
    }
    s->fdmi = (dcoef - s->fdm) * step;     /* DampingFilter::Configure */
    s->fbri = (bright - s->fbr) * step;
    st_svf_set(&s->iir, st_tan_acc(df), 0.5f);
    si = (int32_t)dcut;
    sh = st_svf_shift[si] + (st_svf_shift[si + 1] - st_svf_shift[si]) * (dcut - (float)si);
    cv = s->dcomp, ci = ((1.0f - sh) - s->dcomp) / (float)k;
    while (k--) {
        s->src += src_ratio;
        if (s->src > 1.0f) {
            float d, comb, x, df2, xm1, x0, x1, x2, c, v, w, a, b;
            uint32_t t;
            int32_t ii;
            s->src -= 1.0f;
            dv += di;
            pv += pi;
            cv += ci;
            d = dv;
            comb = d * pv;
            d *= cv;                       /* the IIR's delay */
            d -= 1.0f;                     /* the FIR's */
            ii = (int32_t)d;               /* ReadHermite */
            df2 = d - (float)ii;
            t = wp + (uint32_t)ii + ST_LINE;
            xm1 = st_rd(line, t - 1u), x0 = st_rd(line, t), x1 = st_rd(line, t + 1u), x2 = st_rd(line, t + 2u);
            c = (x1 - xm1) * 0.5f;
            v = x0 - x1;
            w = c + v;
            a = w + v + (x2 - x0) * 0.5f;
            b = w + a;
            x = (((a * df2) - b) * df2 + c) * df2 + x0;
            x += *in;
            {                              /* DampingFilter::Process */
                float h0 = (1.0f + s->fbr) * 0.5f, h1 = (1.0f - s->fbr) * 0.25f, y = s->fdm * (h0 * s->fx1 + h1 * (x + s->fx2));
                s->fx2 = s->fx1;
                s->fx1 = x;
                s->fbr += s->fbri;
                s->fdm += s->fdmi;
                x = y;
            }
            x = st_svf_lp(&s->iir, x);
            {                              /* Write, 16-bit */
#ifdef ST_MEASURE
                if (fm_fabsf(x) > st_xmax) st_xmax = fm_fabsf(x);
#endif
                float q = fm_clampf(x * ST_QK, -32768.0f, 32767.0f);
                {                          /* rounded up or down at random, in proportion (so on average
                                            * exact): rounding to the nearest let a quiet string hold its level for good
                                            * (a limit cycle: it rang on after the drones stopped), rounding toward zero
                                            * damped every string (3.6 dB quieter than Rings) */
                    float fl = fm_floorf(q);
                    st_js.rng = st_js.rng * 1664525u + 1013904223u;
                    if ((float)(st_js.rng >> 8) * (1.0f / 16777216.0f) < q - fl && fl < 32767.0f)
                        fl += 1.0f;
                    line[wp] = (int16_t)fl;
                }
                wp = (wp - 1u) & (ST_LINE - 1u);
            }
            s->o1 = s->o0;
            s->a1 = s->a0;
            s->o0 = x;
            ii = (int32_t)comb;            /* Read, linear */
            {
                float fr = comb - (float)ii, ra = st_rd(line, wp + (uint32_t)ii), rb = st_rd(line, wp + (uint32_t)ii + 1u);
                s->a0 = ra + (rb - ra) * fr;
            }
        }
        *out++ += s->o1 + (s->o0 - s->o1) * s->src;
        *aux++ += s->a1 + (s->a0 - s->a1) * s->src;
        in++;
    }
    s->w = wp;
    s->delay = dv;
    s->cpos = pv;
    s->dcomp = cv;
    s->fx1 = fm_flush(s->fx1);             /* (silent drones: the states decay toward denormals) */
    s->fx2 = fm_flush(s->fx2);
    s->iir.s1 = fm_flush(s->iir.s1);
    s->iir.s2 = fm_flush(s->iir.s2);
}


void dr_st_job(uint32_t arg)               /* the strings for one block: st_job_in -> st_out (on the second core) */
{
    const uint32_t kk = st_job_in.k < BLK ? st_job_in.k : BLK;   /* (a misread count can't run past the block) */
    uint32_t o, j, i;
    (void)arg;
    if (st_js.cleared != st_job_in.clear) {    /* back after fading out: no old tail */
        st_reset();
        st_js.cleared = st_job_in.clear;
    }
    for (o = 0; o < kk; o += ST_CH) {
        const uint32_t k = kk - o < ST_CH ? kk - o : ST_CH;
        float in[ST_CH], out[ST_CH], aux[ST_CH], fr[ST_N], note, cut, fc;
        for (j = 0; j < 4u; j++)
            st_js.cur[j] += 0.02f * (st_job_in.tgt[j] - st_js.cur[j]);
        note = st_note_filter(st_job_in.note);
        cut = ST_BRIGHT * (2.0f - ST_BRIGHT);
        fc = fm_minf(0.4f * st_s2r((cut - 1.0f) * 108.0f), 0.499f);
        st_svf_set(&st_js.p.exc, st_tan_dirty(fc), 0.8f);
        for (j = 0; j < k; j++) {
            float x = st_svf_lp(&st_js.p.exc, st_job_in.in[o + j] * ST_GAIN), ox = st_js.p.dcx;
            st_js.p.dcx = x;               /* the DC blocker */
            st_js.p.dcy = st_js.p.dcy * (1.0f - 10.0f / ST_SR) + x - ox;
            in[j] = st_js.p.dcy;
            out[j] = aux[j] = 0.0f;
        }
        st_js.p.exc.s1 = fm_flush(st_js.p.exc.s1);
        st_js.p.exc.s2 = fm_flush(st_js.p.exc.s2);
        st_js.p.dcy = fm_flush(st_js.p.dcy);
        st_notes(note, st_job_in.f5, ST_STRUCT, fr);
        /* Rings rings its upper strings longer, toward 0.95 whatever the Damping: kept from 0.7 up (test build 2's
         * sound), narrowed below it so a short Length is short (the renders: at 0.3 the upper strings still rang ~10 s) */
        for (i = 0; i < ST_N; i++)
            st_string(&st_js.s[i], st_line[i], st_s2r(fr[i] - 69.0f) * ST_A3, ST_BRIGHT,
                      ST_DAMP + ((float)i / (float)ST_N) * (0.95f - ST_DAMP) * fm_minf(1.0f, ST_DAMP * (1.0f / 0.7f)),
                      ST_POS, in, out, aux, k);
        for (j = 0; j < k; j++) {          /* Rings' Limiter (pre-gain 1 for this model): 0.8, a slow peak, a soft clip */
            const float l = out[j], r = aux[j];
            const float pk = fm_maxf(fm_maxf(fm_fabsf(l), fm_fabsf(r)), fm_fabsf(r - l)), e = pk - st_js.p.peak;
            float g;
            st_js.p.peak += (e > 0.0f ? 0.05f : 0.00002f) * e;
            g = (st_js.p.peak <= 1.0f ? 1.0f : 1.0f / st_js.p.peak) * 0.8f;
            st_out[0][o + j] = st_softlimit(l * g);
            st_out[1][o + j] = st_softlimit(r * g);
        }
        st_js.p.peak = fm_flush(st_js.p.peak);
    }
}

/* the Mix and Hollow knobs as a gain: Mix (k / 70)^2 (70: the test builds' level, 100 +6 dB); Hollow evens its own
 * level (the renders, 2026-11-16: the strings ~12 dB louder at the ends than in the middle, linear in the distance from
 * the middle; 0 dB at Rings' 0.3, Hollow 60) */
static float st_level(void)
{
    const float a = pfs[P_ST_MIX] * (1.0f / 70.0f), d = 0.5f - pfs[P_ST_HOLLOW] * 0.005f;
    return a * a * fm_exp2f(-23.8f * (d - 0.2f) * 0.16609640f);   /* dB -> a gain: 2^(dB / 6.02) */
}

static void st_publish_knobs(void)         /* (CPU0, no job out) the knobs as Rings' 0..1 */
{
    st_job_in.tgt[0] = pfs[P_ST_CHORD] * 0.01f;
    st_job_in.tgt[1] = pfs[P_ST_TONE] * 0.01f;
    st_job_in.tgt[2] = pfs[P_ST_LENGTH] * 0.01f;
    st_job_in.tgt[3] = pfs[P_ST_HOLLOW] * 0.005f;   /* the nut .. the middle */
}

int dr_st_job_on(void)                     /* the platform asks, once a block: a job this block? */
{
    return dr_st_avail && (par[P_ST_MIX] > 0 || st_ramp > 0.0f);
}

static void st_block_start(void)           /* the block's start: on the host, the job here */
{
    st_two = 0;
    if (dr_st_wait) {
        st_two = dr_st_two;
        dr_st_two = 0;
    } else if (dr_st_job_on()) {
        if (!dr_st_late)
            dr_st_job(0);
        st_two = 2;
    }
}

static void st_off(void)                   /* (CPU0, no job out) silent: the next time on starts from still strings */
{
    st_ramp = st_lvl_s = st_hold = 0.0f;
    st_job_in.k = 0;
    st_job_in.clear = ++st_clears;
}

static void st_mix(float *l, float *r, uint32_t k)   /* after the PLAY fade: the strings in, the drones out to them */
{
    const float rate = 1.0f / (ST_FADE_S * DR_SR);
    const int on = par[P_ST_MIX] > 0;
    float lvl;
    uint32_t j, n;
    if (!st_two) {                         /* no job: off, or the second core gone (they stop at once) */
        if (st_ramp > 0.0f)
            st_off();
        return;
    }
    if (st_two == 2 && dr_st_late)
        dr_st_job(0);
    if (st_two == 1 && dr_st_wait() < 0) { /* the second core retired: the strings stop */
        st_off();
        return;
    }
    n = st_job_in.k < k ? st_job_in.k : k;
    if (dr_st_poison) {                    /* (a debug query: the check tried) */
        const union { uint32_t u; float f; } nan = { 0x7FC00000u };
        st_out[0][n ? n - 1u : 0u] = nan.f;
        dr_st_poison = 0;
    }
    for (j = 0; j < n; j++)                /* a misread in the data itself: never into the delay or the reverb */
        if (!(fm_fabsf(st_out[0][j]) <= ST_OUT_MAX && fm_fabsf(st_out[1][j]) <= ST_OUT_MAX)) {
            dr_st_failed = 1;
            dr_st_avail = 0;
            st_off();
            return;
        }
    if (on)
        st_hold = st_level();              /* (Mix at 0: the last level, washing out) */
    lvl = st_hold;
    for (j = 0; j < k; j++) {
        float g;
        st_job_in.in[j] = 0.5f * (l[j] + r[j]);
        st_ramp = on ? fm_minf(st_ramp + rate, 1.0f) : fm_maxf(st_ramp - rate, 0.0f);
        st_lvl_s += 0.001f * (lvl - st_lvl_s);
        g = st_ramp * st_ramp * st_lvl_s;
        if (j < n) {
            l[j] += st_out[0][j] * g;
            r[j] += st_out[1][j] * g;
        }
    }
    if (!on && st_ramp <= 0.0f) {          /* faded out: no more jobs */
        st_off();
        return;
    }
    st_job_in.k = k;
    st_job_in.note = grp[0].ps2;
    st_job_in.f5 = dr_tun_over(0, 7.0f, 702);   /* the Tuning's fifth over drone I's set note (12-tone: 7.00) */
    st_publish_knobs();
}

/* THE PICTURE's (the UI's main loop, read-only; a value read mid-write only moves a line for a frame) */
float dr_st_heard(void) { return st_ramp * st_ramp * st_lvl_s; }   /* the strings' level as heard, 0 .. ~4 */
void dr_st_semis(float *dst)               /* each string's note, semitones over drone I, gliding with Chord */
{                                          /* (Rings' lowest choice is its tonic, MIDI 0, not drone I) */
    const float note = st_job_in.note;
    int i;
    st_notes(note, st_job_in.f5, ST_STRUCT, dst);
    for (i = 0; i < (int)ST_N; i++)
        dst[i] -= note;
}
const int16_t *dr_st_line(int i, uint32_t *w, float *len)   /* string i's line, where it writes next, one cycle's
                                            * length in it (samples) */
{
    const st_str_t *s = &st_js.s[(uint32_t)i & (ST_N - 1u)];
    *w = s->w & (ST_LINE - 1u);
    *len = s->delay;
    return st_line[(uint32_t)i & (ST_N - 1u)];
}
