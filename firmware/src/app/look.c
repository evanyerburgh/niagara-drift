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
static const uint16_t LK_DRONE[DR_NGROUPS] = {RGB(255, 154, 31), RGB(31, 200, 255)};   /* one amber, one cyan */
static const uint32_t LK_RAMP[DR_NGROUPS][2] = {{0xFF8A1Fu, 0xFFF4D6u}, {0x6F8CFFu, 0xE8FFFBu}};

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
static int lk_page(int v) { return v == V_PLAY; }
static int lk_on(void) { return lk_page(ui.view) && !keymap_on() && plat_ms() >= ui.glab_until; }

static uint16_t lk_mix(uint32_t a, uint32_t b, float t)   /* 0xRRGGBB a -> b */
{
    int r = (int)((float)(a >> 16) + ((float)(b >> 16) - (float)(a >> 16)) * t);
    int g = (int)((float)(a >> 8 & 255u) + ((float)(b >> 8 & 255u) - (float)(a >> 8 & 255u)) * t);
    int bl = (int)((float)(a & 255u) + ((float)(b & 255u) - (float)(a & 255u)) * t);
    return RGB(r, g, bl);
}
static uint16_t lk_ramp(int g, float t) { return lk_mix(LK_RAMP[g][0], LK_RAMP[g][1], t); }

/* a message's colour, by who said it (the callers still pass the old palette's): gold the scenes and saves, amber
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
/* the banner (a message): a dark band, a bar and the key word (up to the first space) in who said it's colour, the
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
 * after it, centred on the knob's column; SEL held, the box fills. Drawn last in the picture, on a dark patch */
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
    lk_pic_drones1();
    if (msg)
        lk_banner();
    cv_blit(0, LK_PIC_Y);
}

/* ---------------------------------------------------------- values row --- */
/* four columns of 56, each its name (20, else 18, else 17), its value (26 turning, 21 at rest, the unit smaller,
 * shrinking when long) on one baseline, and its meter; the knob turning in its drone's outline */
static void lk_dots(int32_t cx, int32_t y, float frac, int g)   /* a dot row: 8 steps of 5 px */
{
    int i, n = (int)(fm_clampf(frac, 0.0f, 1.0f) * 8.0f + 0.5f);
    for (i = 0; i < 8; i++) {
        const int32_t x = cx - 25 + (int32_t)((float)i * 6.5f + 0.5f);
        if (i < n) {
            cv_round(x, y, 5, 5, 2, lk_ramp(g, (float)i / 7.0f));
        } else {
            cv_round(x, y, 5, 5, 2, LK_UNLIT);
            cv_round(x + 1, y + 1, 3, 3, 1, LK_BG);
        }
    }
}

static const felucca_font_t *lk_name_font(const char *s)
{
    return text_w(&FONT_T20, s) <= 52 ? &FONT_T20 : text_w(&FONT_T18, s) <= 52 ? &FONT_T18 : &FONT_Q17;
}

static void lk_values(void)
{
    uint32_t h = hash(2166136261u, 0x4C56u | ui.view << 16 | (uint32_t)(proj.par[P_PLOCK] != 0) << 21 |
                                       (uint32_t)proj.par[P_PLINK] << 22), i;
    int sh[G];
    for (i = 0; i < G; i++) {                  /* Shift's move of each drone now, cents: the pitch as heard */
        const float c = dr_gest_pitch((int)i) * 100.0f;
        sh[i] = (int)(c + (c >= 0.0f ? 0.5f : -0.5f));
        h = hash(h, (uint32_t)sh[i]);
    }
    for (i = 0; i < 4; i++) {
        const int p = knob_param((int)i);
        h = hash(h, (p < 0 ? 0xFFFFu : (uint32_t)(uint16_t)proj.par[p]) | (uint32_t)lit((int)i) << 16);
    }
    if (h == ui.sig[2])
        return;
    ui.sig[2] = h;
    cv_begin(240, LK_VAL_H, LK_BG);
    cv_rect(8, 0, 224, 1, LK_RULE);
    cv_rect(119, 9, 1, 74, LK_RULE);           /* drone I | drone II */
    for (i = 0; i < 4; i++) {
        const int p = knob_param((int)i);
        const knob_t kn = VIEW_KNOB[ui.view][i];
        const int g = kn.g < G ? kn.g : 0, is_pitch = kn.p == GP_PITCH && kn.g < G;
        const int32_t x = 8 + 56 * (int32_t)i, cx = x + 28;
        int on = lit((int)i), v, lo, hi;
        const felucca_font_t *fb, *fs, *fn;
        char t[16], *sp;
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
        fn = lk_name_font(card_name(p));
        text_c(cx, 7 + (int32_t)(FONT_T20.h - fn->h) / 2, fn, card_name(p), on ? LK_DRONE[g] : LK_LABEL);
        dr_param_text(p, v, t);
        sp = t;                                /* "D2 +2": the note large, the cents smaller; "177ms": the unit */
        while (*sp && *sp != ' ')
            sp++;
        if (!*sp)
            for (sp = t; *sp && !(*sp >= 'a' && *sp <= 'z' && sp > t && sp[-1] >= '0' && sp[-1] <= '9'); sp++)
                ;
        fb = on ? &FONT_N1 : &FONT_N2;
        fs = on ? &FONT_U1 : &FONT_U2;
        {
            char big[16];
            const int nb = (int)(sp - t);
            const char *unit = *sp == ' ' ? sp + 1 : sp;
            int c;
            for (c = 0; c < nb; c++)
                big[c] = t[c];
            big[nb] = 0;
            wb = text_w(fb, big);
            ws = *unit ? (*sp == ' ' ? 3 : 1) + text_w(fs, unit) : 0;
            if (wb + ws > 50 && on) {          /* too wide: the next size down */
                fb = &FONT_N2;
                fs = &FONT_U2;
                wb = text_w(fb, big);
                ws = *unit ? (*sp == ' ' ? 3 : 1) + text_w(fs, unit) : 0;
            }
            y = 9 + (int32_t)FONT_T20.h + (62 - (int32_t)FONT_T20.h - (int32_t)fb->h) / 2;
            cv_text(cx - (wb + ws) / 2, y, fb, big, LK_WHITE);
            if (ws)
                cv_text(cx - (wb + ws) / 2 + wb + (*sp == ' ' ? 3 : 1), y + (int32_t)(fb->h - fs->h) - 2, fs, unit,
                        LK_WHITE);
        }
        lk_dots(cx, 70, hi > lo ? (float)(v - lo) / (float)(hi - lo) : 0.0f, g);
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
