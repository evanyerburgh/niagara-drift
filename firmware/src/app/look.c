/* SPDX-License-Identifier: GPL-3.0-only */
/* 0.3's new look ("Signal light", the screen spec in ~/niagara-drift-notes and the Figma boards): the header, the
 * picture's frame (the banner, the tip, the SEL tag) and the values row, the same on every page that has moved to
 * it; the pages move one PR at a time (LK_PAGE). Sizes as Evan judged them on the FM-1 (2026-11-01): the page's name
 * 22 bold, knob names 20 (18, 17 when long), values 26 / 21, the banner 19, small words 16-18, everything on its
 * line's baseline. Part of ui.c's unity build (included after the pictures and the values row it replaces) */

/* the bands: the header, the picture, the values row (the values row's place is the same on every page) */
#define LK_BAR_H 28
#define LK_VAL_Y 152
#define LK_VAL_H 88
#define LK_PIC_Y LK_BAR_H
#define LK_PIC_H (LK_VAL_Y - LK_BAR_H)

/* the palette (the spec's tokens; RGB565 on the FM-1, rechecked for contrast after rounding) */
#define LK_BG RGB(7, 6, 11)                  /* the ground */
#define LK_LABEL RGB(143, 138, 156)          /* names, sources: 6.0:1 */
#define LK_WHITE RGB(255, 255, 255)          /* values */
#define LK_INK RGB(242, 238, 248)
#define LK_RULE RGB(30, 28, 38)
#define LK_UNLIT RGB(90, 86, 107)            /* an unlit meter step, as a ring: 3.0:1 */
#define LK_BAND RGB(22, 20, 29)              /* the banner's band, a step above the ground */
#define LK_CORAL RGB(255, 143, 115)          /* the SWELL badge */
#define LK_GOLD RGB(255, 208, 74)            /* scenes, Presets, blends, tips */
#define LK_VIOLET RGB(201, 166, 255)         /* the LFO */
#define LK_NEUTRAL RGB(232, 228, 240)        /* the global keys, the system */
#define LK_RED RGB(255, 42, 31)              /* the one caution red */
#define LK_G DR_NGROUPS                       /* a global knob's "drone": the neutral */
static const uint16_t LK_DRONE[DR_NGROUPS + 1] = {RGB(255, 154, 31), RGB(31, 200, 255), RGB(232, 228, 240)};   /* one
                                                                                  * amber, one cyan, the neutral */
static const uint32_t LK_RAMP[DR_NGROUPS + 1][2] = {{0xFF8A1Fu, 0xFFF4D6u}, {0x6F8CFFu, 0xE8FFFBu},
                                                    {0x9A95ABu, 0xE8E4F0u}};
#define LK_PINK 0xFF93D7u                    /* Tone's pink noise (pink, not red: red is caution) */

/* the fonts' roles */
#define LK_F_PAGE FONT_H22                   /* the page's name */
#define LK_F_HALF FONT_T18                   /* its "1/2" */
#define LK_F_TAG FONT_Q17                    /* badges, the sound's key, PLAY's state */
#define LK_F_SOUND FONT_T18                  /* a Preset's name */
#define LK_F_DRONE FONT_T22                  /* I, II */
#define LK_F_SMALL FONT_T18                  /* the interval, a SEL tag's job, the tip's "once" */
#define LK_F_SEL FONT_XS                     /* the word SEL in its box */
#define LK_F_MSG FONT_S                      /* the banner, 19 */
#define LK_B_(f) f##_BASE
#define LK_B(f) LK_B_(f)                     /* a font's baseline from the top of its cell */

/* the pages in the new look (the rest as they were, until their PR) */
static int lk_page(int v)
{
    return v == V_PLAY || v == V_PLAY2 || v == V_LIFE || v == V_TONE || v == V_OSC || v == V_SOURCE;
}
static int lk_on(void) { return lk_page(ui.view) && !keymap_on() && plat_ms() >= ui.glab_until; }

static uint16_t lk_mix(uint32_t a, uint32_t b, float t)   /* 0xRRGGBB a -> b */
{
    int r = (int)((float)(a >> 16) + ((float)(b >> 16) - (float)(a >> 16)) * t);
    int g = (int)((float)(a >> 8 & 255u) + ((float)(b >> 8 & 255u) - (float)(a >> 8 & 255u)) * t);
    int bl = (int)((float)(a & 255u) + ((float)(b & 255u) - (float)(a & 255u)) * t);
    return RGB(r, g, bl);
}
static uint16_t lk_ramp(int g, float t) { return lk_mix(LK_RAMP[g][0], LK_RAMP[g][1], t); }

/* a message's color, by who said it (the callers still pass the old palette's): gold the scenes and saves, amber
 * and cyan the drones, violet the LFO and the gestures, the neutral the rest */
static uint16_t lk_msg_col(uint16_t c)
{
    if (c == K_DRONE[0])
        return LK_DRONE[0];
    if (c == K_DRONE[1])
        return LK_DRONE[1];
    if (c == K_LFO || c == RGB(183, 145, 255))
        return LK_VIOLET;
    if (c == K_GOLD)
        return LK_GOLD;
    return LK_NEUTRAL;
}

/* ------------------------------------------------------------ header --- */
/* left to right the page's name, its "1/2", what's running (REC or LOOP, SWELL, VISIT, II OUT); at the right PLAY's
 * state, then the sound: its key, and a Preset's name only with room for both. When it's full, what gives way, in
 * order: the name, PLAY's state to its short form ("PLAY", as before), the "1/2", SWELL, the key; the page's name
 * never (Evan, 2026-11-01) */
static void lk_bar(void)
{
    uint32_t rec_s = ui.loop == L_REC ? (plat_ms() - ui.loop_t0) / 1000u : 0u;
    int visiting = ui.visit >= 0 && plat_ms() - ui.visit_t >= HOLD_MS, i, drop;
    uint32_t h = hash(hash(2166136261u ^ rec_s, 0x4C42u | ui.view << 16 | ui.playing << 21 | ui.loop << 22 |
                                                    (uint32_t)proj.par[P_PULSE] << 24 | (uint32_t)proj.par[P_II_OUT] << 25),
                      (uint32_t)fade_q() | (uint32_t)(visiting ? ui.visit + 1 : 0) << 8 |
                          (uint32_t)(uint8_t)ui.scene_at << 16 | (uint32_t)proj.bank << 24);
    char name[20], half[6] = "", rec[12] = "REC ", visit[12] = "VISIT ", key[8] = "";
    const char *badge[4], *sound = 0, *state, *st_short;
    uint16_t bcol[4];
    int nb = 0, swell = -1;
    if (h == ui.sig[0])
        return;
    ui.sig[0] = h;
    for (i = 0; VIEW_NAME[ui.view][i] && i < 19; i++) {
        char c = VIEW_NAME[ui.view][i];
        name[i] = (char)(c >= 'a' && c <= 'z' ? c - 32 : c);
    }
    name[i] = 0;
    for (i = 0; i < (int)NPAGE_BTN; i++)       /* which of its button's pages: "1/2" */
        if (on_pages(i) && PAGE_BTN[i].n > 1) {
            half[0] = (char)('1' + ui.view - PAGE_BTN[i].first);
            half[1] = '/';
            half[2] = (char)('0' + PAGE_BTN[i].n);
            half[3] = 0;
        }
    for (i = 0; name[i]; i++)                  /* "Drones 1" with its "1/2": DRONES 1/2 */
        ;
    if (half[0] && i > 2 && name[i - 2] == ' ' && name[i - 1] == half[0])
        name[i - 2] = 0;
    if (ui.loop != L_IDLE) {                   /* "REC 1:05" (white: REC is never red), or LOOP */
        char *q = rec + 4;
        itoa_u(rec_s / 60u, q);
        while (*q)
            q++;
        *q++ = ':';
        *q++ = (char)('0' + rec_s % 60u / 10u);
        *q++ = (char)('0' + rec_s % 10u);
        *q = 0;
        badge[nb] = ui.loop == L_REC ? rec : "LOOP";
        bcol[nb++] = LK_WHITE;
    }
    if (proj.par[P_PULSE] && ui.view != V_PULSE) {
        swell = nb;
        badge[nb] = "SWELL";
        bcol[nb++] = LK_CORAL;
    }
    if (visiting) {
        scene_name(visit + 6, proj.bank, ui.visit);
        badge[nb] = visit;
        bcol[nb++] = LK_GOLD;
    }
    if (proj.par[P_II_OUT]) {
        badge[nb] = "II OUT";
        bcol[nb++] = LK_DRONE[1];
    }
    state = !ui.playing ? (fade_q() > 0 ? "FADING OUT" : "PLAY to start") : fade_q() < 20 ? "FADING IN" : 0;
    st_short = !ui.playing && fade_q() == 0 ? "PLAY" : "FADING";
    if (ui.scene_at >= 0) {
        scene_name(key, proj.bank, ui.scene_at);
        sound = proj.bank == BANK_PRESETS ? preset_name(ui.scene_at) : 0;
    }
    for (drop = 0; drop < 6; drop++) {         /* 0 all; 1 no name; 2 PLAY's state short; 3 no 1/2; 4 no SWELL; 5 no
                                                * key: as drawn below */
        const char *st = drop >= 2 ? st_short : state;
        int32_t x = 8 + text_w(&LK_F_PAGE, name), rx = 232, gap = 0;
        if (half[0] && drop < 3)
            x += 4 + text_w(&LK_F_HALF, half);
        for (i = 0; i < nb; i++)
            if (!(i == swell && drop >= 4))
                x += 12 + text_w(&LK_F_TAG, badge[i]);
        if (state) {
            rx -= text_w(&LK_F_TAG, st);
            gap = 10;
        }
        if (sound && drop < 1) {
            rx -= gap + text_w(&LK_F_SOUND, sound);
            gap = 6;
        }
        if (key[0] && drop < 5)
            rx -= gap + text_w(&LK_F_TAG, key);
        if (rx >= x + 12)
            break;
    }
    if (state && drop >= 2)
        state = st_short;
    cv_begin(240, LK_BAR_H, LK_BG);
    {
        const int32_t bl = (LK_BAR_H - (int32_t)LK_F_PAGE.h) / 2 + LK_B(LK_F_PAGE);   /* the header's baseline */
        int32_t x = cv_text(8, bl - LK_B(LK_F_PAGE), &LK_F_PAGE, name, LK_WHITE), rx = 232, gap = 0;
        if (half[0] && drop < 3)
            x = cv_text(x + 4, bl - LK_B(LK_F_HALF), &LK_F_HALF, half, lk_mix(0x07060Bu, 0xFFFFFFu, 0.6f));
        for (i = 0; i < nb; i++)
            if (!(i == swell && drop >= 4))
                x = cv_text(x + 12, bl - LK_B(LK_F_TAG), &LK_F_TAG, badge[i], bcol[i]);
        if (state) {
            rx -= text_w(&LK_F_TAG, state);
            cv_text(rx, bl - LK_B(LK_F_TAG), &LK_F_TAG, state, LK_LABEL);
            gap = 10;
        }
        if (sound && drop < 1) {
            rx -= gap + text_w(&LK_F_SOUND, sound);
            cv_text(rx, bl - LK_B(LK_F_SOUND), &LK_F_SOUND, sound, LK_WHITE);
            gap = 6;
        }
        if (key[0] && drop < 5 && rx - gap - text_w(&LK_F_TAG, key) >= x + 12)
            cv_text(rx - gap - text_w(&LK_F_TAG, key), bl - LK_B(LK_F_TAG), &LK_F_TAG, key, LK_LABEL);
    }
    cv_blit(0, 0);
}

/* ------------------------------------------- the picture's frame: overlays --- */
/* the banner (a message): a dark band, a bar and the key word (up to the first space) in who said it's color, the
 * rest white; the tip: outlined in gold, a filled TIP tag, "once" at the right (Evan, 2026-11-01: banner B) */
static void lk_banner(void)
{
    const int32_t bh = (int32_t)LK_F_MSG.h + 6, ty = 3 + (bh - (int32_t)LK_F_MSG.h) / 2;
    uint16_t c = ui.msg_tip ? LK_GOLD : lk_msg_col(ui.msg_col);
    char kw[24];
    const char *rest = ui.msg[0];
    int32_t x, n = 0;
    if (ui.msg_tip) {
        const int32_t tw = text_w(&LK_F_SEL, "TIP") + 8;
        cv_round(8, 3, 224, bh, 3, c);
        cv_round(9, 4, 222, bh - 2, 3, LK_BG);
        cv_round(13, 6, tw, bh - 6, 2, c);
        cv_text(17, 3 + (bh - (int32_t)LK_F_SEL.h) / 2, &LK_F_SEL, "TIP", LK_BG);
        x = cv_text(13 + tw + 6, ty, &LK_F_MSG, ui.msg[0], c);
        if (x + 8 + text_w(&LK_F_SMALL, "once") <= 226)
            cv_text(226 - text_w(&LK_F_SMALL, "once"), ty + LK_B(LK_F_MSG) - LK_B(LK_F_SMALL), &LK_F_SMALL, "once",
                    LK_LABEL);
        return;
    }
    cv_round(8, 3, 224, bh, 3, LK_BAND);
    cv_rect(11, 6, 3, bh - 6, c);
    while (*rest && *rest != ' ' && n < 23)
        kw[n++] = *rest++;
    kw[n] = 0;
    x = cv_text(20, ty, &LK_F_MSG, kw, c);
    if (*rest)
        x = cv_text(x, ty, &LK_F_MSG, rest, LK_INK);
    if (ui.msg[1][0])
        cv_text(x + 5, ty, &LK_F_MSG, ui.msg[1], LK_INK);
}

/* a knob's second job with SEL, on the rule above its name (Evan, 2026-11-01): "SEL" outlined in its box and the job
 * after it, centered on the knob's column; SEL held, the box fills. Drawn last in the picture, on a dark patch */
static void lk_sel_tag(int col, const char *job, int held)
{
    const int32_t tw = text_w(&LK_F_SEL, "SEL") + 6, w = tw + 3 + text_w(&LK_F_SMALL, job);
    const int32_t th = (int32_t)LK_F_SEL.h + 1, ty = LK_PIC_H - th - 1, cx = 8 + 56 * col + 28;
    int32_t x = cx - w / 2;
    if (x < 8)
        x = 8;
    if (x + w > 232)
        x = 232 - w;
    cv_rect(x - 3, ty - 2, w + 6, LK_PIC_H - ty + 2, LK_BG);
    cv_round(x, ty, tw, th, 2, held ? LK_INK : LK_LABEL);
    if (!held)
        cv_round(x + 1, ty + 1, tw - 2, th - 2, 2, LK_BG);
    cv_text(x + 3, ty + 1, &LK_F_SEL, "SEL", held ? LK_BG : LK_LABEL);
    cv_text(x + tw + 3, ty + LK_B(LK_F_SEL) + 1 - LK_B(LK_F_SMALL), &LK_F_SMALL, job, LK_INK);
}

/* why a knob does nothing heard ("needs sqr"), gray on the rule above its name, in its drone's half (Evan,
 * 2026-11-02: clear of the meters; its value and meter gray too) */
static void lk_why_tag(int col, const char *why)
{
    const int32_t w = text_w(&LK_F_SMALL, why), ty = LK_PIC_H - (int32_t)LK_F_SMALL.h - 1, cx = 8 + 56 * col + 28;
    const int32_t lo = col < 2 ? 8 : 122, hi = col < 2 ? 118 : 232;
    int32_t x = cx - w / 2;
    if (x + w > hi)
        x = hi - w;
    if (x < lo)
        x = lo;
    cv_rect(x - 3, ty, w + 6, LK_PIC_H - ty, LK_BG);
    cv_text(x, ty, &LK_F_SMALL, why, LK_LABEL);
}

/* an up or down arrow, 2 px, its foot on the baseline (the fonts have none) */
static void lk_arrow(int32_t x, int32_t base, int32_t h, int up, uint16_t c)
{
    const int32_t tip = up ? base - h : base, d = up ? 4 : -4, e = up ? 1 : -1;
    cv_rect(x + 3, base - h, 2, h + 1, c);
    cv_line(x + 3, tip, x, tip + d, c);
    cv_line(x + 4, tip, x + 7, tip + d, c);
    cv_line(x + 3, tip + e, x, tip + d + e, c);
    cv_line(x + 4, tip + e, x + 7, tip + d + e, c);
}

/* the two-drone pages: drone I's half and drone II's, their names at the top */
static void lk_halves(void)
{
    cv_rect(120, 4, 1, LK_PIC_H - 8, LK_RULE);
    cv_text(10, 2, &LK_F_DRONE, "I", LK_DRONE[0]);
    cv_text(130, 2, &LK_F_DRONE, "II", LK_DRONE[1]);
}

/* a line of points, 2 px thick, y given at every 2 px from x0 */
static void lk_plot(int32_t x0, const int16_t *y, int n, uint16_t c)
{
    int i;
    for (i = 1; i < n; i++) {
        cv_line(x0 + 2 * (i - 1), y[i - 1], x0 + 2 * i, y[i], c);
        cv_line(x0 + 2 * (i - 1), y[i - 1] + 1, x0 + 2 * i, y[i] + 1, c);
    }
}

/* a still shape as a bundle of six strands (the Figma boards' look): the light one in front, each deeper one further
 * back by (dx, dy), with a gap between them so it reads as strands, not a band (Evan, 2026-11-02: "the Figma looks
 * great"). The shapes on Osc and Sub · Drive don't move with the music, so the spread is fixed */
#define LK_BDX 2                             /* Osc and Drones 2: each strand 2 px right, 4 down */
#define LK_BDY 4
#define LK_SDX 0                             /* Sub · Drive's lanes are small: 3 px down */
#define LK_SDY 3
static void lk_bundle(int32_t x0, const int16_t *y, int n, int g, int dx, int dy)
{
    int j, i;
    for (j = 0; j < 6; j++) {                  /* the deepest, furthest back, first */
        const int32_t ox = x0 + dx * (5 - j), oy = dy * (5 - j);
        const uint16_t c = lk_ramp(g, (float)j / 5.0f);
        for (i = 1; i < n; i++) {
            cv_line(ox + 2 * (i - 1), y[i - 1] + oy, ox + 2 * i, y[i] + oy, c);
            cv_line(ox + 2 * (i - 1), y[i - 1] + oy + 1, ox + 2 * i, y[i] + oy + 1, c);
        }
    }
}

/* ------------------------------------------------- Drones 1's picture --- */
/* drone II's interval from drone I, by the nearest semitone: "5th", "m10", "2 8ve 4th" (12-tone names: the Tunings'
 * ratios come with Setup 2's PR). No cents (Pitch's value shows them); which way is an arrow (Evan, 2026-11-01) */
static void lk_interval(char *t, int c)
{
    static const char *const N[12] = {"unison", "m2", "M2", "m3", "M3", "4th", "tritone", "5th", "m6", "M6", "m7", "M7"};
    static const char *const N8[12] = {"8ve", "m9", "9th", "m10", "10th", "11th", "aug 11th", "12th", "m13", "13th",
                                       "m14", "14th"};
    int a = c < 0 ? -c : c, st = (a + 50) / 100, oct = st / 12, s = st % 12;
    char o[4];
    if (oct < 2) {
        scopy(t, oct ? N8[s] : N[s]);
        return;
    }
    itoa_u((uint32_t)oct, o);                  /* "2 8ves", "2 8ve 5th", "2 8ve TT" (to fit one line) */
    scopy(scopy(scopy(t, o), s ? " 8ve " : " 8ves"), s == 6 ? "TT" : s ? N[s] : "");
}

/* each drone's last six pictures, drawn as its strands, 2 px: still, they lie on one line; moving (Drift, detune,
 * an LFO on pitch or X-Wave), they fan out. Each wave scaled to its own loudness (fast up, slow down) so a quiet
 * drone isn't flat (Drones 2 shows the levels); between the names' line and the values row */
#define LK_STRANDS 6
static int8_t lk_hist[DR_NGROUPS][LK_STRANDS][DR_SCOPE_N];
static float lk_peak[DR_NGROUPS];

static void lk_pic_drones1(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 2, bot = LK_PIC_H - 2, mid = (top + bot) / 2, half = (bot - top) / 2;
    int g, j, i;
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 128 : 8;
        float m = 0.0f, k;
        for (i = 0; i < DR_SCOPE_N; i++)
            m = fm_maxf(m, fm_fabsf(dr_scope[g][i]));
        lk_peak[g] = m > lk_peak[g] ? m : lk_peak[g] + (m - lk_peak[g]) * 0.03f;
        k = (float)half / fm_maxf(lk_peak[g], 0.05f);
        for (j = 0; j < LK_STRANDS - 1; j++)
            for (i = 0; i < DR_SCOPE_N; i++)
                lk_hist[g][j][i] = lk_hist[g][j + 1][i];
        for (i = 0; i < DR_SCOPE_N; i++) {     /* smoothed as draw_scope's (raw captures jump) */
            sc_disp[g][i] += (dr_scope[g][i] - sc_disp[g][i]) * 0.45f;
            lk_hist[g][LK_STRANDS - 1][i] = (int8_t)fm_clampf(sc_disp[g][i] * k, (float)-half, (float)half);
        }
        for (j = 0; j < LK_STRANDS; j++) {     /* the oldest first, deep; the newest last, light */
            const uint16_t c = lk_ramp(g, (float)j / (float)(LK_STRANDS - 1));
            for (i = 1; i < DR_SCOPE_N; i++) {
                cv_line(x0 + i - 1, mid - lk_hist[g][j][i - 1], x0 + i, mid - lk_hist[g][j][i], c);
                cv_line(x0 + i - 1, mid + 1 - lk_hist[g][j][i - 1], x0 + i, mid + 1 - lk_hist[g][j][i], c);
            }
        }
    }
    cv_rect(120, 4, 1, LK_PIC_H - 8, LK_RULE); /* drone I | drone II */
    cv_text(10, 2, &LK_F_DRONE, "I", LK_DRONE[0]);
    {   /* "II", and its interval from drone I after it, on the names' baseline */
        char t[24];
        const int c1 = proj.par[DR_GP(0, GP_PITCH)],
                  c2 = proj.par[P_PLINK] ? c1 + proj.par[P_PLINK_OFF] : proj.par[DR_GP(1, GP_PITCH)];
        const int32_t x1 = 130 + text_w(&LK_F_DRONE, "II") + 6, iy = 2 + LK_B(LK_F_DRONE) - LK_B(LK_F_SMALL);
        int32_t ax;
        lk_interval(t, c2 - c1);
        cv_text(130, 2, &LK_F_DRONE, "II", LK_DRONE[1]);
        ax = cv_text(x1, iy, &LK_F_SMALL, t, LK_DRONE[1]);
        if (c2 != c1)
            lk_arrow(ax + 5, iy + LK_B(LK_F_SMALL) - 1, 11, c2 > c1, LK_DRONE[1]);
    }
    for (i = 0; i < 4; i += 2)                 /* a pitch knob turning: its fine tuning, SEL's job */
        if (lit(i) && !proj.par[P_PLOCK])
            lk_sel_tag(i, "fine", (int)(ui.btn >> B_SEL & 1u));
}

/* ------------------------------------------------- Drones 2's picture --- */
/* each drone's filter curve (its resonance's peak growing) as six strands, the last six pictures' cutoffs (an LFO or
 * Drift on it fans them), as high as the drone is loud (Evan, 2026-11-01: by Level); its level in dB under it */
static float lk_cut_hist[DR_NGROUPS][LK_STRANDS];

static void lk_pic_drones2(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 4, bot = LK_PIC_H - (int32_t)LK_F_SMALL.h - 6, h = bot - top;
    int g, j, i;
    lk_halves();
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 130 : 10;
        const float l = dr_level[g], db = l > 1e-4f ? 6.0206f * fm_log2f(l) : -99.0f;
        const float lf = fm_clampf((db + 48.0f) / 48.0f, 0.0f, 1.0f);
        char t[12];
        for (j = 0; j < LK_STRANDS - 1; j++)
            lk_cut_hist[g][j] = lk_cut_hist[g][j + 1];
        lk_cut_hist[g][LK_STRANDS - 1] = dr_cut[g];
        for (j = 0; j < LK_STRANDS; j++) {     /* the oldest first, deep; the newest last, light */
            int16_t y[51];
            for (i = 0; i <= 50; i++) {
                const float d = filt_db(g, lk_cut_hist[g][j], 100.0f * (float)i / 50.0f);
                y[i] = (int16_t)(bot - LK_BDY * (j + 1) -
                                 (int32_t)((fm_clampf(d, -36.0f, 18.0f) + 36.0f) / 54.0f * (float)(h - 6 * LK_BDY) * lf));
            }
            lk_plot(x0, y, 51, lk_ramp(g, (float)j / (float)(LK_STRANDS - 1)));
        }
        if (db > -60.0f) {                     /* "-10 dB" */
            char *q = t;
            *q++ = '-';
            itoa_u((uint32_t)(int)(-db + 0.5f), q);
            while (*q)
                q++;
            scopy(q, " dB");
            if (db > -0.5f)
                scopy(t, "0 dB");
        } else {
            scopy(t, ui.playing ? "silent" : "stopped");
        }
        cv_text(x0 + 2, LK_PIC_H - (int32_t)LK_F_SMALL.h - 2, &LK_F_SMALL, t, LK_DRONE[g]);
    }
}

/* ----------------------------------------------------- Osc's picture --- */
/* each drone's wave, one 2 px line in its color (Evan, 2026-11-02: strands are for what moves; here they read as a
 * lo-fi band): X-Wave morphs it sine, triangle, saw, square; Width changes it only near square, as it does the sound.
 * About a third of the picture's height: lower than before, not filling it (Evan, 2026-11-02) */
static const char *lk_unheard(int p);

static void lk_pic_osc(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 4, half = (LK_PIC_H - top - 16) / 2;
    int g, i;
    lk_halves();
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 130 : 10;
        const int x = gpar(g, GP_WAVE);
        const float pw = pw_of(g);
        int16_t y[51];
        for (i = 0; i <= 50; i++) {
            float ph = 2.0f * (float)i / 50.0f;
            ph -= (float)(int)ph;
            y[i] = (int16_t)(top + half - 5 * LK_BDY / 2 - (int32_t)(xwave_at(x, pw, ph) * (float)half * 0.4f));
        }
        lk_bundle(x0, y, 51 - 5 * LK_BDX / 2, g, LK_BDX, LK_BDY);
    }
    for (i = 0; i < 4; i++) {                  /* Width without a square in the wave: why, above it */
        const int p = knob_param(i);
        const char *why = p < 0 || (ui.btn >> B_SEL & 1u) ? 0 : lk_unheard(p);
        if (why)
            lk_why_tag(i, why);
    }
}

/* ------------------------------------------------ Sub · Drive's picture --- */
/* two lanes a drone, like two mixer channels: "osc" the oscillators' wave, "sub" the sub's (the wave SEL + Osc/Sub
 * picks), each as large as its share of the equal-power crossfade (the engine's sqrt(1 - mix), sqrt(mix)), then the
 * drive's soft clip as the engine's (drv = 0.5 * 2^(drive / 20), tanh(drv x) / tanh(drv)): the peaks stay, the shape
 * fattens. One line a lane, 2 px across its edges too (Evan, 2026-11-02: the drawings are small) */
static float lk_sub_at(int w, float ph)        /* the sub's waves: Square, Sine, Triangle, Saw */
{
    switch (w) {
    case 1: return fm_sin_turns(ph);
    case 2: return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
    case 3: return 1.0f - 2.0f * ph;
    default: return ph < 0.5f ? 1.0f : -1.0f;
    }
}

static void lk_pic_source(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 2, bot = LK_PIC_H - (int32_t)LK_F_SEL.h - 6;   /* over the tags */
    const int32_t lane = (bot - top) / 2, amp = lane / 4;   /* half its lane (Evan, 2026-11-02) */
    int g, i, k;
    lk_halves();
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 130 : 10;
        const float mix = (float)gpar(g, GP_MIX) * 0.01f, drv = 0.5f * fm_exp2f((float)gpar(g, GP_DRIVE) * 0.05f);
        const float share[2] = {fm_sqrtf(1.0f - mix), fm_sqrtf(mix)}, td = fm_tanhf(drv);
        const int x = gpar(g, GP_WAVE), sw = proj.par[g ? P_SUBW_1 : P_SUBW_0];
        const float pw = pw_of(g);
        for (k = 0; k < 2; k++) {              /* osc above, sub below */
            const int32_t yc = top + lane * k + lane / 2;
            cv_text(x0, yc - (int32_t)LK_F_TAG.h / 2, &LK_F_TAG, k ? "sub" : "osc", LK_LABEL);
            {                                  /* the wave after its lane's name */
                int16_t y[37];
                for (i = 0; i <= 36 - 5 * LK_SDX / 2; i++) {
                    float ph = 2.0f * (float)i / (36.0f - (float)(5 * LK_SDX / 2)), v;
                    ph -= (float)(int)ph;
                    v = share[k] * (k ? lk_sub_at(sw, ph) : xwave_at(x, pw, ph));
                    y[i] = (int16_t)(yc - 5 * LK_SDY / 2 - (int32_t)(fm_tanhf(drv * v) / td * (float)amp));
                }
                lk_bundle(x0 + 28, y, 37 - 5 * LK_SDX / 2, g, LK_SDX, LK_SDY);
            }
        }
    }
    for (i = 0; i < 4; i += 2) {               /* each Osc/Sub knob's second job: the sub's wave */
        char t[16];
        const int ps = sel_job(ui.view, i), held = (int)(ui.btn >> B_SEL & 1u);
        if (held) {                            /* SEL held: the job it returns to, "mix 40" */
            char *q = scopy(t, "mix ");
            itoa_u((uint32_t)proj.par[knob_own(i)], q);
        } else {
            dr_param_text(ps, proj.par[ps], t);
        }
        lk_sel_tag(i, t, held);
    }
}

/* ---------------------------------------------------- Life's pictures --- */
static uint32_t lk_rnd(uint32_t x);

/* four drawings, one above each knob, each filling its column, all moving; a global page, so the neutral ramp
 * (Character's harmonics are the drones', in theirs) */
static void lk_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* an outline, 1 px */
{
    cv_rect(x, y, w, 1, c);
    cv_rect(x, y + h - 1, w, 1, c);
    cv_rect(x, y, 1, h, c);
    cv_rect(x + w - 1, y, 1, h, c);
}

/* FEEDBACK: rings feeding on themselves, one a 20 (up to five), each shaped by one cycle of drone I's sound as it is
 * now, breathing with the loop's share of the sound (dr_fb); the outer ones fainter. From 70, where it sings, they
 * stop fading and spread to the column's edge, the outer ring a thin red one rippling calmly (six bumps) */
static void lk_life_fb(int32_t cx, int32_t cy)
{
    const int fb = proj.par[P_FEEDBACK], n = (fb + 10) / 20, sing = fb >= 70;
    const float t = (float)plat_ms() * 0.001f, br = 0.8f + 0.2f * fm_clampf(dr_fb, 0.0f, 1.0f);
    const float rmax = sing ? 26.0f : 10.0f + 14.0f * (float)fb / 70.0f;
    float m = 0.05f, w[33];
    int k, j;
    for (j = 0; j < DR_SCOPE_N / 2; j++)
        m = fm_maxf(m, fm_fabsf(dr_scope[0][j]));
    for (j = 0; j <= 32; j++)                  /* one cycle of drone I (the scope spans about two) */
        w[j] = dr_scope[0][(j % 32) * (DR_SCOPE_N / 2) / 32] / m;
    if (!n) {                                  /* off: a small still ring */
        int16_t px = 0, py = 0;
        for (j = 0; j <= 16; j++) {
            const int16_t x = (int16_t)(cx + (int32_t)(3.0f * fm_cosf(0.3926991f * (float)j)));
            const int16_t y = (int16_t)(cy + (int32_t)(3.0f * fm_sinf(0.3926991f * (float)j)));
            if (j)
                cv_line(px, py, x, y, LK_UNLIT);
            px = x;
            py = y;
        }
        return;
    }
    for (k = 1; k <= n; k++) {
        const int red = sing && k == n;
        const float r0 = rmax * br * (float)k / (float)n;
        const uint16_t c = red ? LK_RED : lk_ramp(LK_G, sing ? 1.0f : 1.0f - 0.7f * (float)(k - 1) / (float)n);
        int32_t px = 0, py = 0;
        for (j = 0; j <= 32; j++) {
            const float a = 0.19634954f * (float)j;
            const float r = r0 * (1.0f + 0.14f * w[j]) + (red ? 1.6f * fm_sinf(6.0f * a + 1.5f * t) : 0.0f);
            const int32_t x = cx + (int32_t)(r * fm_cosf(a)), y = cy + (int32_t)(r * fm_sinf(a));
            if (j) {
                cv_line(px, py, x, y, c);
                if (!red)                      /* 2 px, the red ring 1 (thin) */
                    cv_line(px, py + 1, x, y + 1, c);
            }
            px = x;
            py = y;
        }
    }
}

/* a soft pixel: c at a share a (0..1) of its brightness, the brighter of it and what's there kept (on the black
 * ground that is the edge of a line drawn smooth) */
static void lk_soft_px(int32_t x, int32_t y, uint16_t c, float a)
{
    uint32_t i, o, r, g, b;
    y += cv_oy;
    if ((uint32_t)x >= cv_w || y < cv_y0 || y >= cv_y1)
        return;
    i = (uint32_t)y * cv_w + (uint32_t)x;
    o = swap16(cv_px[i]);
    r = (uint32_t)((float)(c >> 11) * a);
    g = (uint32_t)((float)(c >> 5 & 63u) * a);
    b = (uint32_t)((float)(c & 31u) * a);
    if (r < (o >> 11)) r = o >> 11;
    if (g < (o >> 5 & 63u)) g = o >> 5 & 63u;
    if (b < (o & 31u)) b = o & 31u;
    cv_px[i] = swap16((uint16_t)(r << 11 | g << 5 | b));
}

/* a 1 px smooth line, mostly upright (each row split between the two pixels it crosses) */
static void lk_soft_vline(float x0, int32_t y0, float x1, int32_t y1, uint16_t c)
{
    const int32_t n = y1 > y0 ? y1 - y0 : y0 - y1, sy = y1 > y0 ? 1 : -1;
    int32_t k;
    for (k = 0; k <= n; k++) {
        const float x = x0 + (x1 - x0) * (n ? (float)k / (float)n : 0.0f), f = x - fm_floorf(x);
        lk_soft_px((int32_t)fm_floorf(x), y0 + sy * k, c, 1.0f - f);
        lk_soft_px((int32_t)fm_floorf(x) + 1, y0 + sy * k, c, f);
    }
}

/* DRIFT: a flame, nine smooth 1 px strands from one root, opening toward the tip (widest two-thirds up, up to 3.5 px
 * apart) and swaying as one bundle, a little apart in phase, more toward the tip; the ones behind dimmer. The opening
 * and the sway grow evenly with Drift (Evan, 2026-11-02: an even spread from 1 to 100); the motion from the clock
 * (never a rhythm). At 0 one still line */
static void lk_life_drift(int32_t cx, int32_t top, int32_t bot)
{
    const float d = (float)proj.par[P_DRIFT] * 0.01f, t = (float)plat_ms() * 0.001f, spin = t * (0.25f + 0.5f * d);
    const int ns = d > 0.0f ? 9 : 1;
    int j, i;
    for (j = 0; j < ns; j++) {
        const float o = ns > 1 ? (float)j - 4.0f : 0.0f;
        float px = 0.0f;
        int32_t py = 0;
        for (i = 0; i <= 24; i++) {
            const float h = (float)i / 24.0f, a = 0.6981317f * (float)j + 3.0f * h + spin;
            const float sw = 0.6f * fm_sinf(t * 0.53f + 0.35f * (float)j + 4.0f * h) +
                             0.4f * fm_sinf(t * 0.29f + 0.6f * (float)j + 6.5f * h);
            const float x = (float)cx + (0.15f + 0.85f * d) * o * 0.55f * (3.0f + 7.0f * fm_sinf(2.8f * h)) * h +
                            d * sw * 14.0f * h * fm_sqrtf(h);
            const int32_t y = bot - (int32_t)((float)(bot - top) * h);
            if (i)
                lk_soft_vline(px, py, x, y, lk_ramp(LK_G, ns > 1 ? 0.6f + 0.4f * fm_sinf(a) * h : 1.0f));
            px = x;
            py = y;
        }
    }
}

/* AGE: drone I's saw worn as heard (Age's 20 s wear-in, never the knob): its corners round off as the highs go, six
 * strands bending with the wow and fraying with the flutter (both drawn larger than heard, to be seen), hiss specks
 * coming in, dimming as it darkens. Sharp, bright and single at 0 */
static void lk_life_age(int32_t x0, int32_t cy, int32_t top, int32_t bot)
{
    float ph;
    const float a = dr_age_now(&ph), k = 1.0f - 0.75f * a, tw = 6.2831853f * ph;
    const int ns = a > 0.01f ? 6 : 1;
    int j, i;
    for (j = 0; j < ns; j++) {
        const float fl = a * (0.7f * fm_sinf(tw * 5.7f + 1.3f * (float)j) +
                              0.3f * fm_sinf(tw * 8.9f + 2.9f * (float)j));
        const uint16_t c = lk_ramp(LK_G, (1.0f - 0.7f * a) * (ns > 1 ? 0.4f + 0.6f * (float)j / 5.0f : 1.0f));
        float r = 0.0f;
        int32_t px = 0, py = 0;
        for (i = -10; i <= 26; i++) {
            const float x = (float)i / 13.0f;
            float u = x + a * (0.24f / 14.0f * (float)AGE_WOW) * fm_sinf(tw * 0.21f + 0.9f * x) + 0.06f * fl, sv;
            u -= (float)(int)(u + 8.0f) - 8.0f;
            sv = 1.0f - 2.0f * u;
            r += (sv - r) * k;
            if (i >= 0) {
                const int32_t xx = x0 + 2 * i, yy = cy - (int32_t)(r * 20.0f + 3.0f * fl);
                if (i) {
                    cv_line(px, py, xx, yy, c);
                    cv_line(px, py + 1, xx, yy + 1, c);
                }
                px = xx;
                py = yy;
            }
        }
    }
    for (i = 0; i < (int)(a * 30.0f); i++) {   /* hiss */
        const uint32_t hv = lk_rnd((uint32_t)i * 0x9E3779B9u ^ ui.frame / SCOPE_EVERY * 0x85EBCA6Bu);
        cv_rect(x0 + (int32_t)(hv % 52u), top + (int32_t)((hv >> 8) % (uint32_t)(bot - top)), 1, 1, LK_LABEL);
    }
}

/* Character's saw at x (cycles), bowed as its asym bends it (the DC out, as the engine's) */
static float lk_chr_saw(float x, float chr)
{
    float u = x - fm_floorf(x), sv = 1.0f - 2.0f * u;
    return sv + 1.4f * CHR_ASYM * chr * (0.6666667f - sv * sv);
}

/* CHARACTER: ruler to hand (Evan, 2026-11-02). Each drone's wave, two cycles running: a crisp thin line at 0; at 100
 * drawn by hand, thick and soft, its corners round and the stroke trembling with the jitter. Drone I's stroke filled,
 * glowing (its ramp's pale end at the core, amber at the edges); drone II's outlined, in its own ramp */
static void lk_life_char(int32_t x0, int32_t cy, int32_t top, int32_t bot)
{
    const float chr = (float)proj.par[P_CHAR] * 0.01f, k = 1.0f - 0.8f * chr, t = (float)plat_ms() * 0.001f;
    const float amp = (float)(bot - top) * 0.36f;
    int g, i, j;
    for (g = 0; g < DR_NGROUPS; g++) {
        float r = 0.0f;
        int32_t py = 0, pw = 2;
        for (i = -14; i < 52; i++) {
            const float x = (float)i / 26.0f + t * (0.3f + 0.1f * (float)g) + 0.37f * (float)g;
            const uint32_t hv = lk_rnd((uint32_t)(i + 64) * 0x9E3779B9u ^ (uint32_t)g ^
                                       ui.frame / SCOPE_EVERY * 0x85EBCA6Bu);
            const float tr = chr * 1.5f * ((float)(hv & 255u) / 127.5f - 1.0f);
            const int32_t w = 2 + (int32_t)(chr * (6.0f + (float)((hv >> 8) & 1u)));   /* 2 px at 0, 8-9 at 100 */
            r += (lk_chr_saw(x, chr) - r) * k;
            if (i >= 0) {
                const int32_t xx = x0 + i, yy = cy - (int32_t)(r * amp + tr);
                if (g == 0) {
                    for (j = 0; j < w; j++) {  /* the core pale, the edges amber */
                        const float e = fm_fabsf((float)j - 0.5f * (float)(w - 1)) / (0.5f * (float)w);
                        if (i)
                            cv_line(xx - 1, py - pw / 2 + j * pw / w, xx, yy - w / 2 + j, lk_ramp(0, 1.0f - e));
                    }
                } else if (i) {
                    if (w < 4) {               /* thin: one line */
                        cv_line(xx - 1, py, xx, yy, lk_ramp(1, 0.75f));
                        cv_line(xx - 1, py + 1, xx, yy + 1, lk_ramp(1, 0.75f));
                    } else {                   /* thick: its stroke's two edges */
                        cv_line(xx - 1, py - pw / 2, xx, yy - w / 2, lk_ramp(1, 0.6f));
                        cv_line(xx - 1, py + pw / 2, xx, yy + w / 2, lk_ramp(1, 0.6f));
                    }
                }
                py = yy;
                pw = w;
            }
        }
    }
}

static void lk_pic_life(void)
{
    const int32_t top = 6, bot = LK_PIC_H - 6, cy = (top + bot) / 2;
    lk_life_fb(8 + 28, cy);
    lk_life_drift(64 + 28, top, bot);
    lk_life_char(120 + 2, cy, top, bot);
    lk_life_age(176 + 2, cy, top, bot);
}

/* ------------------------------------------------------ Tone's picture --- */
static uint32_t lk_rnd(uint32_t x)            /* a well-mixed hash (the snow, the hiss) */
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    return x ^ x >> 16;
}

/* each drone's noise as TV snow, low to high: as tall as it sounds loud (Evan, 2026-11-02: twice as tall for each
 * 10 dB, so 10, 20 dB under, is a quarter up; the top: noise at 100, as loud as the oscillators), its low end there,
 * then flat at White and falling 3 dB an octave at Pink (Color blends them) over 36 dB, rolled off by the drone's
 * filter as heard; its edge drawn bright. Square specks, white or pink (pink as often as Color says), at 60 %; each
 * changes about once in seven pictures, at its own time, so it shimmers without strobing (no buffer: from a hash) */
static void lk_pic_tone(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 4, base = LK_PIC_H - (int32_t)LK_F_SMALL.h - 4, hh = base - top;
    const uint32_t pic = ui.frame / SCOPE_EVERY;
    const uint16_t white = lk_mix(0x07060Bu, 0xFFFFFFu, 0.6f), pink = lk_mix(0x07060Bu, LK_PINK, 0.6f);
    int g, c;
    lk_halves();
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 128 : 8;
        const int nv = gpar(g, GP_NOISE), cv = gpar(g, GP_NCOLOR);
                const float slope = -3.0f * (float)cv * 0.01f, lv = fm_powf((float)nv * 0.01f, 0.60206f);
        int32_t px = 0, py = 0;
        if (nv > 0)
            for (c = 0; c < 35; c++) {         /* 35 columns of 3 px */
                const float f = 100.0f * (float)c / 34.0f;
                const float db = slope * f * 0.0981378f + filt_db(g, dr_cut[g], f);   /* 0 at the low end */
                const int32_t edge = base - (int32_t)((float)hh * lv * fm_clampf((db + 36.0f) / 36.0f, 0.0f, 1.0f));
                const int32_t x = x0 + 1 + 3 * c;
                int32_t y;
                for (y = base - 2; y >= edge; y -= 3) {
                    const uint32_t id = (uint32_t)c | (uint32_t)(base - y) << 8 | (uint32_t)g << 16;
                    const uint32_t hv = lk_rnd(id * 0x9E3779B9u ^ (pic + lk_rnd(id) % 7u) / 7u * 0x85EBCA6Bu);
                    if ((hv & 255u) < 96u)
                        cv_rect(x, y, 2, 2, (hv >> 8) % 100u < (uint32_t)cv ? pink : white);
                }
                if (c && (py < base || edge < base))   /* the edge, not along the floor */
                    cv_line(px, py, x + 1, edge, lk_ramp(g, 0.85f));
                px = x + 1;
                py = edge;
            }
        cv_text(x0 + 2, LK_PIC_H - (int32_t)LK_F_SMALL.h - 1, &LK_F_SMALL, "low", LK_LABEL);
        cv_text(x0 + 104 - text_w(&LK_F_SMALL, "high"), LK_PIC_H - (int32_t)LK_F_SMALL.h - 1, &LK_F_SMALL, "high",
                LK_LABEL);
    }
}

static void lk_pic(void)
{
    const int msg = plat_ms() < ui.msg_until;
    uint32_t h = hash(hash(2166136261u, 0x4C50u | (uint32_t)msg | (uint32_t)lit(0) << 1 | (uint32_t)lit(2) << 2 |
                                            (ui.btn >> B_SEL & 1u) << 3 | (uint32_t)ui.view << 4),
                      msg ? ui.msg_until : 0);
    if (h == ui.sig[1] && ui.frame % SCOPE_EVERY)
        return;
    ui.sig[1] = h;
    cv_begin(240, LK_PIC_H, LK_BG);
    switch (ui.view) {
    case V_PLAY2: lk_pic_drones2(); break;
    case V_LIFE: lk_pic_life(); break;
    case V_TONE: lk_pic_tone(); break;
    case V_OSC: lk_pic_osc(); break;
    case V_SOURCE: lk_pic_source(); break;
    default: lk_pic_drones1(); break;
    }
    if (msg)
        lk_banner();
    cv_blit(0, LK_PIC_Y);
}

/* ---------------------------------------------------------- values row --- */
/* four columns of 56, each its name (20, else 18, else 17), its value (26 turning, 21 at rest, the unit smaller,
 * shrinking when long) on one baseline, and its meter; the knob turning in its drone's outline */
static void lk_dots(int32_t cx, int32_t y, float frac, int g, int caution)   /* a dot row: 8 of 5 px; caution:
                                                                               * the last one red */
{
    int i, n = (int)(fm_clampf(frac, 0.0f, 1.0f) * 8.0f + 0.5f);
    for (i = 0; i < 8; i++) {
        const int32_t x = cx - 25 + (int32_t)((float)i * 6.5f + 0.5f);
        const uint16_t c = caution && i == 7 ? LK_RED : lk_ramp(g, (float)i / 7.0f);
        if (i < n) {
            cv_round(x, y, 5, 5, 2, c);
        } else {
            cv_round(x, y, 5, 5, 2, caution && i == 7 ? LK_RED : LK_UNLIT);
            cv_round(x + 1, y + 1, 3, 3, 1, LK_BG);
        }
    }
}

static void lk_cells(int32_t cx, int32_t y, float frac, uint32_t c0, uint32_t c1, int caution)   /* ENV's: 8 squares
                                                                                          * of 5; caution: the
                                                                                          * last two red, the
                                                                                          * value rounding up */
{
    const float v = fm_clampf(frac, 0.0f, 1.0f) * 8.0f;
    int i, n = caution && v > 6.0f ? (int)(v + 0.999f) : (int)(v + 0.5f);
    for (i = 0; i < 8; i++) {
        const int32_t x = cx - 25 + (int32_t)((float)i * 6.5f + 0.5f);
        const int red = caution && i >= 6;
        if (i < n)
            cv_rect(x, y, 5, 5, red ? LK_RED : lk_mix(c0, c1, (float)i / 7.0f));
        else
            lk_box(x, y, 5, 5, red ? LK_RED : LK_UNLIT);
    }
}

static void lk_stripes(int32_t cx, int32_t y, float frac, int g, int gray)   /* EDIT's gauge: 10 bars of 3 x 11 */
{
    int i, n = (int)(fm_clampf(frac, 0.0f, 1.0f) * 10.0f + 0.5f);
    for (i = 0; i < 10; i++) {
        const int32_t x = cx - 24 + 5 * i;
        if (i < n) {
            cv_rect(x, y, 3, 11, gray ? LK_UNLIT : lk_ramp(g, (float)i / 9.0f));
        } else {
            cv_rect(x, y + 7, 3, 4, LK_UNLIT);
        }
    }
}

static void lk_ticks4(int32_t cx, int32_t y)    /* under X-Wave's gauge: sine, triangle, saw, square */
{
    int k;
    for (k = 0; k < 4; k++)
        cv_rect(cx - 24 + (int32_t)(k * 46 / 3), y, 1, 3, LK_LABEL);
}

static void lk_xfader(int32_t cx, int32_t y, float frac, int g)   /* Osc/Sub: the oscillators' color to a white mark,
                                                                    * the sub's lighter tint after it */
{
    const int32_t x0 = cx - 25, w = 50, m = x0 + (int32_t)(fm_clampf(frac, 0.0f, 1.0f) * (float)w);
    cv_rect(x0, y + 1, w, 3, lk_ramp(g, 0.75f));
    if (m > x0)
        cv_rect(x0, y + 1, m - x0, 3, LK_DRONE[g]);
    cv_rect(m - 1, y - 2, 2, 9, LK_WHITE);
}

static void lk_xfade_col(int32_t cx, int32_t y, float frac)   /* Tone's Color, as lk_xfader: pink as far as it
                                                                 * goes, white after it, a white mark */
{
    const int32_t x0 = cx - 25, w = 50, m = x0 + (int32_t)(fm_clampf(frac, 0.0f, 1.0f) * (float)w);
    cv_rect(x0, y + 1, w, 3, lk_mix(0x07060Bu, 0xFFFFFFu, 0.6f));
    if (m > x0)
        cv_rect(x0, y + 1, m - x0, 3, lk_mix(0x07060Bu, LK_PINK, 1.0f));
    cv_rect(m - 1, y - 2, 2, 9, LK_WHITE);
}

static void lk_one_of(int32_t cx, int32_t y, int n, int at, int g)   /* one of n segments, the chosen one lit */
{
    const int32_t sw = (50 - 2 * (n - 1)) / n;
    int k;
    for (k = 0; k < n; k++) {
        const int32_t x = cx - 25 + k * (sw + 2);
        cv_round(x, y, sw, 5, 1, k == at ? LK_DRONE[g] : LK_UNLIT);
        if (k != at)
            cv_round(x + 1, y + 1, sw - 2, 3, 1, LK_BG);
    }
}

static const felucca_font_t *lk_name_font(const char *s)
{
    return text_w(&FONT_T20, s) <= 52 ? &FONT_T20 : text_w(&FONT_T18, s) <= 52 ? &FONT_T18 : &FONT_Q17;
}

/* a value's words in the new look: X-Wave's waves short and lower case, as the boards ("saw", "tri", "saw/sqr") */
static void lk_text(int p, int v, char *t)
{
    dr_param_text(p, v, t);
    if (p < P_XMOD && p % GP_N == GP_WAVE) {
        char *q;
        if (t[0] == 'T' && t[1] == 'r' && t[2] == 'i' && t[3] == 'a')
            t[3] = 0;                          /* Triangle: tri */
        for (q = t; *q; q++)
            if (*q >= 'A' && *q <= 'Z')
                *q = (char)(*q + 32);
    }
}

/* a knob that does nothing heard just now, and why: Width (the pulse width) without a square in the wave */
static const char *lk_unheard(int p)
{
    if (p < P_XMOD && p % GP_N == GP_WIDTH && gpar(p / GP_N, GP_WAVE) < 70)
        return "needs sqr";
    return 0;
}

static void lk_values(void)
{
    const int held = (int)(ui.btn >> B_SEL & 1u);
    uint32_t h = hash(2166136261u, 0x4C56u | ui.view << 16 | (uint32_t)(proj.par[P_PLOCK] != 0) << 21 |
                                       (uint32_t)proj.par[P_PLINK] << 22 | (uint32_t)held << 23), i;
    int sh[G];
    for (i = 0; i < G; i++) {                  /* Shift's move of each drone now, cents: the pitch as heard */
        const float c = dr_gest_pitch((int)i) * 100.0f;
        sh[i] = (int)(c + (c >= 0.0f ? 0.5f : -0.5f));
        h = hash(h, (uint32_t)sh[i]);
    }
    for (i = 0; i < 4; i++) {
        const int p = knob_param((int)i);
        h = hash(h, (p < 0 ? 0xFFFFu : (uint32_t)(uint16_t)proj.par[p]) | (uint32_t)lit((int)i) << 16);
        if (VIEW_KNOB[ui.view][i].g < G)       /* a drone's knob may follow another of its settings (PW the wave) */
            h = hash(h, (uint32_t)gpar(VIEW_KNOB[ui.view][i].g, GP_WAVE));
    }
    if (h == ui.sig[2])
        return;
    ui.sig[2] = h;
    cv_begin(240, LK_VAL_H, LK_BG);
    cv_rect(8, 0, 224, 1, LK_RULE);
    if (VIEW_KNOB[ui.view][0].g < G)
        cv_rect(119, 9, 1, 74, LK_RULE);       /* drone I | drone II (not on a global page) */
    for (i = 0; i < 4; i++) {
        const int p = knob_param((int)i), second = held && sel_job(ui.view, (int)i) >= 0;
        const knob_t kn = VIEW_KNOB[ui.view][i];
        const int g = kn.g < G ? kn.g : LK_G, is_pitch = kn.p == GP_PITCH && kn.g < G;   /* a global knob: neutral */
        const int32_t x = 8 + 56 * (int32_t)i, cx = x + 28;
        const char *why = second ? 0 : lk_unheard(p), *name;
        int on = lit((int)i), v, lo, hi;
        const felucca_font_t *fb, *fs, *fn;
        char t[16], *sp;
        int pre;
        int32_t wb, ws, y;
        if (p < 0)
            continue;
        if (is_pitch && proj.par[P_PLOCK])
            on = 0;                            /* PITCH LOCK: the knob does nothing, so it doesn't light */
        v = proj.par[p];
        if (is_pitch && (p == P_PLINK_OFF ? sh[1] != sh[0] : sh[g] != 0)) {
            v += p == P_PLINK_OFF ? sh[1] - sh[0] : sh[g];   /* Shift moving it: the pitch as heard, lit */
            on = 1;
        }
        lo = dr_param_info(p)->lo;
        hi = dr_param_info(p)->hi;
        if (on) {
            cv_round(x + 1, 4, 54, 81, 5, LK_DRONE[g]);
            cv_round(x + 2, 5, 52, 79, 4, LK_BG);
        }
        name = second ? "Sub wave" : card_name(p);
        fn = lk_name_font(name);
        text_c(cx, 7 + (int32_t)(FONT_T20.h - fn->h) / 2, fn, name, on || second ? LK_DRONE[g] : LK_LABEL);
        lk_text(p, v, t);
        sp = t;                                /* "D2 +2": the note large, the cents smaller; "177ms": the unit */
        while (*sp && *sp != ' ')
            sp++;
        pre = *sp == ' ' && t[0] >= 'A' && t[0] <= 'Z' && sp[1] >= '0' && sp[1] <= '9';   /* "Pink 70": the word
                                                                                       * smaller, before it */
        if (!*sp)
            for (sp = t; *sp && !(*sp >= 'a' && *sp <= 'z' && sp > t && sp[-1] >= '0' && sp[-1] <= '9'); sp++)
                ;
        {
            static const felucca_font_t *const BIG[3] = {&FONT_N1, &FONT_N2, &FONT_U1},
                                         *const SMALL[3] = {&FONT_U1, &FONT_U2, &FONT_U2};
            char big[16];
            const int nb = (int)(sp - t), sep = *sp == ' ' ? 3 : 1;
            const char *unit = *sp == ' ' ? sp + 1 : sp;
            int c, f = on ? 0 : 1;
            for (c = 0; c < nb; c++)
                big[c] = t[c];
            big[nb] = 0;
            if (pre) {                         /* swapped: the number large, the word as the unit */
                scopy(big, sp + 1);
                unit = t;
                *sp = 0;
            }
            for (;; f++) {                     /* as large as fits: 26 turning, 21 at rest, else 16 */
                fb = BIG[f];
                fs = SMALL[f];
                wb = text_w(fb, big);
                ws = *unit ? sep + text_w(fs, unit) : 0;
                if (wb + ws <= (f == 0 ? 50 : 58) || f == 2)   /* 26 inside the outline; 21 may touch the column's
                                                                 * edges (a note and its cents, "Ab3 +17"); else 16 */
                    break;
            }
            y = 9 + (int32_t)FONT_T20.h + (62 - (int32_t)FONT_T20.h - (int32_t)fb->h) / 2;
            if (pre) {
                cv_text(cx - (wb + ws) / 2, y + (int32_t)(fb->h - fs->h) - 2, fs, unit, LK_WHITE);
                cv_text(cx - (wb + ws) / 2 + ws, y, fb, big, LK_WHITE);
            } else {
                cv_text(cx - (wb + ws) / 2, y, fb, big, why ? LK_LABEL : LK_WHITE);
                if (ws)
                    cv_text(cx - (wb + ws) / 2 + wb + sep, y + (int32_t)(fb->h - fs->h) - 2, fs, unit,
                            why ? LK_LABEL : LK_WHITE);
            }
        }
        {   /* the meter, the page's kind */
            const int gp = p < P_XMOD ? p % GP_N : -1;
            const float frac = hi > lo ? (float)(v - lo) / (float)(hi - lo) : 0.0f;
            if (gp == GP_NCOLOR && ui.view == V_TONE)   /* a crossfade, white to pink (Evan, 2026-11-02) */
                lk_xfade_col(cx, 70, frac);
            else if (ui.view == V_LIFE || ui.view == V_TONE)   /* ENV's square cells */
                lk_cells(cx, 70, frac, LK_RAMP[g][0], LK_RAMP[g][1], p == P_FEEDBACK);
            else if (second)
                lk_one_of(cx, 70, DR_NSUBW, v, g);
            else if (gp == GP_MIX)
                lk_xfader(cx, 70, frac, g);
            else if (gp == GP_WAVE) {
                lk_stripes(cx, 64, frac, g, 0);
                lk_ticks4(cx, 77);
            } else if (gp == GP_WIDTH || gp == GP_DRIVE)
                lk_stripes(cx, 66, frac, g, why != 0);   /* gray with its value when unheard */
            else
                lk_dots(cx, 70, frac, g, gp == GP_RESO);
        }
    }
    cv_blit(0, LK_VAL_Y);
}

/* a frame of the new look, or false on a page not moved to it yet (or while the key map or a readout has the
 * screen: those come with their PR) */
static int lk_frame(void)
{
    static int was = -1;                       /* into or out of the new look: every band afresh */
    const int on = lk_on();
    if (on != was) {
        was = on;
        ui.sig[0] = ui.sig[1] = ui.sig[2] = 0;
        lcd_fill(0, 0, 240, 240, on ? LK_BG : K_BG);
    }
    if (!on)
        return 0;
    lk_bar();
    lk_pic();
    lk_values();
    return 1;
}
