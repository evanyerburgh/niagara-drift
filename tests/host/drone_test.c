/* SPDX-License-Identifier: GPL-3.0-only */
/* The drone engine (dsp/drone.c) on its own: silence until PLAY, the fade, the pitch of a note, the
 * glide, voice on / off, panic, and an output that stays finite and inside full scale with the filter
 * self-oscillating and driven. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "drone.h"

#define FS 8388608.0f
static int bad;

static void check(const char *what, int ok)
{
    printf("%-58s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

static int32_t buf[2 * 256];

/* the engine as it boots, but dry (no reverb, no delay): the drones' own pitch and level are measured;
 * the effects have their own checks below */
static void dry_init(void)
{
    dr_init();
    dr_set(P_REV_MIX, 0);
    dr_set(P_DLY_MIX, 0);
    dr_set(P_MORPH, 0);                             /* switches at once (20 ms); the Morph has its own checks */
    dr_set(DR_GP(0, GP_FLFO_DEPTH), 0);             /* the drones still (the default breathes a little) */
    dr_set(DR_GP(1, GP_FLFO_DEPTH), 0);
}
static float peak, sumsq;
static long frames, upcross, nonzero;
static int finite_ok = 1;
static int32_t last_l;

/* render n blocks of 256 frames at MASTER 4/5 (the clean top), gathering peak, RMS, rising zero crossings (left) */
static void run(int blocks)
{
    int b, i;
    peak = sumsq = 0.0f;
    frames = upcross = nonzero = 0;
    for (b = 0; b < blocks; b++) {
        dr_render(buf, 256, 3277);
        for (i = 0; i < 256; i++) {
            float l = (float)buf[2 * i] / FS, r = (float)buf[2 * i + 1] / FS;
            if (!(l == l) || !(r == r))
                finite_ok = 0;
            peak = fmaxf(peak, fmaxf(fabsf(l), fabsf(r)));
            sumsq += l * l + r * r;
            nonzero += buf[2 * i] != 0 || buf[2 * i + 1] != 0;
            upcross += last_l < 0 && buf[2 * i] >= 0;
            last_l = buf[2 * i];
            frames++;
        }
    }
    for (i = 0; i < DR_NGROUPS; i++)
        if (!(dr_level[i] == dr_level[i]) || dr_level[i] > 100.0f)
            finite_ok = 0;
}
static float rms(void) { return sqrtf(sumsq / (2.0f * (float)frames)); }
static int secs(float s) { return (int)(s * DR_SR / 256.0f + 0.5f); }

/* n frames of one channel (0 left, 1 right) into out, as floats */
static void capture(float *out, int n, int ch)
{
    int k = 0, i;
    while (k < n) {
        dr_render(buf, 256, 3277);
        for (i = 0; i < 256 && k < n; i++)
            out[k++] = (float)buf[2 * i + ch] / FS;
    }
}
static float rms_of(const float *x, int n)
{
    float s = 0.0f;
    int i;
    for (i = 0; i < n; i++)
        s += x[i] * x[i];
    return sqrtf(s / (float)n);
}
/* the period, in samples, between lo and hi: the lag of the strongest autocorrelation */
static int period(const float *x, int n, int lo, int hi)
{
    int lag, best = lo;
    float bv = -1e30f;
    for (lag = lo; lag <= hi; lag++) {
        float r = 0.0f;
        int i;
        for (i = 0; i + lag < n; i++)
            r += x[i] * x[i + lag];
        r /= (float)(n - lag);
        if (r > bv) {
            bv = r;
            best = lag;
        }
    }
    return best;
}

/* the level of the f Hz component in x (n frames): a single-bin DFT */
static float tone_level(const float *x, int n, float f)
{
    double re = 0.0, im = 0.0, w = 2.0 * 3.14159265358979 * (double)f / (double)DR_SR;
    int i;
    for (i = 0; i < n; i++) {
        re += (double)x[i] * cos(w * (double)i);
        im += (double)x[i] * sin(w * (double)i);
    }
    return (float)(2.0 * sqrt(re * re + im * im) / (double)n);
}

/* the power around f (+-20 %, 41 single bins), in dB: a band of a noise */
static float band_db(const float *x, int n, float f)
{
    float s = 0.0f;
    int k;
    for (k = -20; k <= 20; k++) {
        float a = tone_level(x, n, f * (1.0f + 0.01f * (float)k));
        s += a * a;
    }
    return 10.0f * log10f(s / 41.0f + 1e-20f);
}

/* drone I as a plain backdrop for its noise (a sine at 27 Hz, no partner, no sub: its overtones the soft
 * clip makes, 82 and 137 Hz, are under where the noise's slope is measured), the filter open; noise
 * nv of colour c; drone II silent */
static void noise_setup(int c, int nv)
{
    dry_init();
    dr_set(P_FADE, 0);
    dr_set(P_DRIFT, 0);
    dr_set(P_WIDTH, 0);
    dr_set(DR_GP(0, GP_PARTNER), 0);
    dr_set(DR_GP(0, GP_WAVE), 0);
    dr_set(DR_GP(0, GP_PITCH), DR_PITCH_LO);
    dr_set(DR_GP(0, GP_MIX), 0);
    dr_set(DR_GP(0, GP_CUTOFF), 1000);
    dr_set(DR_GP(0, GP_RESO), 0);
    dr_set(DR_GP(0, GP_DRIVE), 0);
    dr_set(DR_GP(0, GP_NOISE), nv);
    dr_set(DR_GP(0, GP_NCOLOR), c);
    dr_set(DR_GP(1, GP_LEVEL), 0);
    dr_play(1);
}
/* the noise alone: the same render with it and without it (every init sounds the same), subtracted; ch 0
 * left, 1 right. setup() is called twice, its noise then set to nv and to 0 */
static float nz_tmp[32768];
static void noise_only(void (*setup)(void), int nv, float *x, int n, int ch)
{
    int i;
    setup();
    dr_set(DR_GP(0, GP_NOISE), nv);
    dr_set(DR_GP(1, GP_NOISE), nv);
    run(secs(0.5f));
    capture(x, n, ch);
    setup();
    dr_set(DR_GP(0, GP_NOISE), 0);
    dr_set(DR_GP(1, GP_NOISE), 0);
    run(secs(0.5f));
    capture(nz_tmp, n, ch);
    for (i = 0; i < n; i++)
        x[i] -= nz_tmp[i];
}
static int nz_c;
static void nz_backdrop(void) { noise_setup(nz_c, 0); }
static void nz_saw(void)                         /* drone I's saw at A3, its partner off */
{
    noise_setup(1, 0);
    dr_set(DR_GP(0, GP_WAVE), 67);
    dr_set(DR_GP(0, GP_PITCH), 5700);
}
static void nz_both(void)                        /* both drones, apart (I left, II right) */
{
    noise_setup(1, 0);
    dr_set(DR_GP(1, GP_LEVEL), 80);
    dr_set(DR_GP(1, GP_CUTOFF), 1000);
    dr_set(DR_GP(1, GP_RESO), 0);
    dr_set(P_WIDTH, 100);
}

/* voice 1 alone, a plain triangle without its partner, the filter open: its fundamental is all there is */
static void pure_tone(void)
{
    dry_init();
    dr_set(P_FADE, 0);
    dr_set(P_DRIFT, 0);
    dr_set(DR_GP(0, GP_PARTNER), 0);
    dr_set(P_WIDTH, 0);
    dr_set(P_GLIDE, 0);
    dr_set(DR_GP(0, GP_WAVE), 33);                  /* X-WAVE: the triangle */
    dr_set(DR_GP(0, GP_MIX), 0);
    dr_set(DR_GP(0, GP_CUTOFF), 1000);
    dr_set(DR_GP(0, GP_RESO), 0);
    dr_set(DR_GP(0, GP_DRIVE), 0);
    dr_set(DR_GP(1, GP_LEVEL), 0);
    dr_play(1);
}

int main(void)
{
    int i;

    dry_init();
    run(secs(1.0f));
    check("silent before PLAY", nonzero == 0);

    dr_play(1);                                     /* default fade: 1 s */
    run(secs(0.1f));
    {
        float early = rms();
        run(secs(1.5f));
        run(secs(0.5f));
        check("fades in (RMS at 0.1 s well under RMS at 2 s)", early < 0.3f * rms() && rms() > 0.02f);
        printf("    default scene: peak %.3f, RMS %.3f\n", (double)peak, (double)rms());
        check("default scene stays under full scale", peak < 0.99f);
    }
    dr_play(0);
    run(secs(1.2f));
    run(secs(0.2f));
    check("fades out to silence", nonzero == 0);

    pure_tone();
    dr_set(DR_GP(0, GP_PITCH), 5700);               /* A3, 220 Hz */
    run(secs(0.5f));
    run(secs(2.0f));
    {
        float hz = (float)upcross / ((float)frames / DR_SR);
        printf("    A3: %.2f Hz\n", (double)hz);
        check("note 57 sounds at 220 Hz (+-1)", fabsf(hz - 220.0f) < 1.0f);
    }
    dr_set(P_TUNE, 50);
    run(secs(0.2f));
    run(secs(2.0f));
    {
        float hz = (float)upcross / ((float)frames / DR_SR), want = 220.0f * powf(2.0f, 50.0f / 1200.0f);
        check("Tune +50 cents", fabsf(hz - want) < 1.0f);
    }
    dr_set(P_TUNE, 0);
    dr_set(DR_GP(0, GP_PITCH), 5737);               /* between notes: A3 + 37 cents */
    run(secs(0.2f));
    run(secs(2.0f));
    {
        float hz = (float)upcross / ((float)frames / DR_SR), want = 220.0f * powf(2.0f, 37.0f / 1200.0f);
        printf("    A3 +37 cents: %.2f Hz (want %.2f)\n", (double)hz, (double)want);
        check("pitch between notes (A3 +37 cents)", fabsf(hz - want) < 1.0f);
    }
    {   /* the pitch range's ends: 27 Hz and 1.6 kHz, the DS-2's oscillator range */
        float lo, hi;
        dr_set(DR_GP(0, GP_PITCH), DR_PITCH_LO);
        run(secs(0.3f));
        run(secs(4.0f));
        lo = (float)upcross / ((float)frames / DR_SR);
        dr_set(DR_GP(0, GP_PITCH), DR_PITCH_HI);
        run(secs(0.3f));
        run(secs(1.0f));
        hi = (float)upcross / ((float)frames / DR_SR);
        printf("    pitch range: %.2f Hz .. %.1f Hz\n", (double)lo, (double)hi);
        check("pitch range: 27 Hz at the bottom, 1.6 kHz at the top", fabsf(lo - 27.0f) < 0.3f && fabsf(hi - 1600.0f) < 3.0f);
    }
    {   /* along the X-WAVE, each wave and the blends between sound at a similar level */
        float lo = 1.0f, hi = 0.0f;
        int w;
        dr_set(DR_GP(0, GP_PITCH), 4500);
        for (w = 0; w <= 100; w += 17) {
            dr_set(DR_GP(0, GP_WAVE), w);
            run(secs(0.2f));
            run(secs(0.5f));
            printf("    X-WAVE %d: RMS %.3f\n", w, (double)rms());
            lo = fminf(lo, rms());
            hi = fmaxf(hi, rms());
        }
        check("the X-WAVE sounds all along, within 3 dB (blends don't cancel)", finite_ok && lo > 0.05f && hi < 1.41f * lo);
        dr_set(DR_GP(0, GP_WAVE), 33);
    }
    {   /* OSC MIX at the sub's end: the square an octave down, alone */
        dr_set(DR_GP(0, GP_PITCH), 5700);
        dr_set(DR_GP(0, GP_MIX), 100);
        run(secs(0.3f));
        run(secs(2.0f));
        {
            float hz = (float)upcross / ((float)frames / DR_SR);
            printf("    OSC MIX 100 on A3: %.2f Hz\n", (double)hz);
            check("OSC MIX 100: the sub alone, an octave down (110 Hz)", fabsf(hz - 110.0f) < 1.0f);
        }
        dr_set(DR_GP(0, GP_MIX), 0);
    }
    {   /* the LFO: its rate, and every shape of both taps inside -1..1 and moving */
        int sh, tap, bad_shape = 0;
        float want = 0.02f * powf(3500.0f, 0.6f), hz;
        long ups = 0;
        float last;
        dr_set(P_LFO_RATE, 60);                     /* 0.02 * 3500^0.6 = 2.7 Hz */
        dr_set(P_LFO_W1, 1);
        run(1);
        last = dr_lfo[0];
        for (i = 0; i < secs(4.0f); i++) {
            run(1);
            ups += last < 0.0f && dr_lfo[0] >= 0.0f;
            last = dr_lfo[0];
        }
        hz = (float)ups / 4.0f;
        printf("    LFO rate 60: %.2f Hz (want %.2f)\n", (double)hz, (double)want);
        check("LFO rate (Wave I triangle at 2.7 Hz)", fabsf(hz - want) < 0.3f);
        for (tap = 0; tap < 2; tap++)
            for (sh = 1; sh < DR_LFO_NSHAPES; sh++) {
                float mn = 9.0f, mx = -9.0f;
                dr_set(tap ? P_LFO_W2 : P_LFO_W1, sh);
                for (i = 0; i < secs(1.0f); i++) {
                    run(1);
                    mn = fminf(mn, dr_lfo[tap]);
                    mx = fmaxf(mx, dr_lfo[tap]);
                }
                if (mn < -1.001f || mx > 1.001f || mx - mn < 0.9f) {
                    printf("    tap %d shape %d: %.2f .. %.2f\n", tap + 1, sh, (double)mn, (double)mx);
                    bad_shape++;
                }
            }
        check("every LFO shape (both taps) inside -1..1, and moving", bad_shape == 0);
        dr_set(P_LFO_W1, 1);
        dr_set(P_LFO_W2, 1);
    }
    {   /* the LFO's shapes: each averages 0 over a cycle (picking one doesn't retune: Roundtooth sat at +0.61);
         * "2x Exp" two rises a cycle and "4x Exp" four, as the other 2x shapes (they ran at half); and a new shape
         * glides over the Morph, as a switch (it jumped; Triangle to 2x Tri: both smooth, apart at the switch) */
        int tap, sh, off = 0, ups[2] = {0, 0}, k;
        float jump = 0.0f, last;
        for (tap = 0; tap < 2; tap++)
            for (sh = 1; sh < DR_LFO_NSHAPES; sh++) {
                double m = 0.0;
                if (sh == DR_LFO_RANDOM)
                    continue;
                for (k = 0; k < 4000; k++)
                    m += (double)dr_lfo_wave(tap, sh, ((float)k + 0.5f) / 4000.0f);
                if (fabs(m / 4000.0) > 0.02) {
                    printf("    tap %d shape %d averages %.3f\n", tap + 1, sh, m / 4000.0);
                    off++;
                }
            }
        check("every LFO shape averages 0 over a cycle (within 0.02)", off == 0);
        for (tap = 0; tap < 2; tap++) {
            float prev = dr_lfo_wave(tap, tap ? 7 : 6, 0.0f);
            for (k = 1; k <= 4000; k++) {
                float v = dr_lfo_wave(tap, tap ? 7 : 6, (float)k / 4000.0f - 1e-6f);
                ups[tap] += prev < 0.0f && v >= 0.0f;
                prev = v;
            }
        }
        printf("    rises a cycle: 2x Exp %d, 4x Exp %d\n", ups[0], ups[1]);
        check("2x Exp rises twice a cycle, 4x Exp four times", ups[0] == 2 && ups[1] == 4);
        dr_set(P_LFO_RATE, 45);
        dr_set(P_LFO_W1, 1);
        dr_set(P_MORPH, 47);                        /* 0.6 s, the default */
        run(secs(1.0f));
        last = dr_lfo[0];
        dr_set(P_LFO_W1, 5);                        /* Triangle to 2x Tri */
        for (i = 0; i < secs(1.0f); i++) {
            run(1);
            jump = fmaxf(jump, fabsf(dr_lfo[0] - last));
            last = dr_lfo[0];
        }
        printf("    Triangle to 2x Tri: biggest step a block %.3f\n", (double)jump);
        check("a new LFO shape glides over the Morph (no jump)", jump < 0.15f);
        dr_set(P_MORPH, 0);
        dr_set(P_LFO_W1, 1);
        run(secs(0.2f));
    }
    {   /* the LFO's timing: the slowest rate on time (a float phase lost ~2 %), and RATE MOD bending the shape
         * but keeping the tempo (it lingered where slowed: a cycle x1.9 at 100, x2.9 with both waves) */
        static const struct { int rm, src; } RM[4] = {{0, DR_LFO_I}, {100, DR_LFO_I}, {-100, DR_LFO_I}, {100, DR_LFO_BOTH}};
        float ph, want, cyc[4];
        int k, bad = 0;
        dr_set(P_LFO_RATE, 0);                      /* 0.02 Hz: a cycle in 50 s */
        run(secs(0.5f));                            /* the rate glides there (25 ms) */
        dr_lfo_reset();
        run(secs(40.0f));
        ph = dr_lfo_ph;
        want = 40.0f * 0.02f;
        printf("    the slowest LFO: %.4f of a cycle in 40 s (want %.4f)\n", (double)ph, (double)want);
        check("the slowest LFO rate on time (within 0.5 %)", fabsf(ph / want - 1.0f) < 0.005f);
        dr_set(P_LFO_RATE, 60);
        dr_set(P_LFO_W1, 1);
        dr_set(P_LFO_W2, 3);
        for (k = 0; k < 4; k++) {
            long wraps = 0;
            float last = 0.0f;
            dr_set(P_LFO_RMOD, RM[k].rm);
            dr_set(P_LFO_RMOD_SRC, RM[k].src);
            run(secs(0.5f));
            dr_lfo_reset();
            for (i = 0; i < secs(20.0f); i++) {
                run(1);
                wraps += dr_lfo_ph < last;
                last = dr_lfo_ph;
            }
            cyc[k] = ((float)wraps + dr_lfo_ph) / 20.0f;
            bad += fabsf(cyc[k] / cyc[0] - 1.0f) > 0.03f;
        }
        printf("    LFO cycles a second: plain %.3f, rate mod +100 %.3f, -100 %.3f, +100 both %.3f\n",
               (double)cyc[0], (double)cyc[1], (double)cyc[2], (double)cyc[3]);
        check("rate mod keeps the LFO's tempo (within 3 %)", bad == 0);
        dr_set(P_LFO_RMOD, 0);
        dr_set(P_LFO_RMOD_SRC, DR_LFO_I);
        dr_set(P_LFO_W2, 1);
    }
    {   /* RANDOM: locked to the Swell (a bar shorter than the LFO's cycle), it still draws a new level each bar
         * (it froze); a reset draws one too */
        float v = 9.0f;
        int n = 0;
        dr_set(P_LFO_W1, DR_LFO_RANDOM);
        dr_set(P_LFO_RATE, 0);                      /* a 50 s cycle */
        dr_set(P_PULSE, 1);
        dr_set(P_LFO_LOCK, 1);
        for (i = 0; i < 40; i++) {
            run(secs(1.0f));
            n += dr_lfo[0] != v;
            v = dr_lfo[0];
        }
        printf("    Random locked to the Swell: %d changes in 40 s\n", n);
        check("Random locked to the Swell keeps changing", n >= 5);
        dr_set(P_PULSE, 0);
        dr_set(P_LFO_LOCK, 0);
        v = dr_lfo[0];
        dr_lfo_reset();
        run(1);
        check("an LFO reset draws a new random level", dr_lfo[0] != v);
        dr_set(P_LFO_W1, 1);
        dr_set(P_LFO_RATE, 60);
    }
    {   /* the filter's LFO swings the level; the oscillator's (pitch) swings the pitch */
        float mn = 9.0f, mx = 0.0f;
        int k;
        dr_set(P_LFO_RATE, 45);                     /* ~0.8 Hz */
        dr_set(DR_GP(0, GP_PITCH), 4500);
        dr_set(DR_GP(0, GP_WAVE), 67);              /* a saw, so the cutoff matters */
        dr_set(DR_GP(0, GP_CUTOFF), 400);
        dr_set(DR_GP(0, GP_FLFO), DR_LFO_I);
        dr_set(DR_GP(0, GP_FLFO_DEPTH), 80);
        run(secs(0.5f));
        for (k = 0; k < 30; k++) {                  /* 30 windows of 0.1 s: 2.5 LFO cycles */
            run(secs(0.1f));
            mn = fminf(mn, rms());
            mx = fmaxf(mx, rms());
        }
        printf("    filter LFO: RMS %.3f .. %.3f\n", (double)mn, (double)mx);
        check("the filter LFO swings the sound (RMS x2 or more)", mx > 2.0f * mn);
        {   /* the filter's LFO on the resonance instead: the peak at the cutoff swells and falls. Measured on
             * the saw's 8th harmonic (880 Hz), right at the cutoff (55: ~840 Hz); depth 0: steady; depth 25
             * heard (linear on the resonance: squared, it swung the peak ~1.4 dB) */
            static float w[4410];
            static const int RD[3] = {0, 100, 25};
            float rmn[3], rmx[3];
            int r;
            dr_set(DR_GP(0, GP_FLFO_DEST), DR_TO_RESO);
            dr_set(DR_GP(0, GP_CUTOFF), 550);
            dr_set(DR_GP(0, GP_RESO), 500);
            for (r = 0; r < 3; r++) {
                dr_set(DR_GP(0, GP_FLFO_DEPTH), RD[r]);
                run(secs(0.5f));
                rmn[r] = 9.0f;
                rmx[r] = 0.0f;
                for (k = 0; k < 30; k++) {
                    float h;
                    capture(w, 4410, 0);
                    h = tone_level(w, 4410, 880.0f);
                    rmn[r] = fminf(rmn[r], h);
                    rmx[r] = fmaxf(rmx[r], h);
                }
            }
            printf("    filter LFO on the resonance: 880 Hz at %.4f .. %.4f (depth 0: %.4f .. %.4f; 25: %.4f .. %.4f)\n",
                   (double)rmn[1], (double)rmx[1], (double)rmn[0], (double)rmx[0], (double)rmn[2], (double)rmx[2]);
            check("the filter LFO can move the resonance (the peak swings x2; steady at depth 0)",
                  rmx[1] > 2.0f * rmn[1] && rmx[0] < 1.1f * rmn[0] && finite_ok);
            check("the resonance LFO heard at depth 25 (the peak swings 3 dB or more)", rmx[2] > 1.41f * rmn[2]);
            dr_set(DR_GP(0, GP_FLFO_DEST), DR_TO_CUTOFF);
            dr_set(DR_GP(0, GP_RESO), 250);
        }
        dr_set(DR_GP(0, GP_FLFO_DEPTH), 0);
        dr_set(DR_GP(0, GP_WAVE), 33);
        dr_set(DR_GP(0, GP_CUTOFF), 1000);
        dr_set(DR_GP(0, GP_OLFO), DR_LFO_I);
        dr_set(DR_GP(0, GP_OLFO_DEPTH), 30);        /* 0.3^2 x 24 = +-2.2 semitones */
        mn = 9e9f;
        mx = 0.0f;
        run(secs(0.5f));
        dr_lfo_reset();                             /* from the cycle's start: not wherever the tests before left it */
        for (k = 0; k < 15; k++) {                  /* 15 windows of 0.2 s */
            float hz;
            run(secs(0.2f));
            hz = (float)upcross / ((float)frames / DR_SR);
            mn = fminf(mn, hz);
            mx = fmaxf(mx, hz);
        }
        printf("    pitch LFO: %.1f .. %.1f Hz\n", (double)mn, (double)mx);
        check("the pitch LFO swings the pitch (about +-2 semitones)", mn < 110.0f * 0.92f && mx > 110.0f * 1.08f);
        {   /* both waves to one route, summed: Wave I a square, Wave II a square a quarter later; each
             * alone +-2.2 semitones, together they reach +-4.4 (on one step in two) */
            float both = 0.0f, one = 0.0f;
            int r;
            dr_set(P_LFO_W1, 2);
            dr_set(P_LFO_W2, 2);
            for (r = 0; r < 2; r++) {
                dr_set(DR_GP(0, GP_OLFO), r ? DR_LFO_BOTH : DR_LFO_I);
                dr_lfo_reset();                     /* the same start each time; windows short enough */
                run(secs(0.1f));                    /* to fit inside a quarter cycle (0.3 s) */
                mx = 0.0f;
                for (k = 0; k < 30; k++) {          /* 0.1 s windows, each pitch from its period */
                    static float w[4410];
                    capture(w, 4410, 0);
                    mx = fmaxf(mx, DR_SR / (float)period(w, 4410, 200, 500));
                }
                *(r ? &both : &one) = mx;
            }
            printf("    LFO to pitch, highest: Wave I %.1f Hz, I+II %.1f Hz\n", (double)one, (double)both);
            check("both LFO waves on one route: summed (I+II twice the reach of I)",
                  one < 110.0f * 1.16f && both > 110.0f * 1.25f);
            dr_set(DR_GP(0, GP_OLFO), DR_LFO_I);
            dr_set(P_LFO_W1, 1);
            dr_set(P_LFO_W2, 1);
        }
        dr_set(DR_GP(0, GP_OLFO_DEPTH), 0);
    }

    {   /* a Level snapped from 100 to 0 fades in a few ms: no step (a click) bigger than the wave's own */
        int32_t prev = 0;
        float steady = 0.0f, snap = 0.0f;
        int j;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);                 /* the sine */
        dr_set(DR_GP(0, GP_PITCH), 3300);             /* A1, 55 Hz: a slow wave, so a click would stand out */
        dr_set(DR_GP(0, GP_LEVEL), 100);
        run(secs(0.5f));
        for (j = 0; j < 40; j++) {
            int k;
            if (j == 20)
                dr_set(DR_GP(0, GP_LEVEL), 0);
            dr_render(buf, 256, 3277);
            for (k = 0; k < 256; k++) {
                float d = fabsf((float)(buf[2 * k] - prev)) / FS;
                prev = buf[2 * k];
                if (j > 0 && j < 20)
                    steady = fmaxf(steady, d);
                else if (j >= 20)
                    snap = fmaxf(snap, d);
            }
        }
        printf("    level snap: largest step %.4f (the sine's own: %.4f)\n", (double)snap, (double)steady);
        check("a Level change doesn't click", snap <= 1.05f * steady);
    }

    pure_tone();                                    /* a quick knob turn: a semitone every ~17 ms */
    dr_set(DR_GP(0, GP_PITCH), 4500);
    run(secs(0.3f));
    {
        float last = dr_pitch[0], dmin = 1e9f, dsum = 0.0f;
        int i, n = 0;
        for (i = 0; i < 90; i++) {
            if (i % 3 == 0 && i < 72) {             /* as the UI sends it: slide over the time to the next */
                dr_slide(0, 17);
                dr_set(DR_GP(0, GP_PITCH), 4500 + 100 * (i / 3 + 1));
            }
            run(1);
            if (i >= 12 && i < 66) {                /* the steady part of the sweep */
                float d = dr_pitch[0] - last;
                dmin = fminf(dmin, d);
                dsum += d;
                n++;
            }
            last = dr_pitch[0];
        }
        printf("    quick sweep: slowest block %.0f %% of the average speed; ends at %.2f\n",
               (double)(100.0f * dmin / (dsum / (float)n)), (double)dr_pitch[0]);
        check("a quick sweep is continuous (never under 60 % of its speed)", dmin > 0.6f * dsum / (float)n);
        run(secs(0.3f));
        check("... and lands exactly", fabsf(dr_pitch[0] - 69.0f) < 0.01f);
    }
    {   /* a slow turn: 60 cents every 150 ms, each sliding over the 150 ms (26 blocks) */
        float last, dmin = 1e9f, dsum = 0.0f;
        int i, n = 0;
        dr_set(DR_GP(0, GP_PITCH), 4500);
        run(secs(0.3f));
        last = dr_pitch[0];
        for (i = 0; i < 26 * 12; i++) {
            if (i % 26 == 0 && i < 26 * 10) {
                dr_slide(0, 150);
                dr_set(DR_GP(0, GP_PITCH), 4500 + 60 * (i / 26 + 1));
            }
            run(1);
            if (i >= 26 && i < 26 * 9) {
                float d = dr_pitch[0] - last;
                dmin = fminf(dmin, d);
                dsum += d;
                n++;
            }
            last = dr_pitch[0];
        }
        printf("    slow turn: slowest block %.0f %% of the average speed\n", (double)(100.0f * dmin / (dsum / (float)n)));
        check("a slow turn of 60-cent detents slides, never stops", dmin > 0.6f * dsum / (float)n);
        check("... and lands exactly", fabsf(dr_pitch[0] - 51.0f) < 0.01f);
    }

    pure_tone();
    dr_set(DR_GP(0, GP_PITCH), 5700);
    run(secs(0.2f));
    dr_set(P_GLIDE, 50);                            /* 2.5 s */
    dr_set(DR_GP(0, GP_PITCH), 6900);
    run(secs(0.5f));
    check("glide: on the way after 0.5 s", dr_pitch[0] > 58.0f && dr_pitch[0] < 67.0f);
    run(secs(2.5f));
    check("glide: there after 3 s", dr_pitch[0] > 68.8f);

    dry_init();
    dr_set(P_FADE, 0);
    dr_play(1);
    run(secs(0.5f));
    for (i = 0; i < DR_NGROUPS; i++)
        dr_voice(i, 0);
    run(secs(0.2f));
    run(secs(0.2f));
    check("both drones off: silence", nonzero == 0);
    dr_voice(1, 1);
    run(secs(0.3f));
    check("one drone back: sound, from it alone", rms() > 0.01f && dr_level[0] == 0.0f && dr_level[1] > 0.0f);

    {   /* the links. Drone I left, drone II right (Width 100), so the right channel is drone II alone */
        static float cap[8192];
        int lag;
        dry_init();
        dr_set(P_FADE, 0);
        dr_set(P_DRIFT, 0);
        dr_set(P_WIDTH, 100);
        for (i = 0; i < DR_NGROUPS; i++) {
            dr_set(DR_GP(i, GP_PARTNER), 0);
            dr_set(DR_GP(i, GP_MIX), 0);
            dr_set(DR_GP(i, GP_DRIVE), 0);
            dr_set(DR_GP(i, GP_RESO), 0);
        }
        dr_set(DR_GP(0, GP_PITCH), 4500);               /* drone I: A2, 110 Hz */
        dr_set(DR_GP(1, GP_PITCH), 5000);               /* drone II: D3, 146.8 Hz, a saw, the filter open */
        dr_set(DR_GP(1, GP_WAVE), 67);
        dr_set(DR_GP(1, GP_CUTOFF), 1000);
        dr_play(1);
        run(secs(0.5f));
        capture(cap, 8192, 1);
        lag = period(cap, 8192, 150, 550);
        printf("    drone II alone: period %d samples (%.1f Hz)\n", lag, (double)(DR_SR / (float)lag));
        check("hard sync off: drone II at its own pitch (146.8 Hz)", abs(lag - 300) <= 2);
        dr_set(P_SYNC, 1);
        run(secs(0.2f));
        capture(cap, 8192, 1);
        lag = period(cap, 8192, 150, 550);
        printf("    hard sync: period %d samples (%.1f Hz)\n", lag, (double)(DR_SR / (float)lag));
        check("hard sync on: drone II repeats at drone I's pitch (110 Hz)", abs(lag - 401) <= 2);
        dr_set(P_SYNC, 0);

        dr_set(P_PLINK_OFF, 500);                       /* pitch link: II = I + 5 semitones */
        dr_set(P_PLINK, 1);
        run(secs(0.3f));
        dr_slide(0, 100);
        dr_set(DR_GP(0, GP_PITCH), 4800);               /* move drone I: drone II follows */
        run(secs(0.5f));
        printf("    pitch link: I %.2f, II %.2f\n", (double)dr_pitch[0], (double)dr_pitch[1]);
        check("pitch link: drone II follows drone I, 5 semitones up", fabsf(dr_pitch[0] - 48.0f) < 0.01f &&
                                                                     fabsf(dr_pitch[1] - 53.0f) < 0.02f);
        dr_set(P_PLINK_OFF, 1200);
        run(secs(0.3f));
        check("pitch link: drone II's knob moves the offset (an octave)", fabsf(dr_pitch[1] - 60.0f) < 0.02f);
        dr_set(P_PLINK, 0);

        {   /* cross-mod: drone I's sound opening drone II's low filter, at audio rate */
            float off, on;
            dr_set(DR_GP(1, GP_CUTOFF), 300);
            run(secs(0.5f));
            capture(cap, 8192, 1);
            off = rms_of(cap, 8192);
            dr_set(P_XMOD, 1);
            dr_set(P_XMOD_AMT, 80);
            run(secs(0.5f));
            capture(cap, 8192, 1);
            on = rms_of(cap, 8192);
            printf("    cross-mod: drone II RMS %.3f off, %.3f on\n", (double)off, (double)on);
            check("cross-mod: drone I opens drone II's filter (RMS up by half or more)", on > 1.5f * off &&
                                                                                        finite_ok);
        }
    }

    {   /* the PULSE: its rate (gate openings a second), its depth, its swing (beats 3:1), and off again */
        int opens = 0, k, last_open = -1, lens[16], nl = 0, was = 1;
        float mn = 9.0f;
        static int32_t small[2 * 32];
        dry_init();
        dr_set(P_FADE, 0);
        dr_set(P_PULSE, 1);
        dr_set(P_PULSE_RATE, 67);                       /* 0.03 * 533^0.67 = 2 a second */
        dr_set(P_PULSE_DEPTH, 100);
        dr_set(P_PULSE_SHAPE, 100);                     /* a hard gate */
        dr_play(1);
        for (k = 0; k < (int)(4.0f * DR_SR / 32.0f); k++) {   /* 4 s in 32-frame steps */
            int open;
            dr_render(small, 32, 3277);
            open = dr_pulse > 0.5f;
            if (open && !was)
                opens++;
            was = open;
            if (k > 1000)
                mn = fminf(mn, dr_pulse);
        }
        printf("    pulse at 2/s: %d openings in 4 s, gain down to %.3f\n", opens, (double)mn);
        check("pulse: 2 a second (8 openings in 4 s, +-1)", opens >= 7 && opens <= 9);
        check("pulse: depth 100, a hard gate: closes to silence", mn < 0.01f);
        dr_set(P_PULSE_SWING, 50);                      /* beats 1.5 and 0.5 long: openings 0.75 s, 0.25 s apart */
        was = 1;
        for (k = 0; k < (int)(6.0f * DR_SR / 32.0f); k++) {
            int open;
            dr_render(small, 32, 3277);
            open = dr_pulse > 0.5f;
            if (open && !was) {
                if (last_open >= 0 && nl < 16)
                    lens[nl++] = k - last_open;
                last_open = k;
            }
            was = open;
        }
        {
            int lo = 99999, hi = 0;
            for (k = 1; k < nl; k++) {                  /* the first may straddle the change */
                lo = lens[k] < lo ? lens[k] : lo;
                hi = lens[k] > hi ? lens[k] : hi;
            }
            /* the rate as set (67: 0.03 x 533^0.67, 2.01 a second): a two-beat cycle split 1:3 */
            float cyc = 2.0f / (0.03f * powf(2.0f, 67.0f * 0.0905875f)) * DR_SR / 32.0f;
            printf("    pulse swing 50: openings %d and %d steps apart (3:1 is %.0f, %.0f)\n", lo, hi,
                   (double)(0.25f * cyc), (double)(0.75f * cyc));
            check("pulse swing 50: beats alternate 1:3 (0.25 / 0.75 of the cycle)",
                  abs(lo - (int)(0.25f * cyc + 0.5f)) <= 4 && abs(hi - (int)(0.75f * cyc + 0.5f)) <= 4);
        }
        dr_set(P_PULSE, 0);
        run(secs(0.1f));
        check("pulse off: open again", dr_pulse > 0.999f && finite_ok);
    }

    {   /* GLIDE follows a knob's slide: with Glide up, a pitch knob's turn (dr_slide, as the UI sends it) glides
         * there (the slide replaced the Glide: knob turns arrived at once at any Glide) */
        float p0, early, late;
        pure_tone();
        dr_set(P_GLIDE, 50);                        /* 2.5 s */
        run(secs(1.0f));
        p0 = dr_pitch[0];
        dr_slide(0, 60);
        dr_set(DR_GP(0, GP_PITCH), (int)(p0 * 100.0f + 0.5f) + 300);
        run(secs(0.3f));
        early = dr_pitch[0] - p0;
        run(secs(4.0f));
        late = dr_pitch[0] - p0;
        printf("    Glide 2.5 s, a knob turn of 3 semitones: %.2f at 0.3 s, %.2f at 4.3 s\n", (double)early, (double)late);
        check("Glide smooths a pitch knob's turn (not there at 0.3 s, there by 4.3 s)", early < 1.5f && late > 2.95f);
        dr_set(P_GLIDE, 0);
    }

    {   /* the EFFECTS. Drone I alone (a saw, filter open), into the effects only (wet), in short bursts */
        static float cap[44100];
        int k, t, first;
        float ms_want[3] = {70.0f, 177.0f, 450.0f};
        int ms_v[3] = {0, 50, 100};
        for (t = 0; t < 3; t++) {                       /* the delay's time: the first echo, on time */
            dry_init();
            dr_set(P_FADE, 0);
            dr_set(DR_GP(1, GP_LEVEL), 0);
            dr_set(DR_GP(0, GP_WAVE), 67);
            dr_set(P_DLY_MIX, 100);
            dr_set(P_DLY_REPEATS, 0);
            dr_set(P_DLY_TIME, ms_v[t]);
            run(secs(0.5f));                            /* stopped: the time settles */
            dr_play(1);
            capture(cap, 30000, 0);
            for (first = 0; first < 30000 && fabsf(cap[first]) < 1e-4f; first++)
                ;
            printf("    delay %d: first echo after %.1f ms (want %.0f)\n", ms_v[t], (double)((float)first * 1000.0f / DR_SR),
                   (double)ms_want[t]);
            check(t == 0 ? "delay: its time (70 ms at 0)" : t == 1 ? "delay: its time (177 ms at 50)" : "delay: its time (450 ms at 100)",
                  fabsf((float)first * 1000.0f / DR_SR - ms_want[t]) < 2.0f);
        }
        {   /* repeats: a 50 ms burst into a 100 ms delay; echo k at k x 100 ms */
            float e[12];
            int rep;
            for (rep = 0; rep < 2; rep++) {
                dry_init();
                dr_set(P_FADE, 0);
                dr_set(DR_GP(1, GP_LEVEL), 0);
                dr_set(DR_GP(0, GP_WAVE), 67);
                dr_set(DR_GP(0, GP_CUTOFF), 1000);
                dr_set(P_DLY_MIX, 100);
                dr_set(P_DLY_TIME, 19);                 /* 99.7 ms */
                dr_set(P_DLY_REPEATS, rep ? 100 : 50);
                run(secs(0.3f));
                dr_play(1);
                run((int)(0.05f * DR_SR / 256.0f));     /* ~50 ms of sound */
                dr_play(0);                             /* (Fade 0: 50 ms out) */
                capture(cap, 44100, 0);
                for (k = 0; k < 12; k++) {              /* each echo's RMS, a 40 ms window on it */
                    int c = (int)((float)(k + 1) * 0.0997f * DR_SR) - (int)(0.05f * DR_SR) + 1500;
                    e[k] = c + 1764 < 44100 ? rms_of(cap + c, 1764) : 0.0f;
                }
                if (!rep) {
                    printf("    repeats 50: echoes %.3f %.3f %.3f %.3f\n", (double)e[0], (double)e[1], (double)e[2], (double)e[3]);
                    check("delay repeats 50: each echo quieter", e[1] < 0.8f * e[0] && e[2] < 0.8f * e[1] && e[3] < 0.8f * e[2]);
                    {   /* and darker: fewer zero crossings a second in the 4th echo than the 1st */
                        int c1 = (int)(0.0997f * DR_SR) - (int)(0.05f * DR_SR) + 1500, c4 = c1 + (int)(3.0f * 0.0997f * DR_SR);
                        int z1 = 0, z4 = 0, i;
                        for (i = 1; i < 1764; i++) {
                            z1 += (cap[c1 + i - 1] < 0.0f) != (cap[c1 + i] < 0.0f);
                            z4 += (cap[c4 + i - 1] < 0.0f) != (cap[c4 + i] < 0.0f);
                        }
                        printf("    darker each time: zero crossings %d (echo 1), %d (echo 4)\n", z1, z4);
                        check("delay: each repeat darker", z4 < z1);
                    }
                } else {
                    printf("    repeats 100: echo 1 %.3f, echo 8 %.3f\n", (double)e[0], (double)e[7]);
                    check("delay repeats 100: it runs on (echo 8 still half of echo 1 or more), finite",
                          e[7] > 0.5f * e[0] && finite_ok);
                }
            }
        }
        {   /* the reverb: each type's tail (time to fall 30 dB after a burst), and stereo */
            float tail[DR_NREV], mono[DR_NREV];
            int ty;
            for (ty = 0; ty < DR_NREV; ty++) {
                static float capr[88200];
                float ref;
                dry_init();
                dr_set(P_FADE, 0);
                dr_set(DR_GP(1, GP_LEVEL), 0);
                dr_set(P_REV_MIX, 100);
                dr_set(P_REV_TYPE, ty);
                run(secs(0.3f));                        /* the tank rebuilt at its size */
                dr_play(1);
                run(secs(0.3f));
                dr_play(0);
                run(secs(0.06f));                       /* the drone gone (50 ms fade) */
                capture(capr, 88200, 0);
                ref = rms_of(capr, 2205);
                for (k = 0; k < 40 && rms_of(capr + k * 2205, 2205) > ref * 0.0316f; k++)
                    ;
                tail[ty] = (float)k * 0.05f;
                {   /* stereo, and mono-safe: the tail's mid (L+R) within 6 dB of its sides (uncorrelated: 3) */
                    float sl = 0.0f, sr = 0.0f, sm = 0.0f;
                    int i2, kk = 0;
                    dr_play(1);
                    run(secs(0.12f));
                    dr_play(0);
                    run(secs(0.07f));
                    while (kk < 20480) {
                        dr_render(buf, 256, 3277);
                        for (i2 = 0; i2 < 256; i2++, kk++) {
                            float l = (float)buf[2 * i2] / FS, r = (float)buf[2 * i2 + 1] / FS;
                            sl += l * l;
                            sr += r * r;
                            sm += 0.25f * (l + r) * (l + r);
                        }
                    }
                    mono[ty] = 10.0f * log10f(sm / (0.5f * (sl + sr)));
                }
            }
            printf("    reverb tails (to -30 dB): room %.2f s, plate %.2f s, hall %.2f s, chorus hall %.2f s\n",
                   (double)tail[0], (double)tail[1], (double)tail[2], (double)tail[3]);
            printf("    reverb mono (mid vs sides): %.1f, %.1f, %.1f, %.1f dB\n", (double)mono[0], (double)mono[1],
                   (double)mono[2], (double)mono[3]);
            check("reverb: stereo but mono-safe (the mid within 6 dB of the sides, every type)",
                  mono[0] > -6.0f && mono[1] > -6.0f && mono[2] > -6.0f && mono[3] > -6.0f && mono[0] < -0.5f);
            check("reverb: room < plate < hall, chorus hall at least the hall", tail[0] < tail[1] && tail[1] < tail[2] &&
                                                                              tail[3] >= tail[2] && tail[0] > 0.1f);
        }
        {   /* the tails ring on after PLAY stops (the default: some reverb) */
            dr_init();
            dr_set(P_FADE, 0);
            dr_play(1);
            run(secs(1.0f));
            dr_play(0);
            run(secs(0.1f));
            run(secs(0.4f));
            check("the reverb rings on after PLAY stops", rms() > 1e-4f);
        }
    }

    dr_panic();
    run(1);
    check("panic: silent in the next block (the effects too)", nonzero == 0);

    /* the worst case: both drones with partners; resonance, drive, mix, level and the LFOs at the top;
     * cross-mod at full and hard sync on; the delay running away into the reverb; cutoffs swept */
    dry_init();
    dr_set(P_FADE, 0);
    dr_set(P_DLY_MIX, 50);
    dr_set(P_DLY_REPEATS, 100);
    dr_set(P_REV_MIX, 60);
    dr_set(P_XMOD, 1);
    dr_set(P_XMOD_AMT, 100);
    dr_set(P_SYNC, 1);
    for (i = 0; i < DR_NGROUPS; i++) {
        dr_set(DR_GP(i, GP_RESO), 1000);
        dr_set(DR_GP(i, GP_DRIVE), 100);
        dr_set(DR_GP(i, GP_MIX), 50);
        dr_set(DR_GP(i, GP_LEVEL), 100);
        dr_set(DR_GP(i, GP_FLFO_DEPTH), 100);       /* and the LFOs at full depth */
        dr_set(DR_GP(i, GP_OLFO_DEPTH), i & 1 ? 100 : -100);
    }
    dr_play(1);
    {
        float worst = 0.0f;
        int c, m;
        for (m = DR_LP; m <= DR_HP; m++) {
            for (i = 0; i < DR_NGROUPS; i++)
                dr_set(DR_GP(i, GP_MODE), m);
            for (c = 0; c <= 100; c += 5) {
                for (i = 0; i < DR_NGROUPS; i++)
                    dr_set(DR_GP(i, GP_CUTOFF), i & 1 ? 100 - c : c);
                run(secs(0.1f));
                worst = fmaxf(worst, peak);
            }
        }
        printf("    worst case peak %.3f\n", (double)worst);
        check("resonance + drive at the top: finite, inside full scale", finite_ok && worst < 1.0f);
    }
    for (i = 0; i < DR_NGROUPS; i++) {
        dr_set(DR_GP(i, GP_CUTOFF), 400);
        dr_set(DR_GP(i, GP_MIX), 0);
        dr_set(DR_GP(i, GP_FLFO_DEPTH), 0);
        dr_set(DR_GP(i, GP_OLFO_DEPTH), 0);
        dr_set(DR_GP(i, GP_DRIVE), 0);
        dr_set(DR_GP(i, GP_MODE), DR_LP);
    }
    run(secs(1.0f));
    run(secs(1.0f));
    check("resonance at the top, cutoff low: stable and sounding", finite_ok && rms() > 0.01f && peak < 1.0f);

    {   /* low-pass, the bass under the cutoff stays as loud when the resonance rises ("no bass loss",
         * as the DS-2's filter): a sine at 55 Hz, the cutoff at 1 kHz, resonance 0 and 90 */
        static float x[44100];
        float lv[2];
        int k;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        dr_set(DR_GP(0, GP_PITCH), 3300);
        dr_set(DR_GP(0, GP_CUTOFF), 600);
        for (k = 0; k < 2; k++) {
            dr_set(DR_GP(0, GP_RESO), k ? 90 : 0);
            run(secs(1.0f));
            capture(x, 44100, 0);
            lv[k] = tone_level(x, 44100, 55.0f);
        }
        printf("    bass at 55 Hz: resonance 0 %.4f, 90 %.4f (%+.1f dB)\n", (double)lv[0], (double)lv[1],
               20.0 * log10((double)(lv[1] / lv[0])));
        check("low-pass: the bass stays within 1.5 dB as the resonance rises", lv[1] > 0.84f * lv[0] && lv[1] < 1.19f * lv[0]);
    }

    {   /* RANDOM: Wave I steps to a new level each cycle and holds it; Wave II glides (no jumps); neither
         * repeats: 40 cycles, the levels spread and differ */
        float lv[40], jump = 0.0f, last, lo = 9.0f, hi = -9.0f;
        int c, same = 0, j;
        dry_init();
        dr_set(P_LFO_W1, DR_LFO_RANDOM);
        dr_set(P_LFO_W2, DR_LFO_RANDOM);
        dr_set(P_LFO_RATE, 71);                         /* ~6.6 Hz: ~26 blocks a cycle */
        dr_lfo_reset();
        run(secs(0.3f));
        last = dr_lfo[1];
        for (c = 0; c < 40; c++) {
            float mid = 0.0f;
            for (j = 0; j < 30; j++) {                   /* a little over a cycle, 256-frame blocks */
                run(1);
                jump = fmaxf(jump, fabsf(dr_lfo[1] - last));
                last = dr_lfo[1];
                if (j == 15)
                    mid = dr_lfo[0];
            }
            lv[c] = mid;
            lo = fminf(lo, mid);
            hi = fmaxf(hi, mid);
        }
        for (c = 1; c < 40; c++)
            same += fabsf(lv[c] - lv[c - 1]) < 1e-6f;
        printf("    random: Wave I levels %.2f .. %.2f, %d repeats in a row; Wave II's biggest step a block %.3f\n",
               (double)lo, (double)hi, same, (double)jump);
        check("random LFO: Wave I a new level each cycle, spread over the range", lo < -0.5f && hi > 0.5f && same < 4);
        check("random LFO: Wave II glides (no jumps)", jump < 0.2f);
    }

    {   /* the LFO locked to the pulse: with the pulse at 2 a second (a bar, two beats: 1 s) and the LFO at
         * ~0.05 Hz, the LFO starts over each bar, so its phase never passes ~1 s of it; free, it runs on */
        float mx[2];
        int lk, b;
        dry_init();
        dr_set(P_PULSE_RATE, 67);                       /* 2 a second */
        dr_set(P_PULSE, 1);
        dr_set(P_LFO_RATE, 10);                         /* 0.02 x 2^1.18 = 0.045 Hz */
        for (lk = 0; lk < 2; lk++) {
            dr_set(P_LFO_LOCK, lk);
            dr_lfo_reset();
            run(secs(0.1f));
            mx[lk] = 0.0f;
            for (b = 0; b < secs(4.0f); b++) {
                run(1);
                mx[lk] = fmaxf(mx[lk], dr_lfo_ph);
            }
        }
        printf("    LFO phase over 4 s: free %.3f, locked to the pulse %.3f (a bar: 0.045)\n", (double)mx[0], (double)mx[1]);
        check("LFO lock: the LFO starts over each pulse bar; free, it runs on", mx[1] < 0.05f && mx[0] > 0.15f);
        dr_set(P_LFO_LOCK, 0);
        dr_set(P_PULSE, 0);
    }

    {   /* RATE MOD: the LFO's own wave speeding it where high, slowing it where low: a triangle on Wave I
         * moving its own rate (+100: up to 8x) spends far less of its time high; at 0, half */
        float hi_share[2];
        int r, b, n;
        dry_init();
        dr_set(P_LFO_W1, 1);
        dr_set(P_LFO_RATE, 60);                         /* ~2.7 Hz */
        dr_set(P_LFO_RMOD_SRC, DR_LFO_I);
        for (r = 0; r < 2; r++) {
            dr_set(P_LFO_RMOD, r ? 100 : 0);
            run(secs(0.5f));
            n = 0;
            for (b = 0; b < secs(4.0f); b++) {
                run(1);
                n += dr_lfo[0] > 0.0f;
            }
            hi_share[r] = (float)n / (float)secs(4.0f);
        }
        printf("    rate mod: time Wave I spends high: %.2f plain, %.2f moving its own rate\n", (double)hi_share[0],
               (double)hi_share[1]);
        check("rate mod: the LFO's own wave speeds it up where high (less time high)",
              fabsf(hi_share[0] - 0.5f) < 0.06f && hi_share[1] < 0.3f && finite_ok);
        dr_set(P_LFO_RMOD, 0);
    }

    {   /* MORPH: a switch moves the sound over the Morph time, not at once. Morph 2.15 s (64): the filter
         * from low-pass to high-pass on a saw takes ~1 s: a quarter of the way in, the low end is part way
         * down; after, all the way. The pulse switched on deepens over it; the sub's octave dips and comes
         * back; at Morph 0 (20 ms) the same switch is done in a block or two */
        static float w[4410];
        float lo0, lo_q, lo_end, lo_fast;
        float pmin_early = 9.0f, pmin_late = 9.0f;
        int b;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 67);
        dr_set(DR_GP(0, GP_PITCH), 3300);                /* 55 Hz */
        dr_set(DR_GP(0, GP_CUTOFF), 500);
        run(secs(0.3f));                                 /* the setup settled (at Morph 0) */
        dr_set(P_MORPH, 64);                             /* 0.02 x 2^6.75 = 2.15 s */
        run(secs(0.7f));
        capture(w, 4410, 0);
        lo0 = tone_level(w, 4410, 55.0f);
        dr_set(DR_GP(0, GP_MODE), DR_HP);
        run(secs(0.4f));
        capture(w, 4410, 0);                             /* 0.4 .. 0.5 s into ~2.1 s */
        lo_q = tone_level(w, 4410, 55.0f);
        run(secs(2.5f));
        capture(w, 4410, 0);
        lo_end = tone_level(w, 4410, 55.0f);
        printf("    morph LP -> HP over 2.1 s: 55 Hz at %.3f, 0.45 s in %.3f, after %.4f\n", (double)lo0, (double)lo_q,
               (double)lo_end);
        check("morph: the filter mode crossfades over the Morph time (part way at 0.45 s, there after)",
              lo_q < 0.9f * lo0 && lo_q > 0.5f * lo0 && lo_end < 0.1f * lo0);
        dr_set(P_MORPH, 0);
        dr_set(DR_GP(0, GP_MODE), DR_LP);
        run(secs(0.05f));
        capture(w, 4410, 0);
        lo_fast = tone_level(w, 4410, 55.0f);
        check("morph 0: a switch is done in ~20 ms", lo_fast > 0.9f * lo0);
        /* the pulse, switched on with Morph 2.1 s: deepens over it (depth 100: closed at the end) */
        dr_set(P_MORPH, 64);
        dr_set(P_PULSE_RATE, 67);                       /* 2 a second (the default is slower now) */
        dr_set(P_PULSE_DEPTH, 100);
        dr_set(P_PULSE_SHAPE, 100);
        dr_set(P_PULSE, 1);
        for (b = 0; b < secs(0.5f); b++) {
            run(1);
            pmin_early = fminf(pmin_early, dr_pulse);
        }
        run(secs(2.0f));
        for (b = 0; b < secs(0.5f); b++) {
            run(1);
            pmin_late = fminf(pmin_late, dr_pulse);
        }
        printf("    morph: the pulse's deepest in its first 0.5 s %.2f, after 2.5 s %.2f\n", (double)pmin_early,
               (double)pmin_late);
        /* like a breath: most of the way at 0.5 s of 2.15 s (pa 1 - e^-0.93 = 0.6: -30 dB x 0.6, about 0.13),
         * not all of it */
        check("morph: the swell comes in over the Morph time, most of it first (part way at 0.5 s)",
              pmin_early > 0.06f && pmin_early < 0.3f && pmin_late < 0.05f);
        dr_set(P_PULSE, 0);
        dr_set(P_MORPH, 0);
        run(secs(0.2f));
    }
    {   /* the sub's octave, switched (Morph 0.6 s): it dips out and comes back an octave down, no click
         * (no sample-to-sample step bigger than the steady wave's own) */
        int32_t prev = 0;
        float steady = 0.0f, sw = 0.0f;
        int j, i2;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        dr_set(DR_GP(0, GP_MIX), 100);                  /* the sub alone */
        dr_set(DR_GP(0, GP_CUTOFF), 400);
        run(secs(0.5f));
        dr_set(P_MORPH, 55);
        run(secs(0.5f));
        for (j = 0; j < 160; j++) {
            if (j == 40)
                dr_set(DR_GP(0, GP_SUBOCT), 2);
            dr_render(buf, 256, 3277);
            for (i2 = 0; i2 < 256; i2++) {
                float d = fabsf((float)(buf[2 * i2] - prev)) / FS;
                prev = buf[2 * i2];
                if (j > 2 && j < 40)
                    steady = fmaxf(steady, d);
                else if (j >= 40)
                    sw = fmaxf(sw, d);
            }
        }
        printf("    sub octave switch: biggest step %.4f (steady %.4f)\n", (double)sw, (double)steady);
        check("morph: the sub's octave changes without a click", sw <= 1.05f * steady);
        dr_set(DR_GP(0, GP_SUBOCT), 1);
        dr_set(P_MORPH, 0);
    }

    {   /* a knob's step glides over the time asked for (dr_pslide), as a pitch's does: drone I's level
         * from 80 to 20 over 0.4 s: a third of the way in, about two thirds of the way; after, there */
        float a0, mid, end;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        dr_set(DR_GP(0, GP_LEVEL), 80);
        run(secs(1.0f));
        run(secs(0.1f));
        a0 = rms();
        dr_pslide(DR_GP(0, GP_LEVEL), 400);
        dr_set(DR_GP(0, GP_LEVEL), 20);
        run(secs(0.1f));
        run(secs(0.05f));
        mid = rms();
        run(secs(0.5f));
        run(secs(0.1f));
        end = rms();
        printf("    level 80 -> 20 over 0.4 s: RMS %.3f, at ~0.13 s %.3f, after %.3f\n", (double)a0, (double)mid, (double)end);
        check("a knob's step glides over its time (no step)", mid < 0.85f * a0 && mid > 0.45f * a0 && end < 0.3f * a0);
    }

    {   /* JUST tuning: the partner a fifth up on A4 (440 Hz): just, it sits at exactly 3/2 (660 Hz); tempered at
         * 659.3 Hz, a 2 s window around 660 Hz barely sees it */
        static float w[88200];
        float j1, j0;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        dr_set(DR_GP(0, GP_PITCH), 6900);
        dr_set(DR_GP(0, GP_PARTNER), 1);
        dr_set(DR_GP(0, GP_INTERVAL), 7);               /* +5th */
        dr_set(DR_GP(0, GP_DETUNE), 0);
        dr_set(P_JUST, 1);
        run(secs(1.0f));
        capture(w, 88200, 0);
        j1 = tone_level(w, 88200, 660.0f);
        dr_set(P_JUST, 0);
        run(secs(0.5f));
        capture(w, 88200, 0);
        j0 = tone_level(w, 88200, 660.0f);
        printf("    the partner's fifth at 660 Hz: just %.3f, tempered %.3f\n", (double)j1, (double)j0);
        check("just tuning: the partner's fifth is a pure 3/2", j1 > 3.0f * j0);
        dr_set(DR_GP(0, GP_PARTNER), 0);
    }
    {   /* UNISON, no Detune: one fat oscillator. A sine and its partner start half a cycle apart, and a detune
         * there and back leaves them anywhere: pulled into phase, they're louder than the sine alone (they cancelled:
         * -42 dB at the start); with Drift, they never beat */
        float alone, uni, uni2, lo = 1.0f;
        int b;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        run(secs(1.0f));
        alone = rms();
        dr_set(DR_GP(0, GP_PARTNER), 1);
        dr_set(DR_GP(0, GP_INTERVAL), 3);
        dr_set(DR_GP(0, GP_DETUNE), 0);
        run(secs(1.0f));
        run(secs(0.5f));
        uni = rms();
        dr_set(DR_GP(0, GP_DETUNE), 37);
        run(secs(1.3f));
        dr_set(DR_GP(0, GP_DETUNE), 0);
        run(secs(1.0f));
        run(secs(0.5f));
        uni2 = rms();
        dr_set(P_DRIFT, 100);
        for (b = 0; b < 40; b++) {
            run(secs(0.5f));
            lo = fminf(lo, rms());
        }
        printf("    sine alone %.3f; unison %.3f, after a detune %.3f; Drift 100, quietest 0.5 s of 20 %.3f\n",
               (double)alone, (double)uni, (double)uni2, (double)lo);
        check("unison without detune: the partner in phase (never cancels)", uni > alone && uni2 > alone && lo > alone);
        dr_set(P_DRIFT, 0);
        dr_set(DR_GP(0, GP_PARTNER), 0);
    }
    {   /* SUBHARMONICS: the sub alone (Osc mix 100) under A3 (220 Hz) at 1/3 and 1/5: 73.3 Hz and 44 Hz */
        static float w[8820];
        int lag3, lag5;
        pure_tone();
        dr_set(DR_GP(0, GP_PITCH), 5700);
        dr_set(DR_GP(0, GP_MIX), 100);
        dr_set(DR_GP(0, GP_SUBOCT), 3);
        run(secs(0.6f));
        capture(w, 8820, 0);
        lag3 = period(w, 8820, 400, 1200);
        dr_set(DR_GP(0, GP_SUBOCT), 4);
        run(secs(0.6f));
        capture(w, 8820, 0);
        lag5 = period(w, 8820, 400, 1200);
        printf("    subharmonics: 1/3 period %d samples (want 601), 1/5 %d (want 1002)\n", lag3, lag5);
        check("subharmonics: the sub at 1/3 and 1/5 of the pitch", abs(lag3 - 601) <= 2 && abs(lag5 - 1002) <= 3);
        dr_set(DR_GP(0, GP_SUBOCT), 1);
        dr_set(DR_GP(0, GP_MIX), 0);
    }
    {   /* the SPEAKER low cut: a 55 Hz sine down 12 dB or more at its top (240 Hz); off, untouched */
        static float w[44100];
        float off, on;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        dr_set(DR_GP(0, GP_PITCH), 3300);
        run(secs(0.5f));
        capture(w, 44100, 0);
        off = tone_level(w, 44100, 55.0f);
        dr_set(P_SPEAKER, 100);
        run(secs(0.3f));
        capture(w, 44100, 0);
        on = tone_level(w, 44100, 55.0f);
        printf("    speaker low cut: 55 Hz %.3f off, %.3f at 240 Hz\n", (double)off, (double)on);
        check("speaker low cut: the deep bass down 12 dB or more", on < 0.25f * off);
        dr_set(P_SPEAKER, 0);
    }
    {   /* MASTER past 4/5: it drives (more harmonics) at about the same loudness, inside full scale */
        float r8 = 0.0f, r10 = 0.0f, p10 = 0.0f;
        int b, i2;
        dry_init();
        dr_set(P_FADE, 0);
        dr_play(1);
        for (b = 0; b < secs(1.0f); b++)
            dr_render(buf, 256, 3277);
        for (b = 0; b < secs(0.5f); b++) {
            dr_render(buf, 256, 3277);
            for (i2 = 0; i2 < 512; i2++)
                r8 += ((float)buf[i2] / FS) * ((float)buf[i2] / FS);
        }
        for (b = 0; b < secs(0.5f); b++) {
            dr_render(buf, 256, 4096);
            for (i2 = 0; i2 < 512; i2++) {
                float v = (float)buf[i2] / FS;
                r10 += v * v;
                p10 = fmaxf(p10, fabsf(v));
            }
        }
        printf("    MASTER 4/5 vs full: RMS ratio %.2f, peak at full %.3f\n", (double)sqrtf(r10 / r8), (double)p10);
        check("MASTER's top drives at about the same loudness (within 3 dB), inside full scale",
              sqrtf(r10 / r8) > 0.7f && sqrtf(r10 / r8) < 1.42f && p10 < 1.0f);
    }

    {   /* MOVEMENT: every LFO depth scaled together. The filter LFO at depth 80 swings the sound; at Movement
         * 0 it's still; at 100 (x2) it swings wider than at 50 (as set) */
        float sw[3];
        int m, k2;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 67);
        dr_set(DR_GP(0, GP_CUTOFF), 400);
        dr_set(P_LFO_RATE, 45);
        dr_set(DR_GP(0, GP_FLFO), DR_LFO_I);
        dr_set(DR_GP(0, GP_FLFO_DEPTH), 50);
        for (m = 0; m < 3; m++) {
            float mn = 9.0f, mx = 0.0f;
            dr_set(P_MOVE, m * 50);
            run(secs(0.5f));
            for (k2 = 0; k2 < 30; k2++) {
                run(secs(0.1f));
                mn = fminf(mn, rms());
                mx = fmaxf(mx, rms());
            }
            sw[m] = mx / mn;
        }
        printf("    movement 0 / 100 / 200 %%: the filter LFO's swing x%.2f, x%.2f, x%.2f\n", (double)sw[0], (double)sw[1],
               (double)sw[2]);
        check("movement: 0 stills every LFO route, 200 % swings wider than 100 %", sw[0] < 1.05f && sw[2] > sw[1] && sw[1] > 1.3f);
        dr_set(P_MOVE, 50);
        dr_set(DR_GP(0, GP_FLFO_DEPTH), 0);
    }

    {   /* FEEDBACK at its worst (100, the delay running away, resonance and drive at the top): finite,
         * inside full scale; it sustains (still sounding 2 s after the drones' level is pulled to 0? no: the
         * loop runs through the drones' filters, which keep their input) */
        dry_init();
        dr_set(P_FADE, 0);
        dr_set(P_FEEDBACK, 100);
        dr_set(P_DLY_MIX, 100);
        dr_set(P_DLY_REPEATS, 100);
        dr_set(P_REV_MIX, 100);
        for (i = 0; i < DR_NGROUPS; i++) {
            dr_set(DR_GP(i, GP_RESO), 1000);
            dr_set(DR_GP(i, GP_DRIVE), 100);
        }
        dr_play(1);
        run(secs(4.0f));
        printf("    feedback at its worst: peak %.3f, RMS %.3f\n", (double)peak, (double)rms());
        check("feedback at its worst: finite, inside full scale", finite_ok && peak < 1.0f && rms() > 0.01f);
        {   /* and it never takes over: stopped, what's left 2 s on is the effects' own tail, as at 0 */
            float t100, t0;
            int r2;
            for (r2 = 0; r2 < 2; r2++) {
                dr_set(P_DLY_REPEATS, 60);
                dr_set(P_FEEDBACK, r2 ? 0 : 100);
                dr_play(1);
                run(secs(3.0f));
                dr_play(0);
                run(secs(2.0f));
                run(secs(0.5f));
                *(r2 ? &t0 : &t100) = rms();
            }
            printf("    stopped, 2 s on: feedback 100 %.4f, 0 %.4f\n", (double)t100, (double)t0);
            check("feedback never sustains itself (stopped, it fades as at 0)", t100 < 2.0f * t0 + 1e-4f);
        }
        dr_set(P_FEEDBACK, 0);
    }
    {   /* CHARACTER 100: darker (a pure saw's top end down; its jitter, a few cents, is printed: under the
         * period's one-sample resolution here). At 0 the code is skipped: exactly the old sound */
        static float w[4410];
        float b0, b1;
        int lmin = 9999, lmax = 0, k3;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 67);
        dr_set(DR_GP(0, GP_PITCH), 4500);
        run(secs(0.5f));
        capture(w, 4410, 0);
        b0 = tone_level(w, 4410, 7040.0f);              /* the 64th harmonic */
        dr_set(P_CHAR, 100);
        run(secs(0.5f));
        capture(w, 4410, 0);
        b1 = tone_level(w, 4410, 7040.0f);
        for (k3 = 0; k3 < 20; k3++) {
            int lag;
            capture(w, 4410, 0);
            lag = period(w, 4410, 380, 420);
            lmin = lag < lmin ? lag : lmin;
            lmax = lag > lmax ? lag : lmax;
        }
        printf("    character 100: the 64th harmonic %.4f -> %.4f; the period over 2 s %d..%d samples\n", (double)b0,
               (double)b1, lmin, lmax);
        check("character: darker (top harmonics down 6 dB or more), finite", b1 < 0.5f * b0 && finite_ok);
        dr_set(P_CHAR, 0);
    }

    {   /* no subsonic sub: at the bottom of the range (27.5 Hz) the 1/5 sub would be 5.5 Hz; it's folded up
         * to 22 Hz (two octaves) */
        static float w[88200];
        float l5, l22;
        pure_tone();
        dr_set(DR_GP(0, GP_PITCH), 2100);               /* 27.5 Hz */
        dr_set(DR_GP(0, GP_MIX), 100);
        dr_set(DR_GP(0, GP_SUBOCT), 4);
        run(secs(1.0f));
        capture(w, 88200, 0);
        l5 = tone_level(w, 88200, 5.5f);
        l22 = tone_level(w, 88200, 22.0f);
        printf("    the 1/5 sub under 27.5 Hz: 5.5 Hz %.4f, 22 Hz %.4f\n", (double)l5, (double)l22);
        check("no subsonic sub: under 20 Hz it goes up an octave", l22 > 10.0f * l5);
        dr_set(DR_GP(0, GP_SUBOCT), 1);
        dr_set(DR_GP(0, GP_MIX), 0);
    }

    {   /* DRIFT: the drones never quite still. Over a minute, drone I's cutoff (no LFO) wanders: not at all at
         * Drift 0, a little at 30, more at 100, and always around where it's set (inside +-0.5 octave) */
        float lo[3], hi[3];
        int d, b;
        for (d = 0; d < 3; d++) {
            dry_init();
            dr_set(P_FADE, 0);
            dr_set(P_DRIFT, d == 0 ? 0 : d == 1 ? 30 : 100);
            dr_play(1);
            run(secs(1.0f));
            lo[d] = 999.0f;
            hi[d] = -999.0f;
            for (b = 0; b < 60; b++) {
                run(secs(1.0f));
                lo[d] = fminf(lo[d], dr_cut[0]);
                hi[d] = fmaxf(hi[d], dr_cut[0]);
            }
        }
        printf("    drift over 60 s, drone I's cutoff (knob 70): 0: %.1f..%.1f, 30: %.1f..%.1f, 100: %.1f..%.1f\n",
               (double)lo[0], (double)hi[0], (double)lo[1], (double)hi[1], (double)lo[2], (double)hi[2]);
        check("drift: still at 0, wanders more as it rises, around where it's set",
              hi[0] - lo[0] < 0.01f && hi[1] - lo[1] > 0.3f && hi[2] - lo[2] > hi[1] - lo[1] &&
              lo[2] > 70.0f - 5.2f && hi[2] < 70.0f + 5.2f);
    }

    {   /* NOISE: its colour's slope, measured over a decade (250 Hz -> 2.5 kHz): white flat, pink -10 dB (-3 dB/oct) */
        static float x[32768];
        float sl[2], lv[2];
        int c;
        for (c = 0; c < 2; c++) {
            nz_c = c;
            noise_only(nz_backdrop, 100, x, 32768, 0);
            sl[c] = band_db(x, 32768, 2500.0f) - band_db(x, 32768, 250.0f);
            lv[c] = 20.0f * log10f(rms_of(x, 32768));
        }
        printf("    noise, 250 Hz -> 2.5 kHz: white %+.1f dB, pink %+.1f; RMS white %.1f, pink %.1f dBFS\n",
               (double)sl[0], (double)sl[1], (double)lv[0], (double)lv[1]);
        /* the colour against white: the chain after it (filter, clip) tilts both alike */
        check("noise colours: white flat, pink -10 dB a decade (against white)", fabsf(sl[0]) < 2.0f &&
              fabsf(sl[1] - sl[0] + 10.0f) < 1.5f);
        check("white and pink about as loud (within 3 dB)", fabsf(lv[1] - lv[0]) < 3.0f);
        /* a level beside the oscillators: they stay as they are (but for the soft clip into the filter, which
         * a hot mix squeezes a little); at 100 it's 6 dB under them (a saw), never over; 50 is -12 dB, 25 -18:
         * heard all along the knob */
        {
            float t0, t100, l0, n100, n50, n25;
            nz_saw();
            run(secs(0.5f));
            capture(x, 32768, 0);
            t0 = tone_level(x, 32768, 220.0f);
            l0 = rms_of(x, 32768);
            nz_saw();
            dr_set(DR_GP(0, GP_NOISE), 100);
            run(secs(0.5f));
            capture(x, 32768, 0);
            t100 = tone_level(x, 32768, 220.0f);
            noise_only(nz_saw, 100, x, 32768, 0);
            n100 = rms_of(x, 32768);
            noise_only(nz_saw, 50, x, 32768, 0);
            n50 = rms_of(x, 32768);
            noise_only(nz_saw, 25, x, 32768, 0);
            n25 = rms_of(x, 32768);
            printf("    the saw %.1f dBFS; its noise at 100 %.1f, at 50 %.1f, at 25 %.1f; the saw's 220 Hz with noise "
                   "100: %+.2f dB\n", (double)(20.0f * log10f(l0)), (double)(20.0f * log10f(n100)),
                   (double)(20.0f * log10f(n50)), (double)(20.0f * log10f(n25)), (double)(20.0f * log10f(t100 / t0)));
            check("noise beside the oscillators: they stay (within 1.5 dB at noise 100)",
                  fabsf(20.0f * log10f(t100 / t0)) < 1.5f);
            check("noise 100 6 dB under the oscillators (within 2 dB)", fabsf(20.0f * log10f(n100 / l0) + 6.0f) < 2.0f);
            check("noise's curve: 50 is -6 dB from the top, 25 -12 (within 1)", fabsf(20.0f * log10f(n50 / n100) + 6.0f) < 1.0f &&
                  fabsf(20.0f * log10f(n25 / n100) + 12.0f) < 1.0f);
        }
    }
    {   /* each drone its own noise: drone I left, drone II right: their noises uncorrelated */
        static float l[16384], r[16384];
        double lr = 0.0, ll = 0.0, rr = 0.0;
        int i;
        noise_only(nz_both, 100, l, 16384, 0);
        noise_only(nz_both, 100, r, 16384, 1);
        for (i = 0; i < 16384; i++) {
            lr += (double)l[i] * (double)r[i];
            ll += (double)l[i] * (double)l[i];
            rr += (double)r[i] * (double)r[i];
        }
        printf("    the two drones' noises: correlation %.3f\n", lr / sqrt(ll * rr));
        check("each drone its own noise (uncorrelated)", fabs(lr / sqrt(ll * rr)) < 0.1);
    }
    {   /* Noise 0: the Colour changes nothing (the noise isn't run): sample for sample, the same */
        static float a[8192], b[8192];
        int i, same = 1;
        dry_init();
        dr_set(P_FADE, 0);
        dr_play(1);
        capture(a, 8192, 0);
        dry_init();
        dr_set(P_FADE, 0);
        dr_set(DR_GP(0, GP_NCOLOR), 0);
        dr_set(DR_GP(1, GP_NCOLOR), 0);
        dr_play(1);
        capture(b, 8192, 0);
        for (i = 0; i < 8192; i++)
            same &= a[i] == b[i];
        check("noise 0: the sound as without it (the Colour changes nothing)", same);
    }
    {   /* the worst case: pink noise at the top over a saw, the filter self-oscillating and driven, both drones:
         * inside full scale */
        int g;
        nz_saw();
        for (g = 0; g < 2; g++) {
            dr_set(DR_GP(g, GP_LEVEL), 100);
            dr_set(DR_GP(g, GP_NOISE), 100);
            dr_set(DR_GP(g, GP_NCOLOR), 1);
            dr_set(DR_GP(g, GP_CUTOFF), 300);
            dr_set(DR_GP(g, GP_RESO), 1000);
            dr_set(DR_GP(g, GP_DRIVE), 100);
        }
        run(secs(0.5f));
        run(secs(3.0f));
        printf("    pink noise, resonance 100, drive 100, both drones: peak %.3f\n", (double)peak);
        check("noise driven into a singing filter stays inside full scale", peak < 0.99f && finite_ok);
    }

    {   /* no clicks: Delay mix, Reverb mix, Repeats, Stereo stepped a detent every 60 ms on a pure tone, and the
         * Speaker cut turned back to off, never step (they were read raw: each detent a click on a soft sound).
         * The worst second difference while turning, against the tone's own */
        static const int KN[5] = {P_DLY_MIX, P_REV_MIX, P_DLY_REPEATS, P_WIDTH, P_SPEAKER};
        static float w[2646];
        float steady, worst[5], p1, p2;
        int k, d, bad = 0;
        pure_tone();
        dr_set(DR_GP(0, GP_WAVE), 0);
        dr_set(P_WIDTH, 0);
        run(secs(1.0f));
        capture(w, 2646, 0);
        steady = 0.0f;
        for (i = 2; i < 2646; i++)
            steady = fmaxf(steady, fabsf(w[i] - 2.0f * w[i - 1] + w[i - 2]));
        for (k = 0; k < 5; k++) {
            worst[k] = 0.0f;
            capture(w, 2560, 0);                    /* whole blocks, so each capture follows the last */
            p2 = w[2558];
            p1 = w[2559];
            for (d = 1; d <= 12; d++) {
                dr_set(KN[k], k == 4 ? (d < 12 ? 30 : 0) : d * 4);   /* the Speaker on, then off at the end */
                capture(w, 2560, 0);                /* the step lands on its first sample: from the last two */
                for (i = 0; i < 2560; i++) {
                    worst[k] = fmaxf(worst[k], fabsf(w[i] - 2.0f * p1 + p2));
                    p2 = p1;
                    p1 = w[i];
                }
            }
            bad += worst[k] > 4.0f * steady;
            dr_set(KN[k], 0);
            run(secs(1.0f));
        }
        printf("    turning, the worst step vs the tone's own (x): delay mix %.1f, reverb mix %.1f, repeats %.1f, "
               "stereo %.1f, speaker off %.1f\n", (double)(worst[0] / steady), (double)(worst[1] / steady),
               (double)(worst[2] / steady), (double)(worst[3] / steady), (double)(worst[4] / steady));
        check("the mixes, Repeats, Stereo and the Speaker turned off don't click", bad == 0);
    }

    {   /* RANDOM's sequence: an init repeats it (the tests rely on that); dr_seed (the UI's, at the first PLAY)
         * makes a new one, so the device wanders differently each time it's switched on */
        float seq[3][12];
        int k, same0 = 1, same1 = 1;
        for (k = 0; k < 3; k++) {
            dry_init();
            if (k == 2)
                dr_seed(4321u);
            dr_set(P_LFO_W1, DR_LFO_RANDOM);
            dr_set(P_LFO_RATE, 60);
            for (i = 0; i < 12; i++) {
                run(secs(0.5f));
                seq[k][i] = dr_lfo[0];
            }
            for (i = 0; i < 12 && k; i++) {
                same0 &= k != 1 || seq[1][i] == seq[0][i];
                same1 &= k != 2 || seq[2][i] == seq[0][i];
            }
        }
        check("Random: an init repeats the sequence; a seed (the first PLAY) makes a new one", same0 && !same1);
    }

    check("output finite throughout", finite_ok);
    printf(bad ? "drone: %d FAILED\n" : "drone: all ok\n", bad);
    return bad ? 1 : 0;
}
