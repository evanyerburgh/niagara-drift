/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift app: the project (everything that is saved) and the app's entry points. Included by the unity
 * build after gfx.c (drawing helpers in scope) and by the host simulator. */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "../dsp/drone.h"
#include "../dsp/fastmath.h"            /* the UI's few float sums (the pitch in Hz) */

#ifndef OM_VERSION                       /* a release build passes its own (tools/build.py --release) */
#define OM_VERSION "DEV"
#endif
#define PROJ_MAGIC 0x314E5244u           /* "DRN1" */
#define PROJ_FORMAT 19u                  /* 19: the Swell's Depth in dB (18: noise, 17: cutoff / resonance 0..1000, 16: Feedback, Character, 15: Movement, 14: tuning, speaker, pitch lock, subharmonics (13: gliding knobs, Scene time, 12: Morph, 11: the LFO rate mod, 10: the LFO lock, 9: the filter LFO's destination, 8: the effects,
                                          * 7: the pulse, 6: scenes) */
#define NSCENES 16                       /* the white keys */

typedef struct {
    uint32_t magic, format;
    int16_t par[DR_NPARAMS];             /* drone.h parameters */
    uint8_t leds;                        /* key lights on */
    uint8_t rsv[15];                     /* room to grow: an older, shorter project loads (zeros here) */
    uint16_t scene_used;                 /* bit n: scene n holds a sound */
    int16_t scene[NSCENES][DR_NPARAMS];  /* the scenes: every parameter, recalled but for the Fade */
} project_t;
_Static_assert(sizeof(project_t) <= PLAT_STORE_MAX, "the project must fit one flash object");

extern project_t proj;

void project_defaults(void);
int project_load(void);                  /* 0 = loaded; else defaults are in place */
int project_save(void);                  /* 0 = saved and verified */
void project_apply(void);                /* every value to the engine (boot, load) */

void ui_init(void);
void ui_frame(void);
void ui_input_only(void);
int ui_dirty(void);
void ui_say(const char *a, const char *b);
