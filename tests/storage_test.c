/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Host test of firmware/src/storage.c against a simulated NOR flash:
 * erase -> 0xFF, program can only clear bits, page writes must not wrap. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t nor[0x100000];
static int fail_after = -1;            /* torn-write injection: stop after N programs */

static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (fail_after == 0)
        return -9;
    if (fail_after > 0)
        fail_after--;
    if ((off & 0xFFu) + n > 256u) {
        printf("page wrap at %#x\n", off);
        exit(1);
    }
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/app/plat.h"     /* OMNI: the object map */
#include "../firmware/src/storage.c"
#define OBJ_PROJECT0 OBJ_PROJ                  /* OMNI has two objects: the project and the panel */
#define OBJ_SETTINGS OBJ_PANEL

static int check(const char *what, int ok)
{
    printf("%-46s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

int main(void)
{
    char a[600], b[600], got[600];
    int bad = 0, n;
    memset(nor, 0xFF, sizeof nor);
    memset(a, 'A', sizeof a);
    memset(b, 'B', sizeof b);
    bad += check("empty flash loads nothing", st_load(OBJ_PROJECT0, got, sizeof got) < 0);
    bad += check("save A", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns A", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    bad += check("save B", st_save(OBJ_PROJECT0, b, sizeof b) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns B (newer seq)", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    fail_after = 1;                                  /* payload page 1 written, the rest torn */
    st_save(OBJ_PROJECT0, a, sizeof a);
    fail_after = -1;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("torn save keeps B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    fail_after = 3;                                  /* all payload pages, header torn */
    st_save(OBJ_PROJECT0, a, sizeof a);
    fail_after = -1;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("header not written keeps B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    bad += check("save A again", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns A", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    nor[st_sector(OBJ_PROJECT0, 0) + ST_PAYLOAD_OFF + 10] ^= 0x01;   /* bit rot in one copy */
    nor[st_sector(OBJ_PROJECT0, 1) + ST_PAYLOAD_OFF + 10] ^= 0x01;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("both copies corrupt -> nothing", n < 0);
    bad += check("other objects untouched", st_load(OBJ_PANEL, got, sizeof got) < 0);
    bad += check("settings save/load",
                 st_save(OBJ_SETTINGS, "hello", 5) == 0 && st_load(OBJ_SETTINGS, got, 5) == 5 && !memcmp(got, "hello", 5));
    bad += check("CRC-32 is zlib's (check value 0xCBF43926)", st_crc32("123456789", 9) == 0xCBF43926u);
    memset(nor, 0xFF, sizeof nor);
    st_save(OBJ_PANEL, a, sizeof a);
    st_save(OBJ_PANEL, b, sizeof b);              /* B is newer, in the other copy */
    {
        st_hdr_t h;
        int cur = st_current(OBJ_PANEL, &h);
        nor[st_sector(OBJ_PANEL, (uint32_t)cur) + ST_PAYLOAD_OFF + 300] ^= 0x10;   /* rot in the newer copy */
    }
    n = st_load(OBJ_PANEL, got, sizeof got);
    bad += check("newer copy rotten -> the older one loads", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    bad += check("save over the rotten copy", st_save(OBJ_PANEL, b, sizeof b) == 0);
    n = st_load(OBJ_PANEL, got, sizeof got);
    bad += check("... loads the new data", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    nor[st_sector(OBJ_PANEL, 0) + 8] ^= 0x01;    /* both headers broken */
    nor[st_sector(OBJ_PANEL, 1) + 8] ^= 0x01;
    bad += check("both headers broken -> nothing", st_load(OBJ_PANEL, got, sizeof got) < 0);
    {   /* in place (plat_store_buf, project.c's ld): loaded into st_buf, changed there, saved from there; the
         * save's choice of copy must not reload st_buf over the change (the old Presets object, project.c) */
        uint8_t big[3000], back[3000], *q = st_buf;
        uint32_t i;
        for (i = 0; i < sizeof big; i++)
            big[i] = (uint8_t)(i * 7u + 3u);
        bad += check("in place: saved", st_save(OBJ_BANKP, big, sizeof big) == 0);
        memset(st_buf, 0xAA, sizeof st_buf);
        n = st_load(OBJ_BANKP, st_buf, sizeof st_buf);
        bad += check("in place: loads, the rest zeroed", n == (int)sizeof big && !memcmp(q, big, sizeof big) &&
                                                             q[sizeof big] == 0 && q[sizeof st_buf - 1u] == 0);
        q[5] ^= 0x5A;
        big[5] ^= 0x5A;
        bad += check("in place: changed and saved from st_buf", st_save(OBJ_BANKP, st_buf, (uint32_t)n) == 0);
        memset(back, 0, sizeof back);
        bad += check("... the change kept", st_load(OBJ_BANKP, back, sizeof back) == (int)sizeof big &&
                                                !memcmp(back, big, sizeof big));
        nor[st_sector(OBJ_BANKP, 0) + ST_PAYLOAD_OFF + 9] ^= 0x01;   /* a rotten newer copy: the save still picks */
        nor[st_sector(OBJ_BANKP, 1) + ST_PAYLOAD_OFF + 9] ^= 0x01;   /* (both bodies flipped: neither valid) */
        bad += check("rotten bodies: nothing loads", st_load(OBJ_BANKP, back, sizeof back) < 0);
        bad += check("... a save over them", st_save(OBJ_BANKP, big, sizeof big) == 0 &&
                                                 st_load(OBJ_BANKP, back, sizeof back) == (int)sizeof big &&
                                                 !memcmp(back, big, sizeof big));
    }
    {   /* OMNI map: every copy of every object a sector in a data region, none in the sample slots
         * (0xA0000..0xDBFFF) or the key_mac sector (0xFF000), no two the same (F's two copies apart, as Felucca's) */
        uint32_t o, p, c, d, ok = 1;
        for (o = 0; o < OBJ_NOBJ; o++)
            for (c = 0; c < 2; c++) {
                uint32_t a0 = st_sector(o, c), a1 = a0 + 4096;
                int in_data = (a0 >= 0x97000 && a1 <= 0xA0000) || (a0 >= 0xDC000 && a1 <= 0xE0000) ||
                              (a0 >= 0xFC000 && a1 <= 0xFF000);
                if (!in_data || (a0 & 0xFFFu))
                    ok = 0;
                for (p = 0; p <= o; p++)
                    for (d = 0; d < 2; d++)
                        if ((p != o || d != c) && st_sector(p, d) == a0)
                            ok = 0;
            }
        bad += check("objects in data regions, clear of the sample slots", (int)ok);
        bad += check("D, E, F at 0x9D000, 0xDE000, 0x9F000 / 0xFE000",
                     st_sector(OBJ_BANKD, 0) == 0x9D000u && st_sector(OBJ_BANKE, 1) == 0xDF000u &&
                         st_sector(OBJ_BANKF, 0) == 0x9F000u && st_sector(OBJ_BANKF, 1) == 0xFE000u);
    }
    printf("%s\n", bad ? "STORAGE TEST FAILED" : "storage test passed");
    return bad != 0;
}
