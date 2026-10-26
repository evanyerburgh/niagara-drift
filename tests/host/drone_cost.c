/* SPDX-License-Identifier: GPL-3.0-only */
/* What the drone engine costs: host instructions per 44.1 kHz sample, counted by the kernel
 * (proc_pid_rusage ri_instructions, macOS; the method and the device estimate are Felucca's
 * tests/regress.c and drum_test.c: about 1.7 % of the FM-1's audio budget per 100 host instructions a
 * sample, the ratio of Felucca's PHYS engine measured on the device). Each case renders 0.5 s to settle,
 * then 2 s counted. For scale: FoMni's engine at its busiest (strings, chord and rhythm), which runs on the
 * FM-1, measured ~1,875 the same way (~32 %).
 *   drone_cost            prints the table; fails if a case is over its limit */
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#ifdef __APPLE__
#include <libproc.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#include "drone.h"

#define DEVICE_PCT_PER_INSTR 0.017            /* % of the device's audio budget per host instruction a sample */

static uint64_t instr_now(void)
{
#ifdef __APPLE__
    struct rusage_info_v4 ri;
    if (!proc_pid_rusage(getpid(), RUSAGE_INFO_V4, (rusage_info_t *)&ri))
        return ri.ri_instructions;
#endif
    return 0;
}

static uint64_t ns_now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000u + (uint64_t)t.tv_nsec;
}

typedef void (*render_fn)(int32_t *, uint32_t, uint32_t);
static double cost(render_fn f, double *ns)
{
    static int32_t buf[2 * 256];
    const uint32_t blocks = (uint32_t)(2.5f * DR_SR / 256.0f), settle = (uint32_t)(0.5f * DR_SR / 256.0f);
    uint64_t i0 = 0, t0 = 0;
    uint32_t b;
    for (b = 0; b < blocks; b++) {
        if (b == settle) {
            i0 = instr_now();
            t0 = ns_now();
        }
        f(buf, 256, 3277);
    }
    *ns = (double)(ns_now() - t0) / ((blocks - settle) * 256.0);
    return i0 ? (double)(instr_now() - i0) / ((blocks - settle) * 256.0) : 0.0;
}

static void drone(int partners, int voices, int heavy)   /* voices: how many drones sound (the rest off) */
{
    int g;
    dr_init();
    dr_set(P_FADE, 0);
    for (g = 0; g < DR_NGROUPS; g++) {
        dr_set(DR_GP(g, GP_PARTNER), partners);
        if (g >= voices)
            dr_voice(g, 0);
        if (heavy) {
            dr_set(DR_GP(g, GP_MIX), 50);
            dr_set(DR_GP(g, GP_DRIVE), 100);
            dr_set(DR_GP(g, GP_RESO), 100);
            dr_set(DR_GP(g, GP_WAVE), 85);        /* between saw and square: both computed */
            dr_set(DR_GP(g, GP_OLFO_DEPTH), 50);  /* both LFO routes on, the LFO on the X-WAVE */
            dr_set(DR_GP(g, GP_OLFO_DEST), DR_TO_XWAVE);
            dr_set(DR_GP(g, GP_FLFO_DEPTH), 50);
            dr_set(DR_GP(g, GP_NOISE), 30);       /* the noise, coloured (six shelves a sample) */
            dr_set(DR_GP(g, GP_NCOLOR), 100);
        }
    }
    if (heavy) {                                  /* and the links: cross-mod (a filter update a sample), sync */
        dr_set(P_XMOD, 1);
        dr_set(P_SYNC, 1);
        dr_set(P_PLINK, 1);
        dr_set(P_PULSE, 1);                      /* and the pulse (a cosine a sample) */
        dr_set(P_DLY_MIX, 50);                   /* and the delay with its LFO, into the chorus hall */
        dr_set(P_DLY_LFO_DEPTH, 50);
        dr_set(P_REV_TYPE, 3);
    }
    if (heavy > 1) {                              /* the worst case: everything above, with the feedback, the
                                                   * character, Drift, stereo, rate mod, Wander, Age and the gestures in */
        dr_set(P_FEEDBACK, 100);
        dr_set(P_CHAR, 100);
        dr_set(P_DRIFT, 100);
        dr_set(P_WIDTH, 100);
        dr_set(P_LFO_RMOD, 50);
        dr_set(P_DLY_REPEATS, 90);
        dr_set(P_REV_MIX, 80);
        dr_set(P_WANDER, 100);
        dr_set(P_AGE, 100);
        dr_set(P_DLY_DRIVE, 60);                 /* the drive after the delay (v0.3) */
        for (g = 0; g < DR_NGEST; g++)            /* every gesture in but Clean (which takes things away) */
            if (g != DR_G_CLEAN)
                dr_gesture(g, 1000);
    }
    dr_play(1);
}

static int bad;
static void report(const char *name, double ipc, double ns, double limit)
{
    int over = limit > 0.0 && ipc > limit;
    if (ipc > 0.0)
        printf("  %-50s %6.0f instr/sample  ~%4.1f %% device  (%5.1f ns host)%s\n", name, ipc,
               ipc * DEVICE_PCT_PER_INSTR, ns, over ? "  OVER LIMIT" : "");
    else
        printf("  %-50s  no instruction counter on this host (%5.1f ns)\n", name, ns);
    bad += over;
}

int main(void)
{
    double ns, ipc;
    printf("drone cost, host instructions per 44.1 kHz sample (device estimate 1.7 %% per 100):\n");

    drone(1, 2, 0);
    dr_play(0);
    dr_panic();
    ipc = cost(dr_render, &ns);
    report("drone stopped (silence)", ipc, ns, 0);

    drone(0, 1, 0);
    ipc = cost(dr_render, &ns);
    report("1 drone, no partner", ipc, ns, 0);

    drone(1, 1, 0);
    ipc = cost(dr_render, &ns);
    report("1 drone with its partner", ipc, ns, 0);

    drone(1, 2, 0);
    ipc = cost(dr_render, &ns);
    report("2 drones with partners (defaults)", ipc, ns, 0);

    drone(1, 2, 1);
    ipc = cost(dr_render, &ns);
    /* the limit: the engine's worst case under ~40 % of the device, leaving the rest for the delay,
     * the reverb and the UI */
    report("everything: blends, LFOs, links, pulse, delay, reverb", ipc, ns, 2350.0);

    drone(1, 2, 2);
    ipc = cost(dr_render, &ns);
    report("worst case: + feedback, character, drift, macros, drive", ipc, ns, 0);

    drone(1, 2, 2);
    dr_set(P_XMOD, DR_XMOD_FM);                   /* the same, Cross-mod on pitch: cross-FM (v0.3) */
    ipc = cost(dr_render, &ns);
    report("the same, Cross-mod on pitch (cross-FM)", ipc, ns, 0);

    drone(1, 2, 2);
    dr_set(P_XMOD, DR_XMOD_OUT);                  /* ... and on the output: Snarl (v0.3) */
    ipc = cost(dr_render, &ns);
    report("the same, Cross-mod on the output (Snarl)", ipc, ns, 0);

    printf(bad ? "drone cost: OVER LIMIT\n" : "drone cost: ok\n");
    return bad ? 1 : 0;
}
