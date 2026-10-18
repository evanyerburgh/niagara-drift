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
    check("now format 24, on bank A", proj.format == 24u && PROJ_FORMAT == 24u && proj.bank == 0);
    check("a value 0.1 didn't have (Weight's depth) at its default", proj.par[P_GD_WEIGHT] == dr_param_info(P_GD_WEIGHT)->def);
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
    check("the Presets can't be written", scene_put(BANK_PRESETS, 0) != 0 && scene_get(BANK_PRESETS, 0));
    proj.bank = 2;
    project_save();
    memset(&proj, 0, sizeof proj);
    check("reloaded (24): as saved", project_load() == 0 && proj.bank == 2 && scene_get(1, 3) &&
                                         scene_get(1, 3)[DR_GP(0, GP_PITCH)] == 3015 && scene_get(2, 2) &&
                                         scene_get(2, 2)[DR_GP(0, GP_PITCH)] == 5000 && scene_get(0, 11));
    {   /* PR #14's (23): the same layout, the gestures Beat, Heat, Converge, Wash */
        static project_t p23;
        p23 = proj;
        p23.format = 23u;
        p23.par[P_M_BLOOM] = 40;
        p23.par[P_GD_SHIFT] = 70;              /* Heat's depth then; out of Shift's range now */
        p23.par[P_GL_WEIGHT] = 100;            /* Beat latched then */
        memcpy(store[OBJ_PROJ], &p23, sizeof p23);
        store_n[OBJ_PROJ] = (int)sizeof p23;
        check("PR #14's project (23) loads: its sound kept, the gestures at their defaults",
              project_load() == 0 && proj.par[P_M_BLOOM] == 40 && proj.par[P_GD_SHIFT] == 0 &&
                  proj.par[P_GL_WEIGHT] == 0 && proj.par[P_GR_WASH] == dr_param_info(P_GR_WASH)->def);
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
                  scene_get(1, 2)[DR_GP(0, GP_PITCH)] == 2222 && scene_get(1, 2)[P_M_BLOOM] == 0 &&
                  scene_get(1, 2)[P_GD_SHIFT] == dr_param_info(P_GD_SHIFT)->def);
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
        p22.par[P_M_BLOOM] = 60;
        p22.par[P_REV_TYPE] = 1;
        p22.mlive = 2;
        p22.bank = 2;
        p22.scene_used = 1u << 1;
        p22.scene[1][P_GL_WASH] = 100;
        b22.magic = BANK_MAGIC;
        b22.format = 22u;
        b22.used = 1u;
        b22.scene[0][P_CHAR] = 77;
        memset(store_n, 0, sizeof store_n);
        memcpy(store[OBJ_PROJ], &p22, sizeof p22);
        store_n[OBJ_PROJ] = (int)sizeof p22;
        memcpy(store[OBJ_BANKC], &b22, sizeof b22);
        store_n[OBJ_BANKC] = (int)sizeof b22;
        check("PR #12's project (22) loads: its macros, reverb type, bank and live macro kept",
              project_load() == 0 && proj.par[P_M_BLOOM] == 60 && proj.par[P_REV_TYPE] == 1 && proj.bank == 2 &&
                  proj.mlive == 2 && proj.par[P_REV_LEN] == dr_param_info(P_REV_LEN)->def);
        check("... A2 and C1 as they were, the new values and the gestures at their defaults",
              scene_get(0, 1) && scene_get(0, 1)[P_GL_WASH] == 0 && scene_get(2, 0) &&
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
