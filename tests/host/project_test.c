/* SPDX-License-Identifier: GPL-3.0-only */
/* The project's formats: a 0.1 project (19, 16 scenes) and v0.2's first (20) load as banks: scenes 1-12 in
 * bank A, 13-16 as B1-B4, the noise Colour (0 / 1 in 19) turned into the blend (0 / 100). A saved v0.2
 * project (21) and its banks come back as they were; older formats give the defaults. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/app.h"

static uint8_t store[OBJ_NOBJ][PLAT_STORE_MAX];
static int store_n[OBJ_NOBJ] = {0};
int plat_store_load(uint32_t obj, void *dst, uint32_t max)
{
    int n = store_n[obj];
    if (!n)
        return -1;
    if ((uint32_t)n > max)
        n = (int)max;
    memcpy(dst, store[obj], (size_t)n);
    return n;
}
static int fail_obj = -1;                       /* a save of this object fails (a power cut, say) */
int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
    if ((int)obj == fail_obj)
        return -1;
    memcpy(store[obj], src, len);
    store_n[obj] = (int)len;
    return 0;
}

#include "../../firmware/src/app/project.c"

/* a project as 23-31 saved it: 98 values a sound (before the delay's drive) */
static void store_v31(const project_t *p)
{
    static proj_v31_t o;
    int i;
    memset(&o, 0, sizeof o);
    o.magic = p->magic;
    o.format = p->format;
    memcpy(o.par, p->par, sizeof o.par);
    o.leds = p->leds;
    o.bank = p->bank;
    o.mtime = p->mtime;
    o.scene_used = p->scene_used;
    for (i = 0; i < NSCENES; i++)
        memcpy(o.scene[i], p->scene[i], sizeof o.scene[i]);
    memcpy(store[OBJ_PROJ], &o, sizeof o);
    store_n[OBJ_PROJ] = (int)sizeof o;
}

static int fails;
static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    fails += !ok;
}

static void old_project(uint32_t format)       /* 16 scenes of 78 values, as 0.1 saved them; scene n's pitch 3000 + n */
{
    static proj_v20_t o;
    int i, k;
    memset(&o, 0, sizeof o);
    o.magic = PROJ_MAGIC;
    o.format = format;
    for (k = 0; k < NP21; k++)
        o.par[k] = dr_param_info(k)->def;
    o.par[DR_GP(0, GP_PITCH)] = 4321;
    o.par[DR_GP(0, GP_NCOLOR)] = format == 19u ? 1 : 100;
    o.par[DR_GP(1, GP_NCOLOR)] = 0;
    o.leds = 1;
    o.scene_used = 1u << 0 | 1u << 11 | 1u << 12 | 1u << 15;
    for (i = 0; i < 16; i++) {
        for (k = 0; k < NP21; k++)
            o.scene[i][k] = o.par[k];
        o.scene[i][DR_GP(0, GP_PITCH)] = (int16_t)(3000 + i);
        o.scene[i][DR_GP(1, GP_NCOLOR)] = format == 19u ? 1 : 100;
    }
    memset(store_n, 0, sizeof store_n);
    memcpy(store[OBJ_PROJ], &o, sizeof o);
    store_n[OBJ_PROJ] = (int)sizeof o;
}

int main(void)
{
    const int16_t *a1, *a12, *b1, *b4;
    old_project(19u);
    check("a 0.1 project loads", project_load() == 0);
    check("now format 37, on bank A", proj.format == 37u && PROJ_FORMAT == 37u && proj.bank == 0 && proj.mtime == MTIME_DEF);
    check("a value 0.1 didn't have (Weight's depth) at its default", proj.par[P_GD_0] == dr_param_info(P_GD_0)->def);
    check("its sound is kept", proj.par[DR_GP(0, GP_PITCH)] == 4321);
    check("Colour pink (1) becomes 100, white stays 0",
          proj.par[DR_GP(0, GP_NCOLOR)] == 100 && proj.par[DR_GP(1, GP_NCOLOR)] == 0);
    a1 = scene_get(0, 0);
    a12 = scene_get(0, 1);                     /* (v0.3: keys 9-12 are gestures, so A12 moved to A2) */
    b1 = scene_get(1, 0);
    b4 = scene_get(1, 3);
    check("scene 1 is A1; scene 12 moved to A2 (keys 9-12 are gestures)", a1 && a12 && a1[DR_GP(0, GP_PITCH)] == 3000 &&
                                                 a12[DR_GP(0, GP_PITCH)] == 3011 && !scene_has(0, 11));
    check("scenes 13 and 16 are B1 and B4", b1 && b4 && b1[DR_GP(0, GP_PITCH)] == 3012 &&
                                                b4[DR_GP(0, GP_PITCH)] == 3015);
    check("empty ones stay empty (A3, B2, C5); P9-P12 in C1-C4", !scene_get(0, 2) && !scene_get(1, 1) && !scene_get(2, 4) &&
                                                                 scene_has(2, 0) && scene_has(2, 3));
    check("the scenes' Colours converted, B's too", a1[DR_GP(1, GP_NCOLOR)] == 100 && b4[DR_GP(1, GP_NCOLOR)] == 100);
    check("saved: the project and banks B and C written", project_save() == 0 && store_n[OBJ_PROJ] == (int)sizeof(project_t) &&
                                                       store_n[OBJ_BANKB] == (int)sizeof(bank_t) &&
                                                       store_n[OBJ_BANKC] == (int)sizeof(bank_t));
    proj.par[DR_GP(0, GP_PITCH)] = 5000;
    check("a scene into C3", scene_put(2, 2) == 0 && scene_get(2, 2) && project_save() == 0 && store_n[OBJ_BANKC]);
    check("no Preset can be saved over (P3, P12)", scene_put(BANK_PRESETS, 2) != 0 &&
                                                       scene_put(BANK_PRESETS, PRESET_LOCKED) != 0 &&
                                                       scene_get(BANK_PRESETS, PRESET_LOCKED)[P_II_OUT] == 1);
    proj.bank = 2;
    project_save();
    {   /* a P3 saved over before 2026-10-19 (OBJ_BANKP): moved to C's first empty slot (C5: C1-C4 hold P9-P12, C3
         * stored over), P3 the firmware's */
        static bank_t bp;
        int i;
        memset(&bp, 0, sizeof bp);
        bp.magic = BANK_MAGIC;
        bp.format = PROJ_FORMAT;
        bp.used = 1u << 2;
        for (i = 0; i < DR_NPARAMS; i++)
            bp.scene[2][i] = dr_param_info(i)->def;
        bp.scene[2][DR_GP(0, GP_PITCH)] = 4321;
        memcpy(store[OBJ_BANKP], &bp, sizeof bp);
        store_n[OBJ_BANKP] = (int)sizeof bp;
    }
    memset(&proj, 0, sizeof proj);
    check("reloaded: as saved", project_load() == 0 && proj.bank == 2 && scene_get(1, 3) &&
                                    scene_get(1, 3)[DR_GP(0, GP_PITCH)] == 3015 && scene_get(2, 2) &&
                                    scene_get(2, 2)[DR_GP(0, GP_PITCH)] == 5000 && scene_get(0, 1));
    check("... the P3 saved over now in C5, P3 the firmware's", scene_get(2, 4) &&
                                                     scene_get(2, 4)[DR_GP(0, GP_PITCH)] == 4321 &&
                                                     scene_get(BANK_PRESETS, 2)[DR_GP(0, GP_PITCH)] != 4321);
    check("... and saved: C and the emptied Presets object", project_save() == 0 &&
                                                     ((bank_t *)store[OBJ_BANKP])->used == 0 &&
                                                     (((bank_t *)store[OBJ_BANKC])->used & (1u << 4)));
    {   /* PR #14's (23): the same layout, the gestures Beat, Heat, Converge, Wash */
        static project_t p23;
        p23 = proj;
        p23.format = 23u;
        p23.par[P_WANDER] = 40;                /* Bloom then: gone */
        p23.par[P_FEEDBACK] = 40;
        p23.par[P_GD_1] = 70;              /* Heat's depth then; out of Shift's range now */
        p23.par[P_GL_0] = 100;            /* Beat latched then */
        store_v31(&p23);
        check("PR #14's project (23) loads: its sound kept, the gestures at their defaults, the set macros gone",
              project_load() == 0 && proj.par[P_FEEDBACK] == 40 && proj.par[P_WANDER] == 0 && proj.par[P_GD_1] == dr_param_info(P_GD_1)->def &&
                  proj.par[P_SHIFT_MOVE] == 1400 &&
                  proj.par[P_GL_0] == 0 && proj.par[P_GR_3] == dr_param_info(P_GR_3)->def);
        p23 = proj;                            /* PR #18's (24): the long swells */
        p23.format = 24u;
        p23.par[P_GR_1] = 70;
        p23.par[P_GD_1] = 2;                   /* Shift's move then (a 4th down) */
        p23.par[P_GL_3] = 100;
        store_v31(&p23);
        check("PR #18's project (24) loads: the swells to the new defaults, Shift's move moved, the latches let go",
              project_load() == 0 && proj.par[P_GR_1] == dr_param_info(P_GR_1)->def &&
                  proj.par[P_SHIFT_MOVE] == 200 && proj.par[P_GD_1] == dr_param_info(P_GD_1)->def &&
                  proj.par[P_GL_3] == 0);
    }
    {   /* PR #24's (29): Tuning was one setting; now each Interval has its just one */
        static project_t p29;
        p29 = proj;
        p29.format = 29u;
        p29.par[DR_GP(0, GP_INTERVAL)] = 7;    /* +5th then */
        p29.par[DR_GP(1, GP_INTERVAL)] = 3;    /* unison then */
        p29.par[P_II_OUT] = 1;
        store_v31(&p29);
        check("PR #24's project (29) loads: a just +5th stays just, unison stays unison, Tuning folded in",
              project_load() == 0 && proj.par[DR_GP(0, GP_INTERVAL)] == 13 &&
                  proj.par[DR_GP(1, GP_INTERVAL)] == DR_IV_UNISON && proj.par[P_II_OUT] == 0);
    }
    {   /* 0.2-beta's (30): Level was linear; now squared, the same loudness at 10 sqrt(v), the scenes' too */
        static project_t p30;
        p30 = proj;
        p30.format = 30u;
        p30.par[DR_GP(0, GP_LEVEL)] = 57;
        p30.par[DR_GP(1, GP_LEVEL)] = 37;
        p30.scene_used |= 1u;
        p30.scene[0][DR_GP(0, GP_LEVEL)] = 100;
        p30.scene[0][DR_GP(1, GP_LEVEL)] = 1;
        store_v31(&p30);
        check("0.2-beta's project (30) loads: Level 57 / 37 is 75 / 61 on the new taper, a scene's 100 / 1 is 100 / 10",
              project_load() == 0 && proj.par[DR_GP(0, GP_LEVEL)] == 75 && proj.par[DR_GP(1, GP_LEVEL)] == 61 &&
                  proj.scene[0][DR_GP(0, GP_LEVEL)] == 100 && proj.scene[0][DR_GP(1, GP_LEVEL)] == 10);
    }
    {   /* 0.2's (31): 98 values a sound; the delay's drive comes in at 0 (off), everything else kept, bank B's too */
        static project_t p31;
        static bank_v31_t b31;
        p31 = proj;
        p31.format = 31u;
        p31.par[P_PRESS_SENS] = 63;
        p31.par[DR_GP(1, GP_CUTOFF)] = 432;
        p31.scene_used |= 1u << 4;
        p31.scene[4][P_DLY_TIME] = 77;
        p31.scene[4][P_PRESS_SENS] = 21;
        store_v31(&p31);
        memset(&b31, 0, sizeof b31);
        b31.magic = BANK_MAGIC;
        b31.format = 31u;
        b31.used = 1u << 2;
        memcpy(b31.scene[2], p31.par, sizeof b31.scene[2]);
        b31.scene[2][P_REV_LEN] = 91;
        memcpy(store[OBJ_BANKB], &b31, sizeof b31);
        store_n[OBJ_BANKB] = (int)sizeof b31;
        check("0.2's project (31) loads: its sound and A5 kept, the delay's drive off",
              project_load() == 0 && proj.par[P_PRESS_SENS] == 63 && proj.par[DR_GP(1, GP_CUTOFF)] == 432 &&
                  proj.par[P_DLY_DRIVE] == 0 && scene_get(0, 4) && scene_get(0, 4)[P_DLY_TIME] == 77 &&
                  scene_get(0, 4)[P_PRESS_SENS] == 21 && scene_get(0, 4)[P_DLY_DRIVE] == 0);
        check("... B3 as it was, its drive off", scene_get(1, 2) && scene_get(1, 2)[P_REV_LEN] == 91 &&
                                                   scene_get(1, 2)[P_DLY_DRIVE] == 0 &&
                                                   scene_get(1, 2)[P_PRESS_SENS] == 63);
        store_n[OBJ_BANKB] = 0;
    }
    {   /* PR #10's (21): bank A in the project, B in its own object, 78 values each */
        static proj_v21_t p21;
        static bank_v21_t b21;
        int k;
        memset(&p21, 0, sizeof p21);
        memset(&b21, 0, sizeof b21);
        p21.magic = PROJ_MAGIC;
        p21.format = 21u;
        for (k = 0; k < NP21; k++)
            p21.par[k] = b21.scene[2][k] = p21.scene[5][k] = dr_param_info(k)->def;
        p21.par[DR_GP(1, GP_PITCH)] = 4444;
        p21.bank = 1;
        p21.scene_used = 1u << 5;
        p21.scene[5][DR_GP(0, GP_PITCH)] = 3333;
        b21.magic = BANK_MAGIC;
        b21.format = 21u;
        b21.used = 1u << 2;
        b21.scene[2][DR_GP(0, GP_PITCH)] = 2222;
        memset(store_n, 0, sizeof store_n);
        memcpy(store[OBJ_PROJ], &p21, sizeof p21);
        store_n[OBJ_PROJ] = (int)sizeof p21;
        memcpy(store[OBJ_BANKB], &b21, sizeof b21);
        store_n[OBJ_BANKB] = (int)sizeof b21;
        check("PR #10's project (21) loads, its sound and bank kept",
              project_load() == 0 && proj.par[DR_GP(1, GP_PITCH)] == 4444 && proj.bank == 1);
        check("... A6 and B3 as they were, the macros at their defaults",
              scene_get(0, 5) && scene_get(0, 5)[DR_GP(0, GP_PITCH)] == 3333 && scene_get(1, 2) &&
                  scene_get(1, 2)[DR_GP(0, GP_PITCH)] == 2222 && scene_get(1, 2)[P_WANDER] == 0 &&
                  scene_get(1, 2)[P_GD_1] == dr_param_info(P_GD_1)->def);
        check("... and B written again in the new layout on the next save",
              project_save() == 0 && store_n[OBJ_BANKB] == (int)sizeof(bank_t));
    }
    {   /* PR #12's (22): the macros, 94 values a sound; the effects' four take their defaults */
        static proj_v22_t p22;
        static bank_v22_t b22;
        int k;
        memset(&p22, 0, sizeof p22);
        memset(&b22, 0, sizeof b22);
        p22.magic = PROJ_MAGIC;
        p22.format = 22u;
        for (k = 0; k < NP22; k++)
            p22.par[k] = b22.scene[0][k] = p22.scene[1][k] = dr_param_info(k)->def;
        p22.par[P_WANDER] = 60;
        p22.par[P_FEEDBACK] = 60;
        p22.par[P_REV_TYPE] = 1;
        p22.bank = 2;
        p22.scene_used = 1u << 1;
        p22.scene[1][P_GL_3] = 100;
        b22.magic = BANK_MAGIC;
        b22.format = 22u;
        b22.used = 1u;
        b22.scene[0][P_CHAR] = 77;
        memset(store_n, 0, sizeof store_n);
        memcpy(store[OBJ_PROJ], &p22, sizeof p22);
        store_n[OBJ_PROJ] = (int)sizeof p22;
        memcpy(store[OBJ_BANKC], &b22, sizeof b22);
        store_n[OBJ_BANKC] = (int)sizeof b22;
        check("PR #12's project (22) loads: its sound, reverb type and bank kept, the set macros gone, Gesture time at its default",
              project_load() == 0 && proj.par[P_FEEDBACK] == 60 && proj.par[P_WANDER] == 0 && proj.par[P_REV_TYPE] == 1 && proj.bank == 2 &&
                  proj.mtime == MTIME_DEF && proj.par[P_REV_LEN] == dr_param_info(P_REV_LEN)->def);
        check("... A2 and C1 as they were, the new values and the gestures at their defaults",
              scene_get(0, 1) && scene_get(0, 1)[P_GL_3] == 0 && scene_get(2, 0) &&
                  scene_get(2, 0)[P_CHAR] == 77 && scene_get(2, 0)[P_DLY_TYPE] == 3 &&
                  scene_get(2, 0)[P_PRESS_SENS] == 50);
        check("... and C written again in the new layout on the next save",
              project_save() == 0 && store_n[OBJ_BANKC] == (int)sizeof(bank_t));
    }
    old_project(20u);
    check("v0.2's first (20) loads, Colours as they were",
          project_load() == 0 && proj.par[DR_GP(0, GP_NCOLOR)] == 100 && scene_get(1, 0) &&
              scene_get(1, 0)[DR_GP(1, GP_NCOLOR)] == 100);
    {   /* 32 (#38, #39): 99 values a sound, the delay types then Tape, Wide, Ping-pong, Clean, Grain, Self; now Clean,
         * Wide, Ping-pong, Tape, Grain, Self */
        static proj_v32_t p32;
        static bank_v32_t b32;
        int i;
        memset(&p32, 0, sizeof p32);
        memset(&b32, 0, sizeof b32);
        p32.magic = PROJ_MAGIC;
        p32.format = 32u;
        b32.magic = BANK_MAGIC;
        b32.format = 32u;
        for (i = 0; i < NP32; i++)
            p32.par[i] = p32.scene[0][i] = p32.scene[1][i] = b32.scene[0][i] = dr_param_info(i)->def;
        p32.par[P_DLY_TYPE] = 3;                   /* Clean then */
        p32.scene[0][P_DLY_TYPE] = 0;              /* Tape */
        p32.scene[1][P_DLY_TYPE] = 4;              /* Grain: the same */
        p32.scene_used = 3u;
        b32.scene[0][P_DLY_TYPE] = 1;              /* Wide: the same */
        b32.used = 1u;
        memset(store_n, 0, sizeof store_n);
        memcpy(store[OBJ_PROJ], &p32, sizeof p32);
        store_n[OBJ_PROJ] = (int)sizeof p32;
        memcpy(store[OBJ_BANKB], &b32, sizeof b32);
        store_n[OBJ_BANKB] = (int)sizeof b32;
        check("32's delay types, in the new order: Clean and Tape trade places, the others stay",
              project_load() == 0 && proj.par[P_DLY_TYPE] == 0 && scene_get(0, 0)[P_DLY_TYPE] == 3 &&
                  scene_get(0, 1)[P_DLY_TYPE] == 4 && scene_get(1, 0)[P_DLY_TYPE] == 1);
        check("... the reverb's route (new in 33) as it was: both the drones and their echoes into it",
              proj.par[P_DLY_ROUTE] == 0 && scene_get(0, 0)[P_DLY_ROUTE] == 0 && scene_get(1, 0)[P_DLY_ROUTE] == 0);
        check("... and written again in 33 on the next save", project_save() == 0 && store_n[OBJ_BANKB] == (int)sizeof(bank_t));
    }
    {   /* 34 (#43): 101 values a sound, before the LFO's, the delay's and the scenes' clock values */
        static proj_v34_t p34;
        int i;
        memset(&p34, 0, sizeof p34);
        p34.magic = PROJ_MAGIC;
        p34.format = 34u;
        for (i = 0; i < NP34; i++)
            p34.par[i] = dr_param_info(i)->def;
        p34.par[P_PULSE_DIV] = 6;
        memset(store_n, 0, sizeof store_n);
        memcpy(store[OBJ_PROJ], &p34, sizeof p34);
        store_n[OBJ_PROJ] = (int)sizeof p34;
        check("34 loads: the Swell's division kept, the new clock values at their defaults (4 bars, 1/4, 4 bars)",
              project_load() == 0 && proj.par[P_PULSE_DIV] == 6 && proj.par[P_LFO_DIV] == 3 && proj.par[P_DLY_NOTE] == 5 &&
                  proj.par[P_SCENE_BARS] == 2);
    }
    {   /* 33 (#40-#42): 100 values a sound, before the Swell's division */
        static proj_v33_t p33;
        int i;
        memset(&p33, 0, sizeof p33);
        p33.magic = PROJ_MAGIC;
        p33.format = 33u;
        for (i = 0; i < NP33; i++)
            p33.par[i] = p33.scene[0][i] = dr_param_info(i)->def;
        p33.par[P_PULSE_RATE] = 40;
        p33.par[P_DLY_ROUTE] = 2;
        p33.scene_used = 1u;
        memset(store_n, 0, sizeof store_n);
        memcpy(store[OBJ_PROJ], &p33, sizeof p33);
        store_n[OBJ_PROJ] = (int)sizeof p33;
        check("33 loads: its values kept, the Swell's division at its default (8 bars)",
              project_load() == 0 && proj.par[P_PULSE_RATE] == 40 && proj.par[P_DLY_ROUTE] == 2 &&
                  proj.par[P_PULSE_DIV] == 2 && scene_get(0, 0)[P_PULSE_DIV] == 2);
    }
    old_project(18u);
    check("format 18 gives the defaults", project_load() == -1 && !scene_get(0, 0) && !scene_get(1, 0));
    {   /* a P2 saved over before 2026-10-19 in an older format (31): moved scene by scene from the load buffer; and the
         * old Presets object is rewritten only once the scene's new home is on the flash (2026-10-27) */
        static bank_v31_t bp;
        int i;
        project_defaults();
        proj.bank = 0;
        project_save();
        memset(&bp, 0, sizeof bp);
        bp.magic = BANK_MAGIC;
        bp.format = 31u;
        bp.used = 1u << 1;
        for (i = 0; i < NP31; i++)
            bp.scene[1][i] = dr_param_info(i)->def;
        bp.scene[1][DR_GP(0, GP_PITCH)] = 4444;
        memcpy(store[OBJ_BANKP], &bp, sizeof bp);
        store_n[OBJ_BANKP] = (int)sizeof bp;
        check("an old-format P2 saved over loads into C5 (C1-C4: P9-P12, v0.3)", project_load() == 0 && scene_get(2, 4) &&
                                                         scene_get(2, 4)[DR_GP(0, GP_PITCH)] == 4444);
        fail_obj = OBJ_BANKC;
        check("... C's save failing: the old Presets object keeps P2", project_save() != 0 &&
                                                         (((bank_v31_t *)store[OBJ_BANKP])->used & (1u << 1)));
        fail_obj = -1;
        check("... saved: C holds it, then the old Presets object lets it go", project_save() == 0 &&
                                                         (((bank_t *)store[OBJ_BANKC])->used & (1u << 4)) &&
                                                         ((bank_v31_t *)store[OBJ_BANKP])->used == 0);
        memset(&proj, 0, sizeof proj);
        check("... and reloaded: in C5 once, not moved again", project_load() == 0 && scene_get(2, 4) &&
                                                         scene_get(2, 4)[DR_GP(0, GP_PITCH)] == 4444 && !scene_has(2, 5));
    }
    {   /* a Preset is built when it's asked for: the same values as the table, each time */
        const int16_t *a = scene_get(BANK_PRESETS, 4);
        int16_t p5[DR_NPARAMS];
        memcpy(p5, a, sizeof p5);
        scene_get(BANK_PRESETS, 9);
        check("a Preset built again is the same", memcmp(scene_get(BANK_PRESETS, 4), p5, sizeof p5) == 0 &&
                                                     scene_has(BANK_PRESETS, 7) && !scene_has(BANK_PRESETS, 8));
    }
    {   /* 0.3: keys 9-12 are gestures. A format-35 project with scenes on A10, A11 and B12: they move to the first
         * empty keys 1-8 (A's own first, then B's); P9-P12 are copied into C's empty keys; nothing over a scene */
        static proj_v35_t o;
        static bank_v35_t bb;
        int i, k;
        memset(store_n, 0, sizeof store_n);
        memset(&o, 0, sizeof o);
        o.magic = PROJ_MAGIC;
        o.format = 35u;
        o.bank = 0;
        o.mtime = MTIME_DEF;
        for (k = 0; k < NP35; k++)
            o.par[k] = dr_param_info(k)->def;
        for (i = 0; i < NSCENES; i++)
            for (k = 0; k < NP35; k++)
                o.scene[i][k] = dr_param_info(k)->def;
        o.scene_used = 0x0FFu | 1u << 9 | 1u << 10;    /* A1-A8 full, and A10, A11 */
        o.scene[9][DR_GP(0, GP_PITCH)] = 4100;
        o.scene[10][DR_GP(0, GP_PITCH)] = 4200;
        memcpy(store[OBJ_PROJ], &o, sizeof o);
        store_n[OBJ_PROJ] = (int)sizeof o;
        memset(&bb, 0, sizeof bb);
        bb.magic = BANK_MAGIC;
        bb.format = 35u;
        bb.used = 1u << 11 | 1u << 0;                  /* B1 and B12 */
        for (i = 0; i < NSCENES; i++)
            for (k = 0; k < NP35; k++)
                bb.scene[i][k] = dr_param_info(k)->def;
        bb.scene[11][DR_GP(0, GP_PITCH)] = 4300;
        memcpy(store[OBJ_BANKB], &bb, sizeof bb);
        store_n[OBJ_BANKB] = (int)sizeof bb;
        check("0.3: a format-35 project loads", project_load() == 0 && proj.format == PROJ_FORMAT);
        check("... A10, A11 to B2, B3 (A full); B12 to B4", scene_get(1, 1) && scene_get(1, 1)[DR_GP(0, GP_PITCH)] == 4100 &&
                                                       scene_get(1, 2)[DR_GP(0, GP_PITCH)] == 4200 &&
                                                       scene_get(1, 3)[DR_GP(0, GP_PITCH)] == 4300 &&
                                                       !scene_has(0, 9) && !scene_has(1, 11) && proj_note_moved == 3);
        check("... P9-P12 copied into C1-C4", proj_note_presets == 4 &&
                                               memcmp(scene_get(2, 0), preset_get(8), sizeof(int16_t) * DR_NPARAMS) == 0 &&
                                               memcmp(scene_get(2, 3), preset_get(11), sizeof(int16_t) * DR_NPARAMS) == 0);
        check("... the new latches at their defaults", proj.par[P_GL_4] == 0 && proj.par[P_GL_7] == 0);
        check("... saved and reloaded: as moved, nothing moved again", project_save() == 0 && project_load() == 0 &&
                                                       proj_note_moved == 0 && proj_note_presets == 0 &&
                                                       scene_get(1, 3)[DR_GP(0, GP_PITCH)] == 4300 && scene_has(2, 3) &&
                                                       !scene_has(2, 4));
    }
    {   /* the same, its first save cut short after the banks (the review, 2026-10-28): the project still 35 on the
         * flash, B and C already 36. Loaded again: A's scenes move within A only, P9-P12 aren't copied twice */
        static proj_v35_t o;
        int i, k;
        memset(&o, 0, sizeof o);
        o.magic = PROJ_MAGIC;
        o.format = 35u;
        o.mtime = MTIME_DEF;
        for (k = 0; k < NP35; k++)
            o.par[k] = dr_param_info(k)->def;
        for (i = 0; i < NSCENES; i++)
            for (k = 0; k < NP35; k++)
                o.scene[i][k] = dr_param_info(k)->def;
        o.scene_used = 0x07Fu | 1u << 9;               /* A1-A7, and A10 (moved to B before the cut) */
        o.scene[9][DR_GP(0, GP_PITCH)] = 4100;
        memcpy(store[OBJ_PROJ], &o, sizeof o);
        store_n[OBJ_PROJ] = (int)sizeof o;             /* B and C as the save before left them (format 36) */
        check("a cut-short first save: loads", project_load() == 0);
        check("... A10 to A8 (within A), not to B again", scene_has(0, 7) && scene_get(0, 7)[DR_GP(0, GP_PITCH)] == 4100 &&
                                                         proj_note_moved == 1 && proj_note_presets == 0);
        check("... C holds P9-P12 once (C5 empty)", scene_has(2, 3) && !scene_has(2, 4));
    }
    {   /* format 36 (108 values): loads, its sounds kept, each drone's sub wave the square it was (2026-10-28) */
        static proj_v36_t o;
        static bank_v36_t bb;
        int i, k;
        memset(store_n, 0, sizeof store_n);
        memset(&o, 0, sizeof o);
        o.magic = PROJ_MAGIC;
        o.format = 36u;
        o.mtime = MTIME_DEF;
        for (k = 0; k < NP36; k++)
            o.par[k] = dr_param_info(k)->def;
        for (i = 0; i < NSCENES; i++)
            for (k = 0; k < NP36; k++)
                o.scene[i][k] = dr_param_info(k)->def;
        o.par[DR_GP(1, GP_PITCH)] = 6100;
        o.par[P_GL_7] = 40;
        o.scene_used = 0x003u;
        o.scene[1][DR_GP(0, GP_PITCH)] = 4400;
        memcpy(store[OBJ_PROJ], &o, sizeof o);
        store_n[OBJ_PROJ] = (int)sizeof o;
        memset(&bb, 0, sizeof bb);
        bb.magic = BANK_MAGIC;
        bb.format = 36u;
        bb.used = 1u << 2;
        for (i = 0; i < NSCENES; i++)
            for (k = 0; k < NP36; k++)
                bb.scene[i][k] = dr_param_info(k)->def;
        bb.scene[2][DR_GP(0, GP_PITCH)] = 4500;
        memcpy(store[OBJ_BANKB], &bb, sizeof bb);
        store_n[OBJ_BANKB] = (int)sizeof bb;
        check("a format-36 project loads", project_load() == 0 && proj.format == PROJ_FORMAT);
        check("... its sound and scenes kept", proj.par[DR_GP(1, GP_PITCH)] == 6100 && proj.par[P_GL_7] == 40 &&
                                               scene_get(0, 1)[DR_GP(0, GP_PITCH)] == 4400 &&
                                               scene_get(1, 2)[DR_GP(0, GP_PITCH)] == 4500 && proj_note_moved == 0);
        check("... the subs square, as they were", proj.par[P_SUBW_0] == DR_SUBW_SQUARE &&
                                                  proj.par[P_SUBW_1] == DR_SUBW_SQUARE &&
                                                  scene_get(1, 2)[P_SUBW_1] == DR_SUBW_SQUARE);
    }
    printf("project: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
