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

static void old_project(uint32_t format)       /* 16 scenes, as 0.1 saved them; scene n's pitch 3000 + n */
{
    static project_old_t o;
    int i, k;
    memset(&o, 0, sizeof o);
    o.magic = PROJ_MAGIC;
    o.format = format;
    for (k = 0; k < DR_NPARAMS; k++)
        o.par[k] = dr_param_info(k)->def;
    o.par[DR_GP(0, GP_PITCH)] = 4321;
    o.par[DR_GP(0, GP_NCOLOR)] = format == 19u ? 1 : 100;
    o.par[DR_GP(1, GP_NCOLOR)] = 0;
    o.leds = 1;
    o.scene_used = 1u << 0 | 1u << 11 | 1u << 12 | 1u << 15;
    for (i = 0; i < OLD_NSCENES; i++) {
        for (k = 0; k < DR_NPARAMS; k++)
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
    check("now format 21, on bank A", proj.format == 21u && PROJ_FORMAT == 21u && proj.bank == 0);
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
    check("reloaded (21): as saved", project_load() == 0 && proj.bank == 2 && scene_get(1, 3) &&
                                         scene_get(1, 3)[DR_GP(0, GP_PITCH)] == 3015 && scene_get(2, 2) &&
                                         scene_get(2, 2)[DR_GP(0, GP_PITCH)] == 5000 && scene_get(0, 11));
    old_project(20u);
    check("v0.2's first (20) loads, Colours as they were",
          project_load() == 0 && proj.par[DR_GP(0, GP_NCOLOR)] == 100 && scene_get(1, 0) &&
              scene_get(1, 0)[DR_GP(1, GP_NCOLOR)] == 100);
    old_project(18u);
    check("format 18 gives the defaults", project_load() == -1 && !scene_get(0, 0) && !scene_get(1, 0));
    printf("project: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
