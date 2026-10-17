/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift project (from FoMni's): one flash object (OBJ_PROJ). A project of another format, or none,
 * gives the defaults. Part of the unity build, after app.h. */
project_t proj;

void project_defaults(void)
{
    int i;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    for (i = 0; i < DR_NPARAMS; i++)
        proj.par[i] = dr_param_info(i)->def;
    proj.leds = 1;
}

static int project_valid(void)
{
    int i;
    if (proj.magic != PROJ_MAGIC)
        return 0;
    if (proj.format == 19u) {                   /* 0.1: the noise Colour was 0 / 1, now 0..100 (white .. pink) */
        int g, k;
        for (g = 0; g < DR_NGROUPS; g++) {
            proj.par[DR_GP(g, GP_NCOLOR)] = proj.par[DR_GP(g, GP_NCOLOR)] ? 100 : 0;
            for (k = 0; k < NSCENES; k++)
                proj.scene[k][DR_GP(g, GP_NCOLOR)] = proj.scene[k][DR_GP(g, GP_NCOLOR)] ? 100 : 0;
        }
        proj.format = PROJ_FORMAT;
    }
    if (proj.format != PROJ_FORMAT)
        return 0;
    if (proj.par[P_LFO_RATE] < 0)                /* slower than 50 s, as it went before 2026-10-14: the slowest */
        proj.par[P_LFO_RATE] = 0;
    for (i = 0; i < NSCENES; i++)
        if (proj.scene[i][P_LFO_RATE] < 0)
            proj.scene[i][P_LFO_RATE] = 0;
    for (i = 0; i < DR_NPARAMS; i++) {
        const dr_param_t *p = dr_param_info(i);
        if (proj.par[i] < p->lo || proj.par[i] > p->hi)
            return 0;
    }
    for (i = 0; i < NSCENES; i++) {             /* a scene out of range is dropped, not the project */
        int k;
        if (!(proj.scene_used >> i & 1u))
            continue;
        for (k = 0; k < DR_NPARAMS; k++) {
            const dr_param_t *p = dr_param_info(k);
            if (proj.scene[i][k] < p->lo || proj.scene[i][k] > p->hi) {
                proj.scene_used &= (uint16_t)~(1u << i);
                break;
            }
        }
    }
    return 1;
}

int project_load(void)
{
    int n;
    memset(&proj, 0, sizeof proj);
    n = plat_store_load(OBJ_PROJ, &proj, sizeof proj);
    if (n >= 8 && project_valid())
        return 0;
    project_defaults();
    return -1;
}

int project_save(void) { return plat_store_save(OBJ_PROJ, &proj, sizeof proj); }

void project_apply(void)
{
    int i;
    for (i = 0; i < DR_NPARAMS; i++)
        dr_set(i, proj.par[i]);
}
