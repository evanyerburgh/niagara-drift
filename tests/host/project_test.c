/* SPDX-License-Identifier: GPL-3.0-only */
/* The project's format change from 0.1 (19) to v0.2 (20): a 0.1 project loads with its sound and its scenes,
 * the noise Colour (0 / 1 then) turned into the blend (0 / 100). Any other format still gives the defaults. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/app.h"

static project_t stored;
static int stored_n;
int plat_store_load(uint32_t obj, void *dst, uint32_t max)
{
    (void)obj;
    if (!stored_n || (uint32_t)stored_n > max)
        return -1;
    memcpy(dst, &stored, (size_t)stored_n);
    return stored_n;
}
int plat_store_save(uint32_t obj, const void *src, uint32_t len) { (void)obj; (void)src; (void)len; return 0; }

#include "../../firmware/src/app/project.c"

static int fails;
static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    fails += !ok;
}

int main(void)
{
    int i;
    project_defaults();                        /* a 0.1 project: format 19, Colour 0 / 1, two scenes */
    proj.format = 19u;
    proj.par[DR_GP(0, GP_NCOLOR)] = 1;
    proj.par[DR_GP(1, GP_NCOLOR)] = 0;
    proj.par[DR_GP(0, GP_PITCH)] = 4321;
    proj.scene_used = 1u << 0 | 1u << 15;
    for (i = 0; i < DR_NPARAMS; i++)
        proj.scene[0][i] = proj.scene[15][i] = proj.par[i];
    proj.scene[0][DR_GP(1, GP_NCOLOR)] = 1;
    proj.scene[15][DR_GP(0, GP_NCOLOR)] = 0;
    stored = proj;
    stored_n = (int)sizeof proj;
    check("a 0.1 project loads", project_load() == 0);
    check("its format is now 20", proj.format == PROJ_FORMAT && PROJ_FORMAT == 20u);
    check("its sound is kept", proj.par[DR_GP(0, GP_PITCH)] == 4321);
    check("Colour pink (1) becomes 100, white stays 0",
          proj.par[DR_GP(0, GP_NCOLOR)] == 100 && proj.par[DR_GP(1, GP_NCOLOR)] == 0);
    check("both scenes kept", proj.scene_used == (1u << 0 | 1u << 15));
    check("the scenes' Colours converted",
          proj.scene[0][DR_GP(1, GP_NCOLOR)] == 100 && proj.scene[15][DR_GP(0, GP_NCOLOR)] == 0 &&
              proj.scene[0][DR_GP(0, GP_NCOLOR)] == 100);
    stored.format = 18u;                       /* older: the defaults, as before */
    check("format 18 gives the defaults", project_load() == -1 && proj.scene_used == 0);
    printf("project: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
