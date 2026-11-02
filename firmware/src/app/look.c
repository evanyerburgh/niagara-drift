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
    return v == V_PLAY || v == V_PLAY2 || v == V_LIFE || v == V_TONE || v == V_OSC || v == V_SOURCE || v == V_PARTNER ||
           v == V_MOVES || v == V_LFO || v == V_LFO2;
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
static uint32_t lk_ramp888(int g, float t)   /* the same, 0xRRGGBB (to dim it) */
{
    const uint32_t a = LK_RAMP[g][0], b = LK_RAMP[g][1];
    uint32_t c = 0;
    int sh;
    for (sh = 16; sh >= 0; sh -= 8)
        c |= (uint32_t)((float)(a >> sh & 255u) + ((float)(b >> sh & 255u) - (float)(a >> sh & 255u)) * t) << sh;
    return c;
}

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

/* a key named by its note in a box, as the SEL tag's ("F#5"), never its number (Evan, 2026-11-02: the spec's rule
 * 18), its text on the baseline base; returns where it ends */
static const char *const LK_BKEY[11] = {"F#3", "G#3", "A#3", "C#4", "D#4", "F#4", "G#4", "A#4", "C#5", "D#5", "F#5"};
static int32_t lk_key(int32_t x, int32_t base, const char *key)
{
    const int32_t w = text_w(&LK_F_SEL, key) + 6, th = (int32_t)LK_F_SEL.h + 1, ty = base - LK_B(LK_F_SEL) - 1;
    cv_round(x, ty, w, th, 2, LK_LABEL);
    cv_round(x + 1, ty + 1, w - 2, th - 2, 2, LK_BG);
    cv_text(x + 3, ty + 1, &LK_F_SEL, key, LK_INK);
    return x + w;
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
static void lk_bundle_n(int32_t x0, const int16_t *y, int n, int g, int dx, int dy, int ns, float bright)
{                                              /* ns strands over the whole ramp, at bright of its light (0..1) */
    int j, i;
    for (j = 0; j < ns; j++) {                 /* the deepest, furthest back, first */
        const int32_t ox = x0 + dx * (ns - 1 - j), oy = dy * (ns - 1 - j);
        const float t = (float)j / (float)(ns - 1);
        const uint16_t c = bright < 1.0f ? lk_mix(0x07060Bu, lk_ramp888(g, t), bright) : lk_ramp(g, t);
        for (i = 1; i < n; i++) {
            cv_line(ox + 2 * (i - 1), y[i - 1] + oy, ox + 2 * i, y[i] + oy, c);
            cv_line(ox + 2 * (i - 1), y[i - 1] + oy + 1, ox + 2 * i, y[i] + oy + 1, c);
        }
    }
}
static void lk_bundle(int32_t x0, const int16_t *y, int n, int g, int dx, int dy)
{
    lk_bundle_n(x0, y, n, g, dx, dy, 6, 1.0f);
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
    for (g = 0; g < DR_NGROUPS; g++) {         /* the sub's octave beside the drone's name, and its key (Evan,
                                                * 2026-11-02: players didn't know what it was or where) */
        const int32_t base = 2 + LK_B(LK_F_DRONE), x1 = (g ? 130 : 10) + text_w(&LK_F_DRONE, g ? "II" : "I") + 6;
        char t[12];
        dr_param_text(DR_GP(g, GP_SUBOCT), gpar(g, GP_SUBOCT), t);
        lk_key(cv_text(x1, base - LK_B(LK_F_SMALL), &LK_F_SMALL, t, LK_DRONE[g]) + 5, base, LK_BKEY[g ? 6 : 1]);
    }
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 130 : 10;
        const float mix = (float)gpar(g, GP_MIX) * 0.01f, drv = 0.5f * fm_exp2f((float)gpar(g, GP_DRIVE) * 0.05f);
        const float share[2] = {fm_sqrtf(1.0f - mix), fm_sqrtf(mix)}, td = fm_tanhf(drv);
        const int x = gpar(g, GP_WAVE), sw = proj.par[g ? P_SUBW_1 : P_SUBW_0];
        const float pw = pw_of(g);
        for (k = 0; k < 2; k++) {              /* osc above, sub below */
            const int32_t yc = top + lane * k + lane / 2;
            cv_text(x0, yc - (int32_t)LK_F_TAG.h / 2, &LK_F_TAG, k ? "sub" : "osc", LK_LABEL);
            {   /* the wave after its lane's name: four strands (Evan, 2026-11-02: six were too many), dimming
                 * toward its share's 0, most of the change near it; at 0 one thin dim line, plainly off */
                const float v0 = k ? mix : 1.0f - mix, d = 1.0f - v0;
                int16_t y[37];
                for (i = 0; i <= 36; i++) {
                    float ph = 2.0f * (float)i / 36.0f, v;
                    ph -= (float)(int)ph;
                    v = share[k] * (k ? lk_sub_at(sw, ph) : xwave_at(x, pw, ph));
                    y[i] = (int16_t)(yc - 3 * LK_SDY / 2 - (int32_t)(fm_tanhf(drv * v) / td * (float)amp));
                }
                if (v0 < 0.005f)
                    cv_rect(x0 + 28, yc, 72, 1, LK_UNLIT);
                else
                    lk_bundle_n(x0 + 28, y, 37, g, LK_SDX, LK_SDY, 4, 0.35f + 0.65f * (1.0f - d * d * d));
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

/* ---------------------------------------------------- Partner's picture --- */
/* a Lissajous knot a drone (the spec, Evan 2026-10-31): x the oscillator, y its partner; the interval's nearest small
 * ratio p:q sets the knot (unison 1:1, 5th 3:2, octave 2:1, Oct+5 3:1) and the beating rolls it, at q·f2 − p·f1 (a
 * just interval with no Detune stands still). Six strands, the knot just before now, fanned wider the faster it
 * rolls (at most a 14th of a beat). On / off and its key beside the drone's name (Evan, 2026-11-02) */
static float lk_knot_th[DR_NGROUPS];
static uint32_t lk_knot_ms;

static void lk_ratio(float r, int *p, int *q)      /* the nearest p/q to r, q 1..4 */
{
    float best = 1e9f;
    int a, b;
    *p = *q = 1;
    for (b = 1; b <= 4; b++) {
        a = (int)(r * (float)b + 0.5f);
        if (a >= 1 && fm_fabsf((float)a / (float)b - r) < best - 1e-4f) {
            best = fm_fabsf((float)a / (float)b - r);
            *p = a;
            *q = b;
        }
    }
}

static void lk_pic_partner(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 2, bot = LK_PIC_H - (int32_t)LK_F_SMALL.h - 4;
    const int32_t ry = (bot - top) / 2 - 1, cy = top + (bot - top) / 2;
    const uint32_t now = plat_ms();
    const float dt = fm_minf((float)(now - lk_knot_ms) * 0.001f, 0.2f);
    int g, j, i;
    lk_knot_ms = now;
    lk_halves();
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 130 : 10, cx = x0 + 50, base = 2 + LK_B(LK_F_DRONE);
        const int on = gpar(g, GP_PARTNER) != 0, iv = gpar(g, GP_INTERVAL);
        const float semi = DR_IV_SEMI[(unsigned)iv < DR_NINTERVALS ? iv : DR_IV_UNISON];
        const float r = fm_exp2f((semi + (float)gpar(g, GP_DETUNE) * 0.003f) / 12.0f);
        const float f1 = 8.1757989f * fm_exp2f(dr_pitch[g] / 12.0f);
        float beat, fan;
        int p, q, n;
        char t[16], *s;
        lk_ratio(fm_exp2f(semi / 12.0f), &p, &q);
        beat = (float)q * f1 * r - (float)p * f1;     /* beats a second, signed */
        lk_knot_th[g] += 6.2831853f * beat / (float)q * dt;
        lk_knot_th[g] -= 6.2831853f * fm_floorf(lk_knot_th[g] * 0.15915494f);
        fan = 6.2831853f / (float)q * fm_minf(fm_fabsf(beat) * 0.12f, 1.0f / 14.0f) / (float)(LK_STRANDS - 1);
        if (beat < 0.0f)
            fan = -fan;
        lk_key(cv_text((g ? 130 : 10) + text_w(&LK_F_DRONE, g ? "II" : "I") + 6, base - LK_B(LK_F_SMALL),
                       &LK_F_SMALL, on ? "on" : "off", on ? LK_DRONE[g] : LK_LABEL) + 5, base, LK_BKEY[g ? 5 : 0]);
        n = 36 * (p > q ? p : q);
        if (n > 144)
            n = 144;
        for (j = on ? 0 : LK_STRANDS - 1; j < LK_STRANDS; j++) {   /* the oldest first, deep; now last, light */
            const float th = lk_knot_th[g] - fan * (float)(LK_STRANDS - 1 - j);
            const uint16_t c = on ? lk_ramp(g, (float)j / (float)(LK_STRANDS - 1)) : LK_UNLIT;
            int32_t px = 0, py = 0;
            for (i = 0; i <= n; i++) {
                const float sv = (float)q * (float)i / (float)n;
                const int32_t x = cx + (int32_t)(fm_sinf(6.2831853f * sv) * 48.0f);
                const int32_t y = cy - (int32_t)(fm_sinf(6.2831853f * sv * (float)p / (float)q + th) * (float)ry);
                if (i)                         /* 1 px, as the boards (Evan, 2026-11-02: 2 px blended them) */
                    cv_line(px, py, x, y, c);
                px = x;
                py = y;
            }
        }
        if (!on) {
            scopy(t, "partner off");
        } else if (fm_fabsf(beat) < 0.05f) {
            scopy(t, "still");
        } else {
            s = scopy(t, "beats ");
            if (fm_fabsf(beat) < 9.95f)
                s = dec1(fm_fabsf(beat), s);
            else
                itoa_u((uint32_t)(fm_fabsf(beat) + 0.5f), s);
            while (*s)
                s++;
            scopy(s, "/s");
        }
        cv_text(x0, LK_PIC_H - (int32_t)LK_F_SMALL.h - 1, &LK_F_SMALL, t, on ? LK_DRONE[g] : LK_LABEL);
    }
}

/* ------------------------------------------------ Pitch moves' picture --- */
/* one pitch scale, 3.5 px a semitone (the spec: 7 on the boards), drone I's root the home. Left, Shift as a range:
 * the fifths above and below, a tick a semitone, the move as a box from home and how far the gesture has come in it
 * filled. Right, drone II's pitch over the last four Wander jumps, ending at now on the right edge, on the scale's
 * rungs (named from drone I's root); drone II's note now at the top right */
#define LK_WN 54                             /* the trail's points, 2 px apart */
static float lk_wtrail[LK_WN], lk_wlo = -10.0f, lk_wpx = 3.5f;
static uint32_t lk_wtrail_ms;

static void lk_note(float m, char *t)        /* a MIDI note, its nearest name: "C3" */
{
    static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const int k = (int)fm_floorf(m + 0.5f), o = (k < 0 ? k - 11 : k) / 12;
    char *q = scopy(t, NN[(k % 12 + 12) % 12]);
    if (o - 1 < 0)
        *q++ = '-';
    itoa_u((uint32_t)(o - 1 < 0 ? 1 - o : o - 1), q);
}

static const char *lk_rung(float s)          /* a rung's name from drone I's root, or none */
{
    static const struct { int8_t st; const char *n; } R[] = {{-12, "-8ve"}, {-7, "-5th"}, {-5, "-4th"}, {0, "root"},
        {5, "+4th"}, {7, "+5th"}, {12, "+8ve"}, {17, "8ve+4"}, {19, "8ve+5"}, {24, "2 8ves"}};
    unsigned i;
    for (i = 0; i < sizeof R / sizeof R[0]; i++)
        if (fm_fabsf(s - (float)R[i].st) < 0.3f)
            return R[i].n;
    return 0;
}

static void lk_pic_moves(void)
{
    const int32_t top = (int32_t)LK_F_SMALL.h + 6, bot = LK_PIC_H - 4;
    float px, span;
    const int mv = proj.par[P_SHIFT_MOVE] - DR_SHIFT_MID, in = ui.g_amt[DR_G_SHIFT] > 0.0f || proj.par[P_GL_0 + DR_G_SHIFT];
    const uint32_t now = plat_ms();
    float lo, hi = -99.0f, mn = 99.0f, wgap = 25.0f, deg[8];
    int i, o, k, nd;
    int32_t y, ly = -99;
    char t[12];
    {   /* the trail: drone II from drone I's root, sampled so LK_WN points hold four jumps */
        if (proj.par[P_WANDER] > 0) {
            const float a = (float)(proj.par[P_WANDER] - 1) * (1.0f / 99.0f);
            wgap = 0.5f * (25.0f * fm_exp2f(-5.643856f * a) + 30.0f * fm_exp2f(-3.906891f * a));   /* wander_gap's */
        }
        if (now - lk_wtrail_ms >= (uint32_t)(4.0f * wgap * 1000.0f / (float)LK_WN)) {
            lk_wtrail_ms = now;
            for (i = 0; i < LK_WN - 1; i++)
                lk_wtrail[i] = lk_wtrail[i + 1];
        }
        lk_wtrail[LK_WN - 1] = dr_pitch[1] + dr_mac_pitch(1) - dr_pitch[0] - dr_mac_pitch(0);   /* with Wander;
                                                                                * Shift moves both: it cancels */
        for (i = 0; i < LK_WN; i++) {
            hi = fm_maxf(hi, lk_wtrail[i]);
            mn = fm_minf(mn, lk_wtrail[i]);
        }
        lo = fm_minf(mn - 1.5f, -10.0f);         /* Shift's fifths with their names always in, the trail too: */
        hi = fm_maxf(hi + 1.5f, 9.0f);           /* 3.5 px a semitone, closer when it must */
        px = fm_minf(3.5f, (float)(bot - top) / (hi - lo));
        lk_wpx += (px - lk_wpx) * 0.25f;
        px = lk_wpx;
        span = (float)(bot - top) / px;
        if (hi > lo + span)
            lo = hi - span;
        lk_wlo += (lo - lk_wlo) * 0.25f;
        lo = lk_wlo;
    }
#define LK_SY(s) (bot - (int32_t)(((s) - lo) * px + 0.5f))
    cv_text(8, 1, &LK_F_SMALL, "Shift", in ? LK_WHITE : LK_LABEL);   /* "Shift [E5]": the gesture's key */
    lk_key(8 + text_w(&LK_F_SMALL, "Shift") + 5, 1 + LK_B(LK_F_SMALL), "E5");
    lk_note(dr_pitch[1] + dr_mac_pitch(1) + dr_wander_to() - dr_wander(), t);   /* where Wander is going, not the
                                                                                     * notes it glides through (Evan) */
    {   /* the note's name, then a quarter note at the right edge, so it stays put (Evan, 2026-11-02) */
        const int32_t nb = 1 + LK_B(LK_F_SMALL), nx = 226;
        cv_text(nx - 4 - text_w(&LK_F_SMALL, t), 1, &LK_F_SMALL, t, LK_DRONE[1]);
        cv_round(nx - 1, nb - 4, 6, 5, 2, LK_LABEL);
        cv_rect(nx + 3, nb - 14, 2, 12, LK_LABEL);
    }
    /* Shift: the fifths, home, a tick a semitone */
    for (k = -7; k <= 7; k++) {
        y = LK_SY((float)k);
        if (k == 0 || k == 7 || k == -7)
            cv_rect(8, y, 26, 1, k ? LK_LABEL : LK_INK);
        else
            cv_rect(8, y, k == 5 || k == -5 ? 7 : 4, 1, LK_UNLIT);
    }
    cv_text(37, LK_SY(7.0f) - LK_B(LK_F_SMALL) / 2 - 1, &LK_F_SMALL, "+5th", LK_LABEL);
    cv_text(37, LK_SY(0.0f) - LK_B(LK_F_SMALL) / 2 - 1, &LK_F_SMALL, "home", LK_INK);
    cv_text(37, LK_SY(-7.0f) - LK_B(LK_F_SMALL) / 2 - 1, &LK_F_SMALL, "-5th", LK_LABEL);
    if (mv) {                                  /* the move, six 1 px strands from home in the neutral ramp (the
                                                * board's); dim to the move, full as far as the gesture has come */
        const int32_t y0 = LK_SY(0.0f), y1 = LK_SY((float)mv * 0.01f);
        const float at = fm_clampf(dr_gest_pitch(0) * 100.0f / (float)mv, 0.0f, 1.0f);
        const int32_t yf = y0 + (int32_t)((float)(y1 - y0) * at);
        for (k = 0; k < LK_STRANDS; k++) {
            const uint16_t c = lk_mix(0xA29EAEu, 0xEFE6FFu, (float)k / (float)(LK_STRANDS - 1));
            const int32_t x = 15 + 2 * k;
            cv_rect(x, y1 < y0 ? y1 : y0, 1, (y1 < y0 ? y0 - y1 : y1 - y0) + 1, lk_mix(0x07060Bu, 0xA29EAEu, 0.45f));
            if (yf != y0)
                cv_rect(x, yf < y0 ? yf : y0, 1, (yf < y0 ? y0 - yf : yf - y0) + 1, c);
        }
        cv_rect(8, y0, 26, 1, LK_INK);         /* home in front of them (Evan) */
    }
    /* Wander: the scale's rungs, a dotted line each, named from drone I's root where there's room */
    nd = dr_wander_degs(deg);
    for (o = -2; o <= 3; o++)
        for (i = 0; i < nd; i++) {
            const float s = deg[i] + 12.0f * (float)o;
            const char *nm = lk_rung(s);
            if (s < lo - 0.2f || s > lo + span + 0.2f)
                continue;
            y = LK_SY(s);
            for (k = 80; k < 190; k += 4)
                cv_rect(k, y, 2, 1, LK_UNLIT);
            if (nm && nm[1] == '4' && lo + span > s + 2.0f && s - 2.0f > lo &&
                ((int)(s + 120.5f) % 12 == 5 || (int)(s + 120.5f) % 12 == 7))
                for (k = 0; k < nd; k++)       /* a fourth gives way to a fifth two semitones off */
                    if (fm_fabsf(fm_fabsf(deg[k] + 12.0f * (float)o - s) - 2.0f) < 0.3f && lk_rung(deg[k] + 12.0f * (float)o))
                        nm = 0;
            if (nm && (ly < -90 || ly - y >= (int32_t)LK_F_SMALL.h - 6 || y - ly >= (int32_t)LK_F_SMALL.h - 6) &&
                y - LK_B(LK_F_SMALL) / 2 - 1 >= top - 2) {
                cv_text(232 - text_w(&LK_F_SMALL, nm), y - LK_B(LK_F_SMALL) / 2 - 1, &LK_F_SMALL, nm, LK_LABEL);
                ly = y;
            }
        }
    {   /* drone II's trail, six 1 px strands 2 px apart (Evan, 2026-11-02: a pixel apart they blended), the light
         * one on top */
        for (k = LK_STRANDS - 1; k >= 0; k--) {
            const uint16_t c = lk_ramp(1, 1.0f - (float)k / (float)(LK_STRANDS - 1));
            for (i = 1; i < LK_WN; i++)
                cv_line(80 + 2 * (i - 1), LK_SY(lk_wtrail[i - 1]) + 2 * k, 80 + 2 * i, LK_SY(lk_wtrail[i]) + 2 * k, c);
        }
    }
#undef LK_SY
}

/* ------------------------------------------------- the LFO pages' parts --- */
static const uint32_t LK_LFO_TINT[2] = {0xA98BFFu, 0xFF5FD2u};   /* LFO I lilac (OCT-), LFO II pink (OCT+) */

/* a send's slider (its knob, -100..100, or 0..100 one-sided): a thin track the same length on every row, the centre
 * tick behind the fill, the fill from the centre in the row's color, a white mark at the value */
static void lk_slider(int32_t x0, int32_t y, int v, int one_sided, uint16_t c)
{
    const int32_t w = 48, cx = one_sided ? x0 : x0 + w / 2;
    const int32_t m = one_sided ? x0 + v * w / 100 : cx + v * (w / 2) / 100;
    cv_rect(x0, y, w + 1, 1, LK_UNLIT);
    if (!one_sided)
        cv_rect(cx, y - 2, 1, 5, LK_LABEL);
    if (m != cx)
        cv_rect(m > cx ? cx : m, y - 1, m > cx ? m - cx : cx - m, 3, c);
    cv_rect(m - 1, y - 3, 2, 7, v ? LK_WHITE : LK_LABEL);
}

/* a horizontal arrow, 2 px (the fonts have none) */
static void lk_harrow(int32_t x, int32_t y, int32_t w, int right, uint16_t c)
{
    const int32_t tip = right ? x + w : x, d = right ? -4 : 4;
    cv_rect(x, y, w + 1, 2, c);
    cv_line(tip, y, tip + d, y - 4, c);
    cv_line(tip, y + 1, tip + d, y + 5, c);
}

/* one cycle of a wave across w, as the engine plays it (random: this cycle's phases), y in -amp..amp; and the x of
 * the phase now on it */
static void lk_lfo_line(int tap, int32_t x0, int32_t yc, int32_t amp, int n, const float *phs, int lit)
{
    const int sh = proj.par[tap ? P_LFO_W2 : P_LFO_W1];
    const float r0 = sh == DR_LFO_RANDOM ? 1.0f : 0.0f, ph = dr_lfo_ph;
    int16_t y[64];
    int i, at = 0;
    for (i = 0; i < n; i++) {
        const float p = phs ? phs[i] : 0.9999f * (float)i / (float)(n - 1);
        y[i] = (int16_t)(yc - (int32_t)(dr_lfo_wave(tap, sh, r0 + p) * (float)amp));
        if (p <= ph)
            at = i;
    }
    if (!sh) {
        lk_plot(x0, y, n, LK_UNLIT);
        return;
    }
    lk_plot(x0, y, n, lk_mix(0x07060Bu, LK_LFO_TINT[tap], lit ? 0.7f : 0.45f));
    if (at > 0)                                /* brighter just behind the dot */
        lk_plot(x0 + 2 * (at > 6 ? at - 6 : 0), y + (at > 6 ? at - 6 : 0), (at > 6 ? 6 : at) + 1,
                lk_mix(0x07060Bu, LK_LFO_TINT[tap], 1.0f));
    cv_round(x0 + 2 * at - 3, y[at] - 3, 7, 7, 3, LK_WHITE);
}

static int lk_wave_on(int route, int w)        /* a route (DR_LFO_*) takes wave w, and it isn't Off */
{
    return (route == w + 1 || route == DR_LFO_BOTH) && proj.par[w ? P_LFO_W2 : P_LFO_W1] != 0;
}

/* -------------------------------------------------- LFO sends' picture --- */
/* left, LFO I and II, one cycle each in its tint, one dot running along both (one clock); the rate under them, the
 * Movement when it isn't 100 % and the Swell lock in words. Right, five rows, each its slider, linked from the waves
 * its key picks (both: two links); a row at 0, or fed only by an Off wave, gray with its link (the spec, 2026-10-31) */
static void lk_pic_lfo(void)
{
    const int clock = dr_ext_spb > 0.0f, lock = proj.par[P_LFO_LOCK] != 0, moved = proj.par[P_MOVE] != 50;
    const int nl = 1 + moved + lock;
    const int32_t lh = (int32_t)LK_F_SMALL.h - 3, tb = LK_PIC_H - 1 - nl * lh, wh = tb / 2;
    int32_t wy[2];
    int w, d;
    char t[24], *q;
    for (w = 0; w < 2; w++) {                  /* "I Triangle", its wave under it */
        const int sh = proj.par[w ? P_LFO_W2 : P_LFO_W1];
        const int32_t ny = w * wh;
        const int32_t x = cv_text(8, ny, &LK_F_SMALL, w ? "II" : "I", lk_mix(0x07060Bu, LK_LFO_TINT[w], 1.0f));
        dr_param_text(w ? P_LFO_W2 : P_LFO_W1, sh, t);
        cv_text(x + 5, ny, &LK_F_SMALL, t, sh ? LK_INK : LK_LABEL);
        wy[w] = ny + (int32_t)LK_F_SMALL.h + (wh - (int32_t)LK_F_SMALL.h) / 2;
        lk_lfo_line(w, 10, wy[w], (wh - (int32_t)LK_F_SMALL.h) / 2 - 3, 41, 0, 1);
    }
    q = scopy(t, clock ? "cycle " : "rate ");
    dr_param_text(clock ? P_LFO_DIV : P_LFO_RATE, proj.par[clock ? P_LFO_DIV : P_LFO_RATE], q);
    cv_text(8, tb, &LK_F_SMALL, t, LK_INK);
    if (moved) {
        q = scopy(t, "movement ");
        itoa_u((uint32_t)proj.par[P_MOVE] * 2u, q);
        while (*q)
            q++;
        scopy(q, "%");
        cv_text(8, tb + lh, &LK_F_SMALL, t, LK_VIOLET);
    }
    if (lock)
        cv_text(8, tb + lh * (1 + moved), &LK_F_SMALL, proj.par[P_PULSE] ? "restarts with Swell" : "lock waits for Swell",
                LK_CORAL);
    for (d = 0; d < 5; d++) {                  /* the rows: I pitch, I cutoff, II pitch, II cutoff, delay */
        const int g = d / 2, osc = d % 2 == 0;
        const int route = d < 4 ? gpar(g, osc ? GP_OLFO : GP_FLFO) : proj.par[P_DLY_LFO];
        const int depth = d < 4 ? gpar(g, osc ? GP_OLFO_DEPTH : GP_FLFO_DEPTH) : proj.par[P_DLY_LFO_DEPTH];
        const int heard = depth && proj.par[P_MOVE] && (lk_wave_on(route, 0) || lk_wave_on(route, 1));
        const int32_t ny = d * 24, sy = ny + 19, sx = 152;
        const uint16_t col = d < 4 ? LK_DRONE[g] : LK_NEUTRAL;
        if (d < 4)
            scopy(scopy(t, g ? "II " : "I "), osc ? (gpar(g, GP_OLFO_DEST) == DR_TO_XWAVE ? "X-Wave" : "pitch")
                                                  : (gpar(g, GP_FLFO_DEST) == DR_TO_RESO ? "reso" : "cutoff"));
        else
            scopy(t, "delay");
        {
            const int32_t x = cv_text(sx, ny, &LK_F_SMALL, t, heard ? col : LK_LABEL);
            if (d == 4)                        /* its knob is on Mods */
                cv_text(x + 4, ny, &LK_F_SMALL, "Mods", LK_LABEL);
        }
        lk_slider(sx, sy, depth, 0, heard ? col : LK_UNLIT);
        for (w = 0; w < 2; w++)                /* a link from each wave the route takes, in its tint */
            if (route == w + 1 || route == DR_LFO_BOTH)
                cv_line(94, wy[w], sx - 3, sy, heard && lk_wave_on(route, w) ? lk_mix(0x07060Bu, LK_LFO_TINT[w], 0.85f)
                                                                              : LK_UNLIT);
    }
}

/* ------------------------------------------------------- Mods' picture --- */
/* two bands (the spec, 2026-10-31). The LFO band: the clock (SELECT's time in a pill) branching to LFO I and II, each
 * a cycle warped as the engine warps the clock (rmod_k, the phase rate rk · 2^(rm · source)), one dot on both; on the
 * right Speed mod's row ("LFO speed", linked from its Source, a loop arrow back into the clock) and Delay mod's
 * ("delay time", from key 10's LFO). The cross-mod band: what it does and its key ("CROSS-MOD  I moves II's cutoff
 * [F#5]"; Evan, 2026-11-02), then drone I, the amount, drone II, gray when off */
static void lk_pic_mods(void)
{
    const int w1 = proj.par[P_LFO_W1], w2 = proj.par[P_LFO_W2], src = proj.par[P_LFO_RMOD_SRC], rmd = proj.par[P_LFO_RMOD];
    const float rm = (float)rmd * 0.03f, rk = dr_lfo_rmod_k(rm, src, w1, w2);
    const int xm = proj.par[P_XMOD], amt = proj.par[P_XMOD_AMT];
    float phs[37], ph = 0.0f;
    int32_t wy[2], x;
    int i, w;
    char t[24];
    for (i = 0; i < 37; i++) {                 /* the clock's phase across one plain cycle, warped */
        const float v0 = dr_lfo_wave(0, w1, ph), v1 = dr_lfo_wave(1, w2, ph);
        const float s = src == DR_LFO_BOTH ? fm_clampf(v0 + v1, -1.0f, 1.0f) : src == DR_LFO_II ? v1 : v0;
        phs[i] = fm_minf(ph, 0.9999f);
        ph += (1.0f / 36.0f) * rk * fm_exp2f(rm * s);
        ph -= fm_floorf(ph);
    }
    {   /* the clock */
        dr_param_text(dr_ext_spb > 0.0f ? P_LFO_DIV : P_LFO_RATE, proj.par[dr_ext_spb > 0.0f ? P_LFO_DIV : P_LFO_RATE], t);
        x = text_w(&LK_F_TAG, t) + 8;
        cv_round(4, 22, x, (int32_t)LK_F_TAG.h + 2, 6, LK_LABEL);
        cv_round(5, 23, x - 2, (int32_t)LK_F_TAG.h, 5, LK_BG);
        cv_text(8, 23, &LK_F_TAG, t, LK_INK);
        x += 4;
    }
    for (w = 0; w < 2; w++) {                  /* "I · Triangle", its warped cycle */
        const int sh = w ? w2 : w1;
        const int32_t ny = w * 30, xn = x + 10;
        const char *nm = (w ? DR_LFO_W2_NAME : DR_LFO_W1_NAME)[(unsigned)sh < DR_LFO_NSHAPES ? sh : 0];
        scopy(scopy(t, w ? "II \xC2\xB7 " : "I \xC2\xB7 "), nm);
        cv_text(xn, ny, &LK_F_SMALL, t, sh ? lk_mix(0x07060Bu, LK_LFO_TINT[w], 1.0f) : LK_LABEL);
        wy[w] = ny + (int32_t)LK_F_SMALL.h + 2;
        cv_line(x, 29, xn - 2, wy[w], LK_LABEL);      /* the clock branching to both */
        lk_lfo_line(w, xn, wy[w], 4, 37, phs, 1);
    }
    {   /* the rows: Speed mod from its Source, Delay mod from key 10's LFO */
        const int dl = proj.par[P_DLY_LFO], dd = proj.par[P_DLY_LFO_DEPTH];
        const int sp_on = rmd && proj.par[P_MOVE] && (lk_wave_on(src, 0) || lk_wave_on(src, 1));
        const int dl_on = dd && proj.par[P_MOVE] && (lk_wave_on(dl, 0) || lk_wave_on(dl, 1));
        cv_text(156, 0, &LK_F_SMALL, "LFO speed", sp_on ? LK_INK : LK_LABEL);
        lk_slider(158, 22, rmd, 0, sp_on ? LK_VIOLET : LK_UNLIT);
        cv_text(156, 30, &LK_F_SMALL, "delay time", dl_on ? LK_INK : LK_LABEL);
        lk_slider(158, 52, dd, 0, dl_on ? LK_NEUTRAL : LK_UNLIT);
        for (w = 0; w < 2; w++) {
            if (src == w + 1 || src == DR_LFO_BOTH)
                cv_line(x + 86, wy[w], 155, 22, sp_on && lk_wave_on(src, w) ? lk_mix(0x07060Bu, LK_LFO_TINT[w], 0.85f)
                                                                              : LK_UNLIT);
            if (dl == w + 1 || dl == DR_LFO_BOTH)
                cv_line(x + 86, wy[w], 155, 52, dl_on && lk_wave_on(dl, w) ? lk_mix(0x07060Bu, LK_LFO_TINT[w], 0.85f)
                                                                             : LK_UNLIT);
        }
        {   /* the loop: out of LFO speed, round the right edge and under the waves, back up into the clock */
            const uint16_t c = sp_on ? LK_LABEL : LK_UNLIT;
            cv_rect(208, 22, 25, 1, c);
            cv_rect(232, 22, 1, 40, c);
            cv_rect(14, 61, 219, 1, c);
            cv_rect(14, 23 + (int32_t)LK_F_TAG.h + 2, 1, 61 - 23 - (int32_t)LK_F_TAG.h - 2, c);
            cv_line(14, 25 + (int32_t)LK_F_TAG.h, 11, 28 + (int32_t)LK_F_TAG.h, c);
            cv_line(14, 25 + (int32_t)LK_F_TAG.h, 17, 28 + (int32_t)LK_F_TAG.h, c);
        }
    }
    cv_rect(8, 66, 224, 1, LK_RULE);
    {   /* cross-mod: what it does, its key */
        static const char *const DOES[4] = {"off", "I moves II's cutoff", "bend each other", "Snarl"};
        const int on = xm > 0 && amt > 0;
        const uint16_t ci = on ? LK_DRONE[0] : LK_LABEL, cii = on ? LK_DRONE[1] : LK_LABEL;
        int16_t y[34];
        int g;
        const int32_t hb = 69 + LK_B(FONT_B);
        x = cv_text(8, hb - LK_B(FONT_U1), &FONT_U1, "CROSS-MOD", xm ? LK_INK : LK_LABEL);
        x = cv_text(x + 4, hb - LK_B(LK_F_TAG), &LK_F_TAG, DOES[(unsigned)xm < 4u ? xm : 0], LK_LABEL);
        lk_key(x + 4, hb, LK_BKEY[10]);
        cv_text(8, 90, &LK_F_TAG, "drone I", ci);
        cv_text(84, 90, &LK_F_TAG, "amount", on ? LK_INK : LK_LABEL);
        cv_text(144, 90, &LK_F_TAG, xm == DR_XMOD_CUT ? "drone II cutoff" : "drone II", cii);
        for (g = 0; g < 2; g++) {              /* each drone's sound now, small, scaled to its own peak */
            float m = 0.05f;
            for (i = 0; i < DR_SCOPE_N; i++)
                m = fm_maxf(m, fm_fabsf(dr_scope[g][i]));
            for (i = 0; i < 34; i++)
                y[i] = (int16_t)(116 - (int32_t)(dr_scope[g][i * DR_SCOPE_N / 34] / m * 5.0f));
            lk_plot(g ? 166 : 8, y, g ? 34 : 30, g ? cii : ci);
        }
        lk_slider(86, 115, amt, 1, on ? LK_NEUTRAL : LK_UNLIT);
        if (xm >= DR_XMOD_FM) {                /* both drones bent: both ways */
            lk_harrow(68, 110, 12, 1, ci);
            lk_harrow(68, 119, 12, 0, cii);
            lk_harrow(142, 110, 16, 1, ci);
            lk_harrow(142, 119, 16, 0, cii);
        } else {
            lk_harrow(68, 115, 12, 1, ci);
            lk_harrow(142, 115, 16, 1, on ? LK_INK : LK_LABEL);
        }
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
    case V_PARTNER: lk_pic_partner(); break;
    case V_MOVES: lk_pic_moves(); break;
    case V_LFO: lk_pic_lfo(); break;
    case V_LFO2: lk_pic_mods(); break;
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

static void lk_one_of_dots(int32_t cx, int32_t y, int n, int at, int g)   /* Scale's: one dot of n lit */
{
    const int32_t step = 8, x0 = cx - (step * (n - 1)) / 2 - 2;
    int k;
    for (k = 0; k < n; k++) {
        cv_round(x0 + k * step, y, 5, 5, 2, k == at ? (g == LK_G ? LK_INK : LK_DRONE[g]) : LK_UNLIT);
        if (k != at)
            cv_round(x0 + k * step + 1, y + 1, 3, 3, 1, LK_BG);
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
    if (p == P_LFO_RMOD_SRC) {                 /* "LFO I", "LFO II", "LFO I+II" */
        char b[8];
        scopy(b, t);
        scopy(scopy(t, "LFO "), b);
        return;
    }
    if (p == P_WANDER) {                       /* "4.4-9s": no ".0" (the spec) */
        char *q, *o;
        for (q = o = t; *q; q++)
            if (!(q[0] == '.' && q[1] == '0' && (q[2] == '-' || q[2] == 's')))
                *o++ = *q;
            else
                q++;
        *o = 0;
        return;
    }
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

/* a value's second line (the boards'): "just" under a just interval and Shift's interval under its cents (1: under the
 * value, raised); the LFO pages' swing in real units in the meter band, their sliders the meters (2; a leading '>'
 * draws an arrow); Scale's long names on two lines at the value's size (3). 0: none */
static void lk_swing(char *q, float x, const char *unit)   /* "±3.6 oct", "±12c", one decimal below 10 */
{
    q = scopy(q, "\xC2\xB1");
    if (x < 9.95f)
        q = dec1(x, q);
    else {
        itoa_u((uint32_t)(x + 0.5f), q);
        while (*q)
            q++;
    }
    scopy(q, unit);
}

static int lk_line2(int p, int v, char *t, char *t2)
{
    const int gp = p < P_XMOD ? p % GP_N : -1;
    const float mv = (float)proj.par[P_MOVE] * 0.02f, d = (float)v * 0.01f, c = d * fm_fabsf(d) * mv;
    char *q;
    t2[0] = 0;
    if (gp == GP_INTERVAL) {                   /* "+5th just": "just" under it */
        for (q = t; *q && *q != ' '; q++)
            ;
        if (*q) {
            *q = 0;
            scopy(t2, q + 1);
            return 1;
        }
        return 0;
    }
    if (p == P_SHIFT_MOVE) {                   /* plain cents, the fourths and fifths named under them */
        const int ct = v - DR_SHIFT_MID, a = ct < 0 ? -ct : ct;
        if (!ct) {
            scopy(t, "none");
            return 0;
        }
        q = t;
        *q++ = ct < 0 ? '-' : '+';
        itoa_u((uint32_t)a, q);
        while (*q)
            q++;
        scopy(q, "c");
        if (a == 700 || a == 500) {
            scopy(scopy(t2, a == 700 ? "5th " : "4th "), ct < 0 ? "down" : "up");
            return 1;
        }
        return 0;
    }
    if (p == P_WSCALE) {                       /* "Minor / pent", "5ths + / 8ves" */
        char *sp = 0;
        for (q = t; *q; q++)
            if (*q == ' ')
                sp = q;
        if (!sp)
            return 0;
        *sp = 0;
        scopy(t2, sp + 1);
        return 3;
    }
    if (gp == GP_OLFO_DEPTH || gp == GP_FLFO_DEPTH) {   /* LFO sends: the swing, its unit following the key */
        const int g = p / GP_N;
        if (!v)
            scopy(t2, "none");
        else if (gp == GP_OLFO_DEPTH && gpar(g, GP_OLFO_DEST) == DR_TO_XWAVE)
            lk_swing(t2, fm_fabsf(c) * 100.0f, "%");
        else if (gp == GP_OLFO_DEPTH)
            fm_fabsf(c) * 24.0f < 1.0f ? lk_swing(t2, fm_fabsf(c) * 2400.0f, "c") : lk_swing(t2, fm_fabsf(c) * 24.0f, " st");
        else if (gpar(g, GP_FLFO_DEST) == DR_TO_RESO)
            lk_swing(t2, fm_fabsf(d) * mv * 100.0f, "%");
        else
            lk_swing(t2, fm_fabsf(c) * 5.0f, " oct");
        return 2;
    }
    if (p == P_LFO_RMOD) {                     /* "×0.5-2.6": how far the speed swings */
        const float rm = (float)v * 0.03f, rk = dr_lfo_rmod_k(rm, proj.par[P_LFO_RMOD_SRC], proj.par[P_LFO_W1],
                                                             proj.par[P_LFO_W2]);
        if (!v) {
            scopy(t2, "none");
            return 2;
        }
        q = dec1(rk * fm_exp2f(-fm_fabsf(rm)), scopy(t2, "\xC3\x97"));
        dec1(rk * fm_exp2f(fm_fabsf(rm)), scopy(q, "-"));
        return 2;
    }
    if (p == P_DLY_LFO_DEPTH) {                /* up to ±25 % of the time */
        if (!v)
            scopy(t2, "none");
        else
            lk_swing(t2, fm_fabsf(c) * 25.0f, "%");
        return 2;
    }
    if (p == P_XMOD_AMT) {                     /* what it moves, after an arrow: cutoff, pitch, snarl, off */
        const int m = proj.par[P_XMOD];
        scopy(t2, m == DR_XMOD_CUT ? ">cutoff" : m == DR_XMOD_FM ? ">pitch" : m ? ">snarl" : "off");
        return 2;
    }
    if (p == P_LFO_RMOD_SRC)
        return 2;
    return 0;
}

/* SEQ's ruler: ten ticks, lit tall, unlit short, a white mark at the value; centred (Shift's) lit from the middle */
static void lk_ruler(int32_t cx, int32_t y, float frac, int g, int centred)
{
    const float f = fm_clampf(frac, 0.0f, 1.0f);
    const int32_t m = cx - 25 + (int32_t)(f * 50.0f + 0.5f);
    int i;
    for (i = 0; i < 10; i++) {
        const int32_t x = cx - 24 + 5 * i;
        const float at = ((float)i + 0.5f) / 10.0f;
        const int on = centred ? (f >= 0.5f ? at > 0.5f && at < f : at < 0.5f && at > f) : at < f;
        if (on)
            cv_rect(x, y, 2, 7, g == LK_G ? LK_INK : lk_ramp(g, (float)i / 9.0f));
        else
            cv_rect(x, y + 4, 2, 3, LK_UNLIT);
    }
    if (centred)
        cv_rect(cx - 1, y + 9, 1, 2, LK_LABEL);
    cv_rect(m - 1, y - 2, 2, 11, LK_WHITE);
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
        if (VIEW_KNOB[ui.view][i].g < G)       /* a drone's knob may follow another of its settings (PW the wave,
                                                * a send its key) */
            h = hash(h, (uint32_t)gpar(VIEW_KNOB[ui.view][i].g, GP_WAVE) |
                            (uint32_t)gpar(VIEW_KNOB[ui.view][i].g, GP_OLFO_DEST) << 8 |
                            (uint32_t)gpar(VIEW_KNOB[ui.view][i].g, GP_FLFO_DEST) << 9);
    }
    h = hash(h, (uint32_t)proj.par[P_MOVE] | (uint32_t)proj.par[P_XMOD] << 8 | (uint32_t)proj.par[P_LFO_RMOD_SRC] << 12 |
                    (uint32_t)proj.par[P_LFO_W1] << 16 | (uint32_t)proj.par[P_LFO_W2] << 20);
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
        char t[16], t2[16], *sp;
        int pre, mode;
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
        if (ui.view == V_LFO)                  /* the sends: each name underlined in its drone's color */
            cv_rect(cx - 22, 28, 44, 1, LK_DRONE[g]);
        lk_text(p, v, t);
        mode = second ? 0 : lk_line2(p, v, t, t2);
        if (mode == 3) {                       /* two lines at the value's size */
            const felucca_font_t *f2 = text_w(&FONT_N2, t) <= 58 && text_w(&FONT_N2, t2) <= 58 ? &FONT_N2 : &FONT_U1;
            text_c(cx, 27, f2, t, LK_WHITE);
            text_c(cx, 27 + (int32_t)f2->h - 4, f2, t2, LK_WHITE);
            lk_one_of_dots(cx, 72, dr_param_info(p)->hi - lo + 1, v - lo, g);
            continue;
        }
        sp = t;                                /* "D2 +2": the note large, the cents smaller; "177ms": the unit */
        while (*sp && *sp != ' ')
            sp++;
        if (p == P_LFO_RMOD_SRC)               /* "LFO II": one word */
            while (*sp)
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
            if (mode == 1)                     /* raised for the line under it */
                y -= 8;
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
        if (mode == 1)
            text_c(cx, y + (int32_t)fb->h - 6, &LK_F_TAG, t2, LK_WHITE);
        if (mode == 2) {                       /* the swing in the meter band (the sliders are in the picture) */
            const int arrow = t2[0] == '>';
            const char *w2 = t2 + arrow;
            const felucca_font_t *f2 = text_w(&LK_F_TAG, w2) + (arrow ? 12 : 0) <= 54 ? &LK_F_TAG : &FONT_U2;
            const int32_t ww = text_w(f2, w2) + (arrow ? 12 : 0), x2 = cx - ww / 2;
            const uint16_t c2 = !v || !proj.par[P_MOVE] || (p == P_XMOD_AMT && !proj.par[P_XMOD]) ? LK_LABEL
                              : p == P_XMOD_AMT && proj.par[P_XMOD] == DR_XMOD_CUT ? LK_DRONE[1] : LK_WHITE;
            if (arrow)
                lk_harrow(x2, 70, 8, 1, c2);
            cv_text(x2 + (arrow ? 12 : 0), 62 + (int32_t)(LK_B(LK_F_TAG) - (f2 == &FONT_U2 ? LK_B(FONT_U2) : LK_B(LK_F_TAG))), f2,
                    w2, c2);
            continue;
        }
        {   /* the meter, the page's kind */
            const int gp = p < P_XMOD ? p % GP_N : -1;
            const float frac = hi > lo ? (float)(v - lo) / (float)(hi - lo) : 0.0f;
            if (p == P_WSCALE)                 /* Scale's dots, one-word names too (Evan: Dorian had a ruler) */
                lk_one_of_dots(cx, 72, hi - lo + 1, v - lo, g);
            else if (ui.view == V_PARTNER || ui.view == V_MOVES)   /* SEQ's rulers; Shift's from the middle */
                lk_ruler(cx, 70, frac, g, p == P_SHIFT_MOVE);
            else if (gp == GP_NCOLOR && ui.view == V_TONE)   /* a crossfade, white to pink (Evan, 2026-11-02) */
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
