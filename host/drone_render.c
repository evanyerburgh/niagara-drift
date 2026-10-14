/* SPDX-License-Identifier: GPL-3.0-only */
/* Render the drone engine (dsp/drone.c) to WAV files, one per scene, so it can be heard without the
 * app or the hardware. A scene is a list of timed commands; a ramp moves a parameter in steps.
 *   drone_render OUTDIR          -> OUTDIR/<scene>.wav (44.1 kHz, 24-bit stereo), peak and RMS printed */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "drone.h"

enum { E_SET, E_RAMP, E_PLAY, E_VOICE, E_SOLO, E_END };   /* E_SOLO: the other voices' levels to 0 */
typedef struct {
    float t;                                       /* seconds */
    int ev, a, b, c;                               /* E_RAMP: param a from b to c over dur */
    float dur;
} ev_t;

typedef struct {
    const char *name, *about;
    ev_t ev[40];
} scene_t;

#define V1(p) DR_GP(0, p)                        /* drone I */
#define V2(p) DR_GP(1, p)                        /* drone II */

static const scene_t SCENES[] = {
    {"01-two-drones", "the two drones (D2 and A2, each with its partner), their filters opening, the fade out",
     {{0, E_SET, P_FADE, 60}, {0, E_PLAY, 1}, {0, E_RAMP, V1(GP_CUTOFF), 350, 700, 12},
      {0, E_RAMP, V2(GP_CUTOFF), 300, 650, 12}, {14, E_PLAY, 0}, {20, E_END}}},
    {"02-partners-off", "the same with both partners off: two plain oscillators and their subs",
     {{0, E_SET, P_FADE, 60}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V2(GP_PARTNER), 0}, {0, E_PLAY, 1},
      {0, E_RAMP, V1(GP_CUTOFF), 350, 700, 12}, {0, E_RAMP, V2(GP_CUTOFF), 300, 650, 12}, {14, E_PLAY, 0}, {20, E_END}}},
    {"03-ultra-analog", "the DS-2 patch: drone II an octave up, slightly detuned, slow LFO; then drone I -> drone II cutoff",
     {{0, E_SET, P_FADE, 60}, {0, E_SET, V2(GP_PITCH), 5012}, {0, E_SET, V2(GP_WAVE), 67},
      {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V2(GP_PARTNER), 0}, {0, E_SET, V2(GP_CUTOFF), 380},
      {0, E_SET, V2(GP_RESO), 450}, {0, E_SET, V2(GP_DRIVE), 60}, {0, E_SET, P_LFO_RATE, 15},
      {0, E_SET, V1(GP_FLFO_DEPTH), 40}, {0, E_SET, V2(GP_FLFO_DEPTH), 35}, {0, E_PLAY, 1},
      {8, E_SET, P_XMOD, 1}, {8, E_RAMP, P_XMOD_AMT, 0, 70, 6}, {20, E_PLAY, 0}, {23, E_END}}},
    {"04-hard-sync", "hard sync: drone II synced to drone I, its pitch swept up and back (the classic sync sweep)",
     {{0, E_SOLO, 1}, {0, E_SET, P_FADE, 30}, {0, E_SET, V2(GP_PARTNER), 0}, {0, E_SET, V2(GP_WAVE), 67},
      {0, E_SET, V2(GP_MIX), 0}, {0, E_SET, V2(GP_CUTOFF), 800}, {0, E_SET, P_SYNC, 1}, {0, E_PLAY, 1},
      {1, E_RAMP, V2(GP_PITCH), 4500, 6900, 7}, {8, E_RAMP, V2(GP_PITCH), 6900, 4500, 5}, {14, E_PLAY, 0},
      {15, E_END}}},
    {"05-pitch-link", "pitch link: drone II a fifth above drone I, following it as drone I moves",
     {{0, E_SET, P_FADE, 40}, {0, E_SET, V2(GP_PITCH), 4500}, {0, E_SET, P_PLINK_OFF, 700}, {0, E_SET, P_PLINK, 1},
      {0, E_PLAY, 1}, {3, E_RAMP, V1(GP_PITCH), 3800, 4300, 5}, {10, E_RAMP, V1(GP_PITCH), 4300, 3600, 4},
      {16, E_PLAY, 0}, {18, E_END}}},
    {"06-resonance", "drone I alone, resonance near the top: low-pass swept up and down, then high-pass",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 30}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V1(GP_RESO), 950},
      {0, E_SET, V1(GP_DRIVE), 50}, {0, E_PLAY, 1}, {0, E_RAMP, V1(GP_CUTOFF), 100, 900, 7},
      {7, E_RAMP, V1(GP_CUTOFF), 900, 100, 7}, {14, E_SET, V1(GP_MODE), DR_HP},
      {14, E_RAMP, V1(GP_CUTOFF), 100, 800, 5}, {19, E_PLAY, 0}, {21, E_END}}},
    {"07-self-oscillation", "drone I alone, resonance at the top, low cutoff: the filter sings its own tone",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 30}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V1(GP_RESO), 1000},
      {0, E_SET, V1(GP_WAVE), 33}, {0, E_SET, V1(GP_MIX), 0}, {0, E_SET, V1(GP_DRIVE), 0}, {0, E_PLAY, 1},
      {0, E_RAMP, V1(GP_CUTOFF), 450, 750, 10}, {10, E_PLAY, 0}, {12, E_END}}},
    {"08-pulse-and-sub", "drone I alone: a square whose width is swept by hand, OSC MIX half way to the sub-sub",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 30}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V1(GP_WAVE), 100},
      {0, E_SET, V1(GP_MIX), 50}, {0, E_SET, V1(GP_SUBOCT), 2}, {0, E_SET, V1(GP_CUTOFF), 650}, {0, E_PLAY, 1},
      {0, E_RAMP, V1(GP_WIDTH), 0, 95, 6}, {6, E_RAMP, V1(GP_WIDTH), 95, 0, 6}, {12, E_PLAY, 0}, {14, E_END}}},
    {"09-x-wave", "drone I alone, filter open: the X-WAVE swept by hand, sine -> triangle -> saw -> square and back",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 20}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V1(GP_MIX), 0},
      {0, E_SET, V1(GP_CUTOFF), 850}, {0, E_SET, V1(GP_RESO), 0}, {0, E_SET, V1(GP_PITCH), 4500},
      {0, E_SET, V1(GP_WAVE), 0}, {0, E_PLAY, 1}, {1, E_RAMP, V1(GP_WAVE), 0, 100, 8},
      {9, E_RAMP, V1(GP_WAVE), 100, 0, 4}, {13, E_PLAY, 0}, {14, E_END}}},
    {"10-between-notes", "drone II held on A2, drone I swept slowly from A2 to E3, through every cent",
     {{0, E_SET, P_FADE, 40}, {0, E_SET, V1(GP_PITCH), 4500}, {0, E_SET, V2(GP_PITCH), 4500},
      {0, E_SET, V2(GP_PARTNER), 0}, {0, E_PLAY, 1}, {2, E_RAMP, V1(GP_PITCH), 4500, 5200, 12},
      {16, E_PLAY, 0}, {18, E_END}}},
    {"11-lfo-filters", "both drones breathing: the filter LFOs at a slow rate (~9 s), I and II a quarter apart",
     {{0, E_SET, P_FADE, 60}, {0, E_SET, P_LFO_RATE, 10}, {0, E_SET, V1(GP_FLFO_DEPTH), 60},
      {0, E_SET, V2(GP_FLFO_DEPTH), -55}, {0, E_SET, V1(GP_RESO), 500}, {0, E_SET, V2(GP_RESO), 500},
      {0, E_PLAY, 1}, {24, E_PLAY, 0}, {27, E_END}}},
    {"12-lfo-x-wave", "drone I alone, its LFO moving the X-WAVE: Wave I triangle, then the 2x exponential",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 30}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V1(GP_MIX), 0},
      {0, E_SET, V1(GP_CUTOFF), 800}, {0, E_SET, V1(GP_WAVE), 50}, {0, E_SET, P_LFO_RATE, 35},
      {0, E_SET, V1(GP_OLFO_DEST), DR_TO_XWAVE}, {0, E_SET, V1(GP_OLFO_DEPTH), 70}, {0, E_PLAY, 1},
      {8, E_SET, P_LFO_W1, 6}, {14, E_PLAY, 0}, {15, E_END}}},
    {"13-lfo-shapes", "drone I alone, its filter on the LFO at ~2 Hz: Wave I's seven shapes, then Wave II's, 2.5 s each",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 20}, {0, E_SET, V1(GP_PARTNER), 0}, {0, E_SET, V1(GP_RESO), 600},
      {0, E_SET, V1(GP_CUTOFF), 400}, {0, E_SET, V1(GP_FLFO_DEPTH), 70}, {0, E_SET, P_LFO_RATE, 55},
      {0, E_PLAY, 1}, {2.5f, E_SET, P_LFO_W1, 2}, {5, E_SET, P_LFO_W1, 3}, {7.5f, E_SET, P_LFO_W1, 4},
      {10, E_SET, P_LFO_W1, 5}, {12.5f, E_SET, P_LFO_W1, 6}, {15, E_SET, P_LFO_W1, 7},
      {17.5f, E_SET, V1(GP_FLFO), DR_LFO_II}, {20, E_SET, P_LFO_W2, 2}, {22.5f, E_SET, P_LFO_W2, 3},
      {25, E_SET, P_LFO_W2, 4}, {27.5f, E_SET, P_LFO_W2, 5}, {30, E_SET, P_LFO_W2, 6}, {32.5f, E_SET, P_LFO_W2, 7},
      {35, E_PLAY, 0}, {36, E_END}}},
    {"14-pulse", "the pulse on both drones at 2 a second: a smooth swell, then a hard gate, then swing",
     {{0, E_SET, P_FADE, 30}, {0, E_SET, P_PULSE, 1}, {0, E_SET, P_PULSE_RATE, 50}, {0, E_SET, P_PULSE_SHAPE, 0},
      {0, E_PLAY, 1}, {6, E_RAMP, P_PULSE_SHAPE, 0, 100, 4}, {12, E_RAMP, P_PULSE_SWING, 0, 40, 3},
      {18, E_PLAY, 0}, {19, E_END}}},
    {"15-delay", "the delay: dark repeats under the pulse, then the LFO warbling its time, then Repeats up to runaway",
     {{0, E_SET, P_FADE, 30}, {0, E_SET, P_REV_MIX, 15}, {0, E_SET, P_DLY_MIX, 45}, {0, E_SET, P_DLY_TIME, 55},
      {0, E_SET, P_DLY_REPEATS, 55}, {0, E_SET, P_PULSE, 1}, {0, E_SET, P_PULSE_RATE, 35}, {0, E_PLAY, 1},
      {8, E_SET, P_LFO_RATE, 40}, {8, E_SET, P_DLY_LFO_DEPTH, 70}, {16, E_SET, P_DLY_LFO_DEPTH, 0},
      {16, E_RAMP, P_DLY_REPEATS, 55, 100, 6}, {24, E_RAMP, P_DLY_TIME, 55, 20, 4}, {28, E_PLAY, 0},
      {28, E_SET, P_DLY_REPEATS, 40}, {32, E_END}}},
    {"16-reverb-types", "the reverb's four types on the pulsing drones, 6 s each: room, plate, hall, chorus hall",
     {{0, E_SET, P_FADE, 20}, {0, E_SET, P_REV_MIX, 55}, {0, E_SET, P_REV_TYPE, 0}, {0, E_SET, P_PULSE, 1},
      {0, E_SET, P_PULSE_RATE, 20}, {0, E_SET, P_PULSE_SHAPE, 100}, {0, E_PLAY, 1}, {6, E_SET, P_REV_TYPE, 1},
      {12, E_SET, P_REV_TYPE, 2}, {18, E_SET, P_REV_TYPE, 3}, {24, E_PLAY, 0}, {28, E_END}}},
    {"17-noise-colours", "the drones as they start, drone I's noise at the top (-6 dB) beside its tones: white, then "
     "pink, white, pink (the switch crossfading over the Morph)",
     {{0, E_SET, P_FADE, 20}, {0, E_SET, V1(GP_NOISE), 100}, {0, E_SET, V1(GP_NCOLOR), 0}, {0, E_PLAY, 1},
      {6, E_SET, V1(GP_NCOLOR), 1}, {11, E_SET, V1(GP_NCOLOR), 0}, {16, E_SET, V1(GP_NCOLOR), 1}, {21, E_PLAY, 0},
      {23, E_END}}},
    {"18-noise-level", "the drones as they start, both drones' noise (pink) brought up slowly from none to the top, "
     "then back down to 50",
     {{0, E_SET, P_FADE, 20}, {0, E_PLAY, 1}, {3, E_RAMP, V1(GP_NOISE), 0, 100, 10},
      {3, E_RAMP, V2(GP_NOISE), 0, 100, 10}, {16, E_RAMP, V1(GP_NOISE), 100, 50, 6},
      {16, E_RAMP, V2(GP_NOISE), 100, 50, 6}, {28, E_PLAY, 0}, {31, E_END}}},
    {"19-noise-resonance", "drone I alone, its noise at 80 (pink), resonance near the top: the cutoff swept slowly, "
     "the filter's ring catching the noise",
     {{0, E_SOLO, 0}, {0, E_SET, P_FADE, 30}, {0, E_SET, V1(GP_NOISE), 80}, {0, E_SET, V1(GP_RESO), 930},
      {0, E_SET, V1(GP_FLFO_DEPTH), 0}, {0, E_SET, V1(GP_CUTOFF), 400}, {0, E_PLAY, 1},
      {1, E_RAMP, V1(GP_CUTOFF), 400, 800, 7}, {8, E_RAMP, V1(GP_CUTOFF), 800, 450, 7}, {16, E_PLAY, 0},
      {19, E_END}}},
};

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

static int render(const scene_t *s, const char *dir)
{
    char path[512];
    FILE *f;
    float end = 0.0f, peak = 0.0f;
    double sumsq = 0.0;
    uint32_t frames = 0, total;
    int i;
    for (i = 0; s->ev[i].ev != E_END; i++)
        ;
    end = s->ev[i].t;
    total = (uint32_t)(end * DR_SR);
    snprintf(path, sizeof path, "%s/%s.wav", dir, s->name);
    f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return 1;
    }
    fwrite("RIFF", 1, 4, f);
    put32(f, 36 + total * 6);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 1);
    put16(f, 2);
    put32(f, 44100);
    put32(f, 44100 * 6);
    put16(f, 6);
    put16(f, 24);
    fwrite("data", 1, 4, f);
    put32(f, total * 6);

    dr_init();
    while (frames < total) {
        int32_t buf[2 * 256];
        uint32_t n = total - frames < 256 ? total - frames : 256, j;
        float now = (float)frames / DR_SR;
        for (i = 0; s->ev[i].ev != E_END; i++) {
            const ev_t *e = &s->ev[i];
            if (e->ev == E_RAMP && now >= e->t && now < e->t + e->dur) {
                float x = (now - e->t) / e->dur;
                dr_set(e->a, (int)lrintf((float)e->b + x * (float)(e->c - e->b)));
            } else if (e->t <= now && e->t > now - 256.0f / DR_SR) {     /* once, in the block it falls in */
                if (e->ev == E_SET)
                    dr_set(e->a, e->b);
                else if (e->ev == E_PLAY)
                    dr_play(e->a);
                else if (e->ev == E_VOICE)
                    dr_voice(e->a, e->b);
                else if (e->ev == E_SOLO) {
                    int g;
                    for (g = 0; g < DR_NGROUPS; g++)
                        if (g != e->a)
                            dr_set(DR_GP(g, GP_LEVEL), 0);
                }
            }
        }
        dr_render(buf, n, 3277);
        for (j = 0; j < 2 * n; j++) {
            float x = (float)buf[j] / 8388608.0f;
            uint8_t b[3] = {(uint8_t)buf[j], (uint8_t)(buf[j] >> 8), (uint8_t)(buf[j] >> 16)};
            fwrite(b, 1, 3, f);
            peak = fmaxf(peak, fabsf(x));
            sumsq += (double)x * x;
        }
        frames += n;
    }
    fclose(f);
    printf("%-24s %5.1f s  peak %6.1f dBFS  RMS %6.1f dBFS   %s\n", s->name, (double)end,
           20.0 * log10((double)peak + 1e-9), 10.0 * log10(sumsq / (2.0 * total) + 1e-18), s->about);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned i;
    int bad = 0;
    if (argc < 2) {
        fprintf(stderr, "usage: drone_render OUTDIR\n");
        return 2;
    }
    for (i = 0; i < sizeof SCENES / sizeof SCENES[0]; i++)
        bad |= render(&SCENES[i], argv[1]);
    return bad;
}
