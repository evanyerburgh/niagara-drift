/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift: host test of the safe restore's flash core (firmware/src/restore_core.c, fm1-base's, MPL-2.0) on
 * Felucca's storage (firmware/src/storage.c), against a simulated NOR flash: a used FM-1 (both copies of each object
 * written), a backup of the four objects (the Presets bank absent: emptied), staged and committed with the power cut
 * after every erase and every write in turn, then a power-on (rs_boot) and a load: all old or all new, never a mix;
 * all new once the journal record is down. And: staged, not committed (HOME), nothing changes, the saves go on. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t nor[0x100000];
static int cut = -1;                              /* the power cut: after this many more erases + writes */
static int ops;

static int down(void)
{
    if (cut == 0)
        return 1;
    if (cut > 0)
        cut--;
    ops++;
    return 0;
}
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    if (down())
        return -9;
    memset(nor + off, 0xFF, 4096);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (down())
        return -9;
    if ((off & 0xFFu) + n > 256u) {
        printf("page wrap at %#x\n", off);
        exit(1);
    }
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/app/plat.h"
#include "../firmware/src/storage.c"
#define APP_BACKUP_OBJS {OBJ_PROJ, OBJ_BANKB, OBJ_BANKC, OBJ_BANKP}
#include "../firmware/src/restore_core.c"
static int rs_rec_erase(void) { return st_erase(RS_REC); }
static int rs_rec_prog(uint32_t off, const void *src, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k += 256u)                 /* (a page at a time, as the update's writes) */
        if (st_prog(RS_REC + off + k, (const uint8_t *)src + k, n - k > 256u ? 256u : n - k))
            return -9;
    return 0;
}
static int rs_rec_read(uint32_t off, void *dst, uint32_t n) { return st_read(RS_REC + off, dst, n); }

static int bad;
static void check(const char *what, int ok)
{
    printf("%-62s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

static uint8_t oldv[BU_NOBJ][900], newv[BU_NOBJ][1200];
static const uint32_t OLDN[BU_NOBJ] = {700, 500, 300, 200}, NEWN[BU_NOBJ] = {1100, 900, 600, 0};   /* (P: absent) */
static int is(uint32_t i, const uint8_t *v, uint32_t n)   /* object i loads as v */
{
    static uint8_t got[ST_PAYLOAD_MAX];
    int k = st_load(bu_obj(i), got, sizeof got);
    return k == (int)n && !memcmp(got, v, n);
}
static int stage_all(void)
{
    uint32_t i;
    for (i = 0; i < BU_NOBJ; i++) {
        rs_ent_t *e = &rs_rec.e[i];
        memcpy(bu_buf, newv[i], NEWN[i]);
        e->obj = (uint16_t)bu_obj(i);
        e->len = NEWN[i];
        e->crc = bu_crc(0, bu_buf, NEWN[i]);
        if (rs_stage(e))
            return -1;
    }
    return 0;
}

int main(void)
{
    static uint8_t used[0x100000];
    uint32_t i;
    int c, total = -1, mixed = 0, fresh = 0, again = 1;
    for (i = 0; i < BU_NOBJ; i++) {
        memset(oldv[i], 'a' + (int)i, sizeof oldv[i]);
        memset(newv[i], 'A' + (int)i, sizeof newv[i]);
    }
    memset(nor, 0xFF, sizeof nor);
    for (i = 0; i < BU_NOBJ; i++) {
        st_save(bu_obj(i), oldv[i], OLDN[i]);
        st_save(bu_obj(i), oldv[i], OLDN[i]);   /* (both copies in use, as on a used FM-1) */
    }
    memcpy(used, nor, sizeof nor);
    for (c = 0; total < 0; c++) {               /* the cut after c operations, until the whole restore fits */
        int nn = 0, no = 0, rc;
        memcpy(nor, used, sizeof nor);
        cut = c;
        ops = 0;
        rc = stage_all() ? -1 : rs_commit();
        if (cut > 0 && rc == 0)
            total = ops;                         /* it went through before the cut */
        cut = -1;
        rs_boot();                               /* power on */
        for (i = 0; i < BU_NOBJ; i++) {
            nn += is(i, newv[i], NEWN[i]);
            no += is(i, oldv[i], OLDN[i]);
        }
        if (nn != (int)BU_NOBJ && no != (int)BU_NOBJ)
            mixed++;
        fresh += nn == (int)BU_NOBJ;
        if (nn == (int)BU_NOBJ) {                /* and a save afterwards keeps the rest */
            st_save(OBJ_PROJ, oldv[0], 10);
            for (i = 1; i < BU_NOBJ; i++)
                again &= is(i, newv[i], NEWN[i]);
        }
    }
    printf("    restore: %d erases and writes; all new after a cut at %d of them\n", total, fresh);
    check("a restore cut anywhere: all old or all new, never a mix", mixed == 0 && total > 0 && fresh > 0);
    check("... restored: the Presets bank (not in the backup) empty, a save after keeps it", again);
    memcpy(nor, used, sizeof nor);
    check("staged, not committed (HOME): the old ones still load", stage_all() == 0 && is(0, oldv[0], OLDN[0]) &&
          is(1, oldv[1], OLDN[1]) && is(3, oldv[3], OLDN[3]));
    check("... and a save over a staged copy works", st_save(OBJ_BANKB, newv[2], 50) == 0 && is(1, newv[2], 50));
    rs_boot();
    check("... a power-on after: nothing to finish, nothing changed", is(0, oldv[0], OLDN[0]) && is(2, oldv[2], OLDN[2]));
    printf("%s\n", bad ? "RESTORE TEST FAILED" : "restore test passed");
    return bad != 0;
}
