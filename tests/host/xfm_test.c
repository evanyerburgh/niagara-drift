/* SPDX-License-Identifier: GPL-3.0-only */
/* Cross-FM and Snarl (Cross-mod on pitch, on the output: v0.3): each drone's sound bends the other's pitch, or the
 * finished sound bends both, and their pitches stay where they're set: each oscillator's average frequency, counted over 20 s from its phase, within 2 cents of its
 * frequency with Cross-mod off, at every depth. Built with the engine itself (its oscillators' phases) */
#include "drone.c"
#include <math.h>
#include <stdio.h>

static int32_t out[2 * 64];

static void count(int mode, int amt, double hz[2])
{
    double cyc[2] = {0.0, 0.0};
    float last[2];
    int b, g;
    dr_init();
    dr_set(P_FADE, 0);
    dr_set(P_PULSE, 0);
    dr_set(P_DRIFT, 0);
    dr_set(P_MOVE, 0);
    dr_set(P_XMOD, mode);
    dr_set(P_XMOD_AMT, amt);
    dr_play(1);
    for (b = 0; b < 44100 * 2 / 64; b++)
        dr_render(out, 64, 3277);
    last[0] = grp[0].ph;
    last[1] = grp[1].ph;
    for (b = 0; b < 44100 * 20 / 64; b++) {
        dr_render(out, 64, 3277);
        for (g = 0; g < 2; g++) {                 /* under a turn a block: its growth, counted */
            float d = grp[g].ph - last[g];
            cyc[g] += d < 0.0f ? d + 1.0f : d;
            last[g] = grp[g].ph;
        }
    }
    hz[0] = cyc[0] / 20.0;
    hz[1] = cyc[1] / 20.0;
}

int main(void)
{
    static const int amts[] = {30, 60, 100};
    double ref[2], hz[2];
    int i, g, bad = 0;
    count(0, 0, ref);
    for (i = 0; i < 3; i++) {
        double c[2];
        count(DR_XMOD_FM, amts[i], hz);
        for (g = 0; g < 2; g++)
            c[g] = 1200.0 * log2(hz[g] / ref[g]);
        printf("    cross-FM at %3d: drone I %+.2f cents, drone II %+.2f cents\n", amts[i], c[0], c[1]);
        if (fabs(c[0]) > 2.0 || fabs(c[1]) > 2.0)
            bad = 1;
    }
    for (i = 0; i < 3; i++) {
        double c[2];
        count(DR_XMOD_OUT, amts[i], hz);
        for (g = 0; g < 2; g++)
            c[g] = 1200.0 * log2(hz[g] / ref[g]);
        printf("    Snarl at %3d: drone I %+.2f cents, drone II %+.2f cents\n", amts[i], c[0], c[1]);
        if (fabs(c[0]) > 2.0 || fabs(c[1]) > 2.0)
            bad = 1;
    }
    printf("%s cross-FM, Snarl: both drones keep their pitch (within 2 cents) at every depth\n", bad ? "FAIL" : "ok  ");
    return bad;
}
