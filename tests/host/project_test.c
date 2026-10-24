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
int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
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
    check("now format 32, on bank A", proj.format == 32u && PROJ_FORMAT == 32u && proj.bank == 0 && proj.mtime == MTIME_DEF);
    check("a value 0.1 didn't have (Weight's depth) at its default", proj.par[P_GD_0] == dr_param_info(P_GD_0)->def);
    check("its sound is kept", proj.par[DR_GP(0, GP_PITCH)] == 4321);
    check("Colour pink (1) becomes 100, white stays 0",
          proj.par[DR_GP(0, GP_NCOLOR)] == 100 && proj.par[DR_GP(1, GP_NCOLOR)] == 0);
    a1 = scene_get(0, 0);
    a12 = scene_get(0, 11);
    b1 = scene_get(1, 0);
    b4 = scene_get(1, 3);
    check("scenes 1 and 12 are A1 and A12", a1 && a12 && a1[DR_GP(0, GP_PITCH)] == 3000 &&
                                                 a12[DR_GP(0, GP_PITCH)] == 3011);
    check("scenes 13 and 16 are B1 and B4", b1 && b4 && b1[DR_GP(0, GP_PITCH)] == 3012 &&
                                                b4[DR_GP(0, GP_PITCH)] == 3015);
    check("empty ones stay empty (A2, B2, C1)", !scene_get(0, 1) && !scene_get(1, 1) && !scene_get(2, 0));
    check("the scenes' Colours converted, B's too", a1[DR_GP(1, GP_NCOLOR)] == 100 && b4[DR_GP(1, GP_NCOLOR)] == 100);
    check("saved: the project and bank B written", project_save() == 0 && store_n[OBJ_PROJ] == (int)sizeof(project_t) &&
                                                       store_n[OBJ_BANKB] == (int)sizeof(bank_t) && !store_n[OBJ_BANKC]);
    proj.par[DR_GP(0, GP_PITCH)] = 5000;
    check("a scene into C3", scene_put(2, 2) == 0 && scene_get(2, 2) && project_save() == 0 && store_n[OBJ_BANKC]);
    check("no Preset can be saved over (P3, P12)", scene_put(BANK_PRESETS, 2) != 0 &&
                                                       scene_put(BANK_PRESETS, PRESET_LOCKED) != 0 &&
                                                       scene_get(BANK_PRESETS, PRESET_LOCKED)[P_II_OUT] == 1);
    proj.bank = 2;
    project_save();
    {   /* a P3 saved over before 2026-10-19 (OBJ_BANKP): moved to C's first empty slot (C1), P3 the firmware's */
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
                                    scene_get(2, 2)[DR_GP(0, GP_PITCH)] == 5000 && scene_get(0, 11));
    check("... the P3 saved over now in C1, P3 the firmware's", scene_get(2, 0) &&
                                                     scene_get(2, 0)[DR_GP(0, GP_PITCH)] == 4321 &&
                                                     scene_get(BANK_PRESETS, 2)[DR_GP(0, GP_PITCH)] != 4321);
    check("... and saved: C and the emptied Presets object", project_save() == 0 &&
                                                     ((bank_t *)store[OBJ_BANKP])->used == 0 &&
                                                     (((bank_t *)store[OBJ_BANKC])->used & 1u));
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
                  scene_get(2, 0)[P_CHAR] == 77 && scene_get(2, 0)[P_DLY_TYPE] == 0 &&
                  scene_get(2, 0)[P_PRESS_SENS] == 50);
        check("... and C written again in the new layout on the next save",
              project_save() == 0 && store_n[OBJ_BANKC] == (int)sizeof(bank_t));
    }
    old_project(20u);
    check("v0.2's first (20) loads, Colours as they were",
          project_load() == 0 && proj.par[DR_GP(0, GP_NCOLOR)] == 100 && scene_get(1, 0) &&
              scene_get(1, 0)[DR_GP(1, GP_NCOLOR)] == 100);
    old_project(18u);
    check("format 18 gives the defaults", project_load() == -1 && !scene_get(0, 0) && !scene_get(1, 0));
    printf("project: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
