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
#define LK_FX RGB(120, 173, 255)              /* the FX blue (the Delay board's left echoes): freeze */
#define LK_SWELL RGB(255, 169, 146)          /* the Swell page's selected knob: its outline and name */
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
           v == V_MOVES || v == V_LFO || v == V_LFO2 || v == V_PULSE || v == V_FX || v == V_FX2 || v == V_SETUP ||
           v == V_SETUP2;
}
static int lk_on(void) { return lk_page(ui.view) && !keymap_on(); }

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

static int32_t lk_ktext(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c);
static int32_t lk_ktext_w(const felucca_font_t *f, const char *s);

/* ------------------------------------------------------------ header --- */
/* left to right the page's name, its "1/2", what's running (REC or LOOP, SWELL, VISIT, II OUT); at the right PLAY's
 * state, then the sound: its key, and a Preset's name only with room for both. When it's full, what gives way, in
 * order: the name, PLAY's state to its short form ("PLAY", as before), the "1/2", SWELL, the key; the page's name
 * never (Evan, 2026-11-01) */
static void lk_bar(void)
{
    uint32_t rec_s = ui.loop == L_REC ? (plat_ms() - ui.loop_t0) / 1000u : 0u;
    int visiting = ui.visit >= 0 && plat_ms() - ui.visit_t >= HOLD_MS, i, drop;
    uint32_t h = hash(hash(2166136261u ^ rec_s ^ (ui.view == V_PULSE ? (uint32_t)ui_clock_bpm() << 20 : 0u), 0x4C42u | ui.view << 16 | ui.playing << 21 | ui.loop << 22 |
                                                    (uint32_t)proj.par[P_PULSE] << 24 | (uint32_t)proj.par[P_II_OUT] << 25),
                      (uint32_t)fade_q() | (uint32_t)(visiting ? ui.visit + 1 : 0) << 8 |
                          (uint32_t)(uint8_t)ui.scene_at << 16 | (uint32_t)proj.bank << 24 |
                          (uint32_t)(ui.view == V_PULSE && dr_ext_spb > 0.0f) << 28 |
                          (uint32_t)(ui.view == V_FX && proj.par[P_DLY_REPEATS] > 85) << 29 |
                          (uint32_t)(ui.view == V_FX2 && proj.par[P_REV_LEN] >= 100) << 30);
    char name[20], half[6] = "", rec[12] = "REC ", visit[12] = "VISIT ", key[8] = "";
    const char *badge[4], *sound = 0, *state, *st_short;
    uint16_t bcol[4], st_col = LK_LABEL, name_col = LK_WHITE;
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
    if (ui.view == V_FX2 && proj.par[P_REV_LEN] >= 100) {   /* Inf: the tail holds (the spec) */
        badge[nb] = "freeze";
        bcol[nb++] = LK_FX;
    }
    if (ui.view == V_FX && proj.par[P_DLY_REPEATS] > 85) {   /* the echoes stop shrinking (the spec) */
        badge[nb] = "runaway";
        bcol[nb++] = LK_RED;
    }
    if (proj.par[P_II_OUT]) {
        badge[nb] = "II OUT";
        bcol[nb++] = LK_DRONE[1];
    }
    state = !ui.playing ? (fade_q() > 0 ? "FADING OUT" : "[PLAY] to start") : fade_q() < 20 ? "FADING IN" : 0;
    st_short = !ui.playing && fade_q() == 0 ? "[PLAY]" : "FADING";
    if (ui.scene_at >= 0) {
        scene_name(key, proj.bank, ui.scene_at);
        sound = proj.bank == BANK_PRESETS ? preset_name(ui.scene_at) : 0;
    }
    if (ui.view == V_PULSE) {                  /* SWELL alone, in its coral; at the right whether it's on (the spec) */
        static char bpm[20];
        if (proj.par[P_PULSE] && dr_ext_spb > 0.0f) {
            char *q = scopy(bpm, "on \xC2\xB7 ");
            itoa_u((uint32_t)ui_clock_bpm(), q);
            while (*q)
                q++;
            scopy(q, " bpm");
        }
        state = !proj.par[P_PULSE] ? "off \xC2\xB7 tap [ARP]" : dr_ext_spb > 0.0f ? bpm : "on";
        st_short = proj.par[P_PULSE] ? "on" : "off";
        st_col = proj.par[P_PULSE] ? LK_SWELL : LK_LABEL;
        name_col = LK_CORAL;
        sound = 0;
        key[0] = 0;
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
            rx -= lk_ktext_w(&LK_F_TAG, st);
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
        int32_t x = cv_text(8, bl - LK_B(LK_F_PAGE), &LK_F_PAGE, name, name_col), rx = 232, gap = 0;
        if (half[0] && drop < 3)
            x = cv_text(x + 4, bl - LK_B(LK_F_HALF), &LK_F_HALF, half, lk_mix(0x07060Bu, 0xFFFFFFu, 0.6f));
        for (i = 0; i < nb; i++)
            if (!(i == swell && drop >= 4))
                x = cv_text(x + 12, bl - LK_B(LK_F_TAG), &LK_F_TAG, badge[i], bcol[i]);
        if (state) {
            rx -= lk_ktext_w(&LK_F_TAG, state);
            lk_ktext(rx, bl - LK_B(LK_F_TAG), &LK_F_TAG, state, st_col);
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
#define LK_BANNER_PAD 8                 /* past a banner's words */
static void lk_banner(void)
{
    const int32_t bh = (int32_t)LK_F_MSG.h + 6, ty = 3 + (bh - (int32_t)LK_F_MSG.h) / 2;
    uint16_t c = ui.msg_tip ? LK_GOLD : lk_msg_col(ui.msg_col);
    char kw[24];
    const char *rest = ui.msg[0];
    int32_t x, n = 0;
    /* each banner only as wide as its words and a little padding, so the picture shows past it (Evan, 2026-11-03) */
    if (ui.msg_tip) {
        const int32_t tw = text_w(&LK_F_SEL, "TIP") + 8, ow = text_w(&LK_F_SMALL, "once");
        int32_t end = 13 + tw + 6 + lk_ktext_w(&LK_F_MSG, ui.msg[0]), w;
        const int once = end + 8 + ow + LK_BANNER_PAD <= 232;
        if (once)
            end += 8 + ow;
        w = end + LK_BANNER_PAD - 8 > 224 ? 224 : end + LK_BANNER_PAD - 8;
        cv_round(8, 3, w, bh, 3, c);
        cv_round(9, 4, w - 2, bh - 2, 3, LK_BG);
        cv_round(13, 6, tw, bh - 6, 2, c);
        cv_text(17, 3 + (bh - (int32_t)LK_F_SEL.h) / 2, &LK_F_SEL, "TIP", LK_BG);
        lk_ktext(13 + tw + 6, ty, &LK_F_MSG, ui.msg[0], c);
        if (once)
            cv_text(end - ow, ty + LK_B(LK_F_MSG) - LK_B(LK_F_SMALL), &LK_F_SMALL, "once", LK_LABEL);
        return;
    }
    while (*rest && *rest != ' ' && *rest != '[' && n < 23)
        kw[n++] = *rest++;
    kw[n] = 0;
    {
        int32_t end = 20 + text_w(&LK_F_MSG, kw) + lk_ktext_w(&LK_F_MSG, rest);
        if (ui.msg[1][0])
            end += 5 + lk_ktext_w(&LK_F_MSG, ui.msg[1]);
        cv_round(8, 3, end + LK_BANNER_PAD - 8 > 224 ? 224 : end + LK_BANNER_PAD - 8, bh, 3, LK_BAND);
    }
    cv_rect(11, 6, 3, bh - 6, c);
    x = cv_text(20, ty, &LK_F_MSG, kw, c);
    if (*rest)
        x = lk_ktext(x, ty, &LK_F_MSG, rest, LK_INK);
    if (ui.msg[1][0])
        lk_ktext(x + 5, ty, &LK_F_MSG, ui.msg[1], LK_INK);
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

/* text with its buttons and keys in boxes (the spec's rule 19, Evan 2026-11-02): "turn on [SEL] + [PLAY]" draws SEL
 * and PLAY as lk_key's boxes; returns where it ends (draw 0: only measures) */
static int32_t lk_fbase(const felucca_font_t *f)
{
    return f == &FONT_S ? FONT_S_BASE : f == &FONT_B ? FONT_B_BASE : f == &FONT_T18 ? FONT_T18_BASE
         : f == &FONT_Q17 ? FONT_Q17_BASE : f == &FONT_T20 ? FONT_T20_BASE : FONT_U1_BASE;
}
static int32_t lk_ktext_(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, int draw)
{
    char seg[24];
    int n;
    while (*s) {
        const int box = *s == '[';
        if (box)
            s++;
        for (n = 0; *s && n < 23 && (box ? *s != ']' : *s != '['); )
            seg[n++] = *s++;
        seg[n] = 0;
        if (box && *s == ']')
            s++;
        if (box) {
            const int32_t w = text_w(&LK_F_SEL, seg) + 6;
            if (draw)
                lk_key(x + 1, y + lk_fbase(f), seg);
            x += w + 2;
        } else {
            x = draw ? cv_text(x, y, f, seg, c) : x + text_w(f, seg);
        }
    }
    return x;
}
static int32_t lk_ktext(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    return lk_ktext_(x, y, f, s, c, 1);
}
static int32_t lk_ktext_w(const felucca_font_t *f, const char *s) { return lk_ktext_(0, 0, f, s, 0, 0); }

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
#define LK_BDX 0                             /* Osc and Drones 2: each strand straight down, 4 px (Evan, 2026-11-02:
                                              * Osc as Drones 2; it was 2 px right too) */
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
            if (j < ns - 1 && !dx && (y[i] - y[i - 1] > 3 * dy || y[i - 1] - y[i] > 3 * dy))
                continue;                      /* a jump (a saw's, a square's): the front strand's alone, or stacked
                                                * straight down they'd fill it in as a solid bar (Evan, 2026-11-02) */
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

/* how a drone's strands follow the level heard (dr_level, so a gesture's or a scene's level moves count), as Sub ·
 * Drive's lanes do (Evan, 2026-11-02): over -48..0 dB, dimming toward silence with most of the change near it,
 * closing up below about -34 dB; silent (under -48 dB): one thin gray line. Returns 0 when silent */
static int lk_heard(int g, float *bright, float *close)
{
    const float l = dr_level[g], db = l > 1e-4f ? 6.0206f * fm_log2f(l) : -99.0f;
    const float lf = fm_clampf((db + 48.0f) / 48.0f, 0.0f, 1.0f), d = 1.0f - lf;
    *bright = 0.35f + 0.65f * (1.0f - d * d * d);
    *close = fm_minf(1.0f, lf / 0.3f);
    return lf > 0.0f;
}

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
        {   /* scaled to its own loudness, but as quiet as it is heard near silence: smaller and dimmer, then a
             * thin gray line */
            float br, cl;
            if (!lk_heard(g, &br, &cl)) {
                cv_rect(x0, mid, DR_SCOPE_N, 1, LK_UNLIT);
                continue;
            }
            for (j = 0; j < LK_STRANDS; j++) { /* the oldest first, deep; the newest last, light */
                const uint16_t c = lk_mix(0x07060Bu, lk_ramp888(g, (float)j / (float)(LK_STRANDS - 1)), br);
                for (i = 1; i < DR_SCOPE_N; i++) {
                    const int32_t a = (int32_t)((float)lk_hist[g][j][i - 1] * cl), b = (int32_t)((float)lk_hist[g][j][i] * cl);
                    cv_line(x0 + i - 1, mid - a, x0 + i, mid - b, c);
                    cv_line(x0 + i - 1, mid + 1 - a, x0 + i, mid + 1 - b, c);
                }
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
 * Drift on it fans them), in place at any level, closing up and dimming toward silence (Evan, 2026-11-02); its
 * level in dB under it */
static float lk_cut_hist[DR_NGROUPS][LK_STRANDS];

static void lk_pic_drones2(void)
{
    const int32_t top = 2 + (int32_t)LK_F_DRONE.h + 4, bot = LK_PIC_H - (int32_t)LK_F_SMALL.h - 6, h = bot - top;
    int g, j, i;
    lk_halves();
    for (g = 0; g < DR_NGROUPS; g++) {         /* the filter's mode beside the drone's name, and its key (Evan,
                                                * 2026-11-02: the curve shows it, the key says where), as Sub · Drive */
        static const char *const MODE[3] = {"LP", "BP", "HP"};
        const int32_t base = 2 + LK_B(LK_F_DRONE), x1 = (g ? 130 : 10) + text_w(&LK_F_DRONE, g ? "II" : "I") + 6;
        const int m = gpar(g, GP_MODE);
        lk_key(cv_text(x1, base - LK_B(LK_F_SMALL), &LK_F_SMALL, MODE[(unsigned)m < 3u ? m : 0], LK_DRONE[g]) + 5, base,
               LK_BKEY[g ? 7 : 2]);
    }
    for (g = 0; g < DR_NGROUPS; g++) {
        const int32_t x0 = g ? 130 : 10;
        const float l = dr_level[g], db = l > 1e-4f ? 6.0206f * fm_log2f(l) : -99.0f;
        char t[12];
        for (j = 0; j < LK_STRANDS - 1; j++)
            lk_cut_hist[g][j] = lk_cut_hist[g][j + 1];
        lk_cut_hist[g][LK_STRANDS - 1] = dr_cut[g];
        {
            float br, cl;
            if (!lk_heard(g, &br, &cl)) {      /* silent: the curve as one thin gray line, where it was */
                int32_t py = 0, yy;
                for (i = 0; i <= 50; i++) {
                    const float d = filt_db(g, lk_cut_hist[g][LK_STRANDS - 1], 100.0f * (float)i / 50.0f);
                    yy = bot - LK_BDY - (int32_t)((fm_clampf(d, -36.0f, 18.0f) + 36.0f) / 54.0f * (float)(h - 5 * LK_BDY));
                    if (i)
                        cv_line(x0 + 2 * (i - 1), py, x0 + 2 * i, yy, LK_UNLIT);
                    py = yy;
                }
            } else
                for (j = 1; j < LK_STRANDS; j++) {   /* five (Evan, 2026-11-02: one fewer, the same ramp), closing
                                                      * up and dimming near silence: the oldest first, deep; the
                                                      * newest last, light */
                    int16_t y[51];
                    const int32_t oy = (int32_t)((float)(LK_BDY * j) * cl + (float)LK_BDY * (1.0f - cl) + 0.5f);
                    for (i = 0; i <= 50; i++) {
                        const float d = filt_db(g, lk_cut_hist[g][j], 100.0f * (float)i / 50.0f);
                        y[i] = (int16_t)(bot - oy - (int32_t)((fm_clampf(d, -36.0f, 18.0f) + 36.0f) / 54.0f *
                                                              (float)(h - 5 * LK_BDY)));   /* in place at any level
                                                                                             * (Evan, 2026-11-02: it
                                                                                             * sank as it got quiet) */
                    }
                    lk_plot(x0, y, 51, lk_mix(0x07060Bu, lk_ramp888(g, (float)(j - 1) / (float)(LK_STRANDS - 2)), br));
                }
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

/* Sub · Drive's strands (Evan, 2026-11-02): as many as the drive is fat, 4 at Drive 0 to 6 from 60 (Evan: from 3 the drive outweighed the crossfade), gliding (the
 * next one fades in, the colors slide along the ramp, so it always spans the whole ramp); 3 px apart, closing up
 * into one line as the lane's share (v0) nears 0, so nothing jumps on the way to the thin "off" line */
static void lk_drive_bundle(int32_t x0, const int16_t *y, int n, int g, float drive, float v0, float bright)
{
    const float nf = 4.0f + 2.0f * fm_clampf(drive / 60.0f, 0.0f, 1.0f), sp = 3.0f * fm_minf(1.0f, v0 / 0.15f);
    const int nn = (int)(nf + 0.999f);
    int j, i;
    for (j = nn - 1; j >= 0; j--) {            /* the deepest, furthest down, first; the light one in front */
        const float t = 1.0f - (float)j / (nf - 1.0f), a = j + 1 <= (int)nf ? 1.0f : nf - (float)(int)nf;
        const int32_t oy = (int32_t)((float)j * sp - (nf - 1.0f) * sp * 0.5f + 0.5f);
        const uint16_t c = lk_mix(0x07060Bu, lk_ramp888(g, fm_clampf(t, 0.0f, 1.0f)), bright * a);
        for (i = 1; i < n; i++) {
            cv_line(x0 + 2 * (i - 1), y[i - 1] + oy, x0 + 2 * i, y[i] + oy, c);
            cv_line(x0 + 2 * (i - 1), y[i - 1] + oy + 1, x0 + 2 * i, y[i] + oy + 1, c);
        }
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
            {   /* the wave after its lane's name, dimming toward its share's 0, most of the change near it; at 0 one
                 * thin dim line, plainly off (Evan, 2026-11-02) */
                const float v0 = k ? mix : 1.0f - mix, d = 1.0f - v0;
                int16_t y[37];
                for (i = 0; i <= 36; i++) {
                    float ph = 2.0f * (float)i / 36.0f, v;
                    ph -= (float)(int)ph;
                    v = share[k] * (k ? lk_sub_at(sw, ph) : xwave_at(x, pw, ph));
                    y[i] = (int16_t)(yc - (int32_t)(fm_tanhf(drv * v) / td * (float)amp));
                }
                if (v0 < 0.005f)
                    cv_rect(x0 + 28, yc, 72, 1, LK_UNLIT);
                else
                    lk_drive_bundle(x0 + 28, y, 37, g, (float)gpar(g, GP_DRIVE), v0,
                                    0.35f + 0.65f * (1.0f - d * d * d));
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
static void lk_swing(char *q, float x, const char *unit);
static const uint32_t LK_LFO_TINT[2] = {0xA98BFFu, 0xFF5FD2u};   /* LFO I lilac (OCT-), LFO II pink (OCT+) */

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
/* the LFO's phase at n even steps of time across one cycle, as Speed bends the clock (a bit of the cycle takes
 * 2^(-rm * Source)): always forward, from its start to its end (stepping by speed overran it and wrapped, and the dot
 * stuck at the end at negative Speed; Evan, 2026-11-03). Mods and LFO sends draw the waves on it, so both show the
 * LFO as it runs (Evan, 2026-11-03); 0 at Speed 0, the plain cycle */
static const float *lk_lfo_phs(float *phs, int n)
{
    const int w1 = proj.par[P_LFO_W1], w2 = proj.par[P_LFO_W2], src = proj.par[P_LFO_RMOD_SRC];
    const float rm = (float)proj.par[P_LFO_RMOD] * 0.03f;
    float tm[64];
    int i, j;
    if (rm == 0.0f)
        return 0;
    tm[0] = 0.0f;
    for (i = 0; i < n - 1; i++) {
        const float ph = ((float)i + 0.5f) / (float)(n - 1);
        const float v0 = dr_lfo_wave(0, w1, ph), v1 = dr_lfo_wave(1, w2, ph);
        const float s = src == DR_LFO_BOTH ? fm_clampf(v0 + v1, -1.0f, 1.0f) : src == DR_LFO_II ? v1 : v0;
        tm[i + 1] = tm[i] + fm_exp2f(-rm * s);
    }
    for (i = 0, j = 0; i < n; i++) {
        const float tt = tm[n - 1] * (float)i / (float)(n - 1);
        while (j < n - 2 && tm[j + 1] < tt)
            j++;
        phs[i] = fm_minf(((float)j + (tt - tm[j]) / fm_maxf(tm[j + 1] - tm[j], 1e-6f)) / (float)(n - 1), 0.9999f);
    }
    return phs;
}

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
/* the wiring as sentences (Evan's board, 2026-11-04, "routing try 2"): each drone its two sends, each a route from its
 * key's LFO to its key's target: "[F#3] LFO II -> [G#3] x-wave", "[A#3] LFO I+II -> [C#4] reso", the keys where they
 * sit (SW_LFO), the LFO in its tint (I+II white), a route at depth 0 (or fed by an Off wave, or Movement 0) all gray.
 * Above them the two LFOs, short-named, each a little cycle with its dot (bent by Speed, as on Mods), the rate at the
 * right; under that the Swell lock's state, right-justified (the waves on top, taller: Evan, 2026-11-07). Words at 18
 * as every page, the arrow 12 px to fit */
static const char *const LK_LFO_SHORT[2][DR_LFO_NSHAPES] = {
    {"Off", "Tri", "Sqr", "Saw", "Round", "2x Tri", "2x Exp", "4x Pat", "Rnd", "AND 3", "OR 3"},
    {"Off", "Tri 90", "Sqr 90", "Exp", "Log tri", "2x Saw", "2x Pat", "4x Exp", "Rnd", "AND 5", "OR 5"}};
static void lk_pic_lfo(void)
{
    static const int32_t RY[4] = {40, 61, 85, 106};   /* the routes' tops: drone I's two, a gap, drone II's */
    const int clock = dr_ext_spb > 0.0f, lock = proj.par[P_LFO_LOCK] != 0;
    const int32_t lb = LK_B(LK_F_SMALL), by = 0;    /* the LFOs' line, at the top */
    int w, d;
    char t[24], *q;
    float phs[28];
    const float *ph = lk_lfo_phs(phs, 28);      /* the waves bent as Speed bends the clock, as on Mods */
    for (d = 0; d < 4; d++) {
        const int g = d / 2, osc = d % 2 == 0;
        const int route = gpar(g, osc ? GP_OLFO : GP_FLFO), depth = gpar(g, osc ? GP_OLFO_DEPTH : GP_FLFO_DEPTH);
        const int heard = depth && proj.par[P_MOVE] && (lk_wave_on(route, 0) || lk_wave_on(route, 1));
        const int32_t ny = RY[d], base = ny + lb;
        const uint16_t lc = !heard ? LK_LABEL : route == DR_LFO_BOTH ? LK_INK
                          : lk_mix(0x07060Bu, LK_LFO_TINT[route == DR_LFO_II], 1.0f);
        const int32_t ay = ny + (int32_t)LK_F_SMALL.h / 2;   /* columns, as the board: the LFO, the arrow, the target */
        const int32_t xl = lk_key(38, base, LK_BKEY[g * 5 + (osc ? 0 : 2)]) + 3;
        const int32_t xa = xl + text_w(&LK_F_SMALL, "LFO I+II") + 3;   /* the widest LFO sets the arrow's column */
        cv_text(xl, ny, &LK_F_SMALL, route == DR_LFO_BOTH ? "LFO I+II" : route == DR_LFO_II ? "LFO II" : "LFO I", lc);
        cv_rect(xa, ay, 9, 1, lc);             /* a thin arrow, its head 3 px */
        cv_line(xa + 6, ay - 3, xa + 9, ay, lc);
        cv_line(xa + 6, ay + 3, xa + 9, ay, lc);
        const int32_t x = lk_key(xa + 12, base, LK_BKEY[g * 5 + (osc ? 1 : 3)]) + 3;
        cv_text(x, ny, &LK_F_SMALL, osc ? (gpar(g, GP_OLFO_DEST) == DR_TO_XWAVE ? "x-wave" : "pitch")
                                        : (gpar(g, GP_FLFO_DEST) == DR_TO_RESO ? "reso" : "cutoff"),
                heard ? LK_INK : LK_LABEL);
        if (!osc) {                            /* the drone's name and its bus, beside its two routes */
            const int32_t y0 = RY[d - 1] + 3, y1 = ny + (int32_t)LK_F_SMALL.h - 3;
            cv_text(8, (y0 + y1) / 2 - (int32_t)LK_F_SMALL.h / 2, &LK_F_SMALL, g ? "d II" : "d I", LK_DRONE[g]);
            cv_rect(33, y0, 1, y1 - y0, LK_DRONE[g]);
        }
    }
    {   /* the LFOs, I over II: "I  Tri", its cycle, the rate at the right; "II  Rnd", its cycle, the Swell lock's state
         * (Evan, 2026-11-03: "hold what?"). Two rows: on one, the longest names and "cycle 32 bars" ran into each other.
         * The waves in one column (after the widest name, "2x Saw"), 13 px tall and 54 long, to x 127, short of the
         * widest rate ("cycle 32 bars") and lock ("[D#4] on Swell"); Evan, 2026-11-07: on top, taller, longer */
        const int32_t xw = 8 + text_w(&LK_F_SMALL, "II") + 3 + text_w(&LK_F_SMALL, "2x Saw") + 4;
        for (w = 0; w < 2; w++) {
            const int sh = proj.par[w ? P_LFO_W2 : P_LFO_W1];
            const int32_t y = by + 17 * w;
            cv_text(8, y, &LK_F_SMALL, w ? "II" : "I", lk_mix(0x07060Bu, LK_LFO_TINT[w], 1.0f));
            cv_text(8 + text_w(&LK_F_SMALL, "II") + 3, y, &LK_F_SMALL,
                    LK_LFO_SHORT[w][(unsigned)sh < DR_LFO_NSHAPES ? sh : 0], sh ? LK_INK : LK_LABEL);
            lk_lfo_line(w, xw, y + (int32_t)LK_F_SMALL.h / 2, 6, 28, ph, 1);
        }
        if (!proj.par[P_MOVE]) {               /* why every route is gray: ALGORITHM's Movement at 0; in the rate's
                                                * place, which isn't heard while nothing moves (Evan, 2026-11-04) */
            cv_text(232 - text_w(&LK_F_SMALL, "mvmt 0%"), by, &LK_F_SMALL, "mvmt 0%", LK_VIOLET);   /* short, so the
                                                * waves can be long (Evan, 2026-11-07) */
        } else {
            q = scopy(t, clock ? "cycle " : "rate ");
            dr_param_text(clock ? P_LFO_DIV : P_LFO_RATE, proj.par[clock ? P_LFO_DIV : P_LFO_RATE], q);
            cv_text(232 - text_w(&LK_F_SMALL, t), by, &LK_F_SMALL, t, LK_INK);
        }
    }
    {
        /* locked with the Swell off, it waits for it: "on Swell" dimmed ("waits Swell" ran into the waves) */
        const char *lt = lock ? "[D#4] on Swell" : "[D#4] free";
        lk_ktext(232 - lk_ktext_w(&LK_F_SMALL, lt), by + 17, &LK_F_SMALL, lt,
                 !lock ? LK_LABEL : proj.par[P_PULSE] ? LK_CORAL : lk_mix(0x07060Bu, 0xFF8F73u, 0.55f));
    }
}

/* ------------------------------------------------------- Mods' picture --- */
/* two bands (the spec, 2026-10-31). The LFO band: the clock (SELECT's time in a pill) branching to LFO I and II, each
 * a cycle warped as the engine warps the clock (a bit of it takes 2^(-rm · source)), one dot on both; on the
 * right Speed mod's row ("LFO speed", linked from its Source, a loop arrow back into the clock) and Delay mod's
 * ("delay time", from key 10's LFO). The cross-mod band: what it does and its key ("CROSS-MOD  I moves II's cutoff
 * [F#5]"; Evan, 2026-11-02), then drone I, the amount, drone II, gray when off */
static void lk_pic_mods(void)
{
    const int w1 = proj.par[P_LFO_W1], w2 = proj.par[P_LFO_W2], src = proj.par[P_LFO_RMOD_SRC], rmd = proj.par[P_LFO_RMOD];
    const int xm = proj.par[P_XMOD], amt = proj.par[P_XMOD_AMT];
    float phs[37];
    int32_t wy[2], x;
    int i, w;
    char t[24];
    const float *ph = lk_lfo_phs(phs, 37);
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
        const int32_t xe = cv_text(xn, ny, &LK_F_SMALL, w ? "II" : "I", lk_mix(0x07060Bu, LK_LFO_TINT[w], 1.0f));
        dr_param_text(w ? P_LFO_W2 : P_LFO_W1, sh, t);
        cv_text(xe + 5, ny, &LK_F_SMALL, t, sh ? LK_INK : LK_LABEL);    /* as LFO sends: the LFO in its tint, its
                                                                         * shape white (Evan, 2026-11-03) */
        wy[w] = ny + (int32_t)LK_F_SMALL.h + 2;
        cv_line(x, 29, xn - 2, wy[w], LK_LABEL);      /* the clock branching to both */
        lk_lfo_line(w, xn, wy[w], 4, 37, ph, 1);
    }
    {   /* the rows: Speed mod from its Source, Delay mod from key 10's LFO */
        const int dl = proj.par[P_DLY_LFO], dd = proj.par[P_DLY_LFO_DEPTH];
        const int sp_on = rmd && proj.par[P_MOVE] && (lk_wave_on(src, 0) || lk_wave_on(src, 1));
        const int dl_on = dd && proj.par[P_MOVE] && (lk_wave_on(dl, 0) || lk_wave_on(dl, 1));
        const int32_t my = (wy[0] + wy[1]) / 2;   /* "delay time" between the waves, the band's one way out */
        int32_t top = 61;
        cv_text(158, my - (int32_t)LK_F_SMALL.h / 2, &LK_F_SMALL, "delay time", dl_on ? LK_INK : LK_LABEL);
        for (w = 0; w < 2; w++) {
            if (dl == w + 1 || dl == DR_LFO_BOTH)
                cv_line(x + 86, wy[w], 155, my, dl_on && lk_wave_on(dl, w) ? lk_mix(0x07060Bu, LK_LFO_TINT[w], 0.85f)
                                                                           : LK_UNLIT);
            if (src == w + 1 || src == DR_LFO_BOTH) {   /* Speed: the Source wave straight back into the clock */
                cv_rect(x + 86, wy[w], 233 - x - 86, 1, sp_on && lk_wave_on(src, w) ? lk_mix(0x07060Bu, LK_LFO_TINT[w], 0.85f)
                                                                                   : LK_UNLIT);
                if (wy[w] < top)
                    top = wy[w];
            }
        }
        {   /* round the right edge and under the waves, up into the clock */
            const uint16_t c = sp_on ? LK_LABEL : LK_UNLIT;
            cv_rect(232, top, 1, 62 - top, c);
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
        /* the one line in smaller type (at 18 it doesn't fit; Evan, 2026-11-03); the drones' names at 18 */
        x = cv_text(8, hb - LK_B(FONT_U1), &FONT_U1, "CROSS-MOD", xm ? LK_INK : LK_LABEL);
        x = cv_text(x + 4, hb - LK_B(LK_F_TAG), &LK_F_TAG, DOES[(unsigned)xm < 4u ? xm : 0], LK_LABEL);
        lk_key(x + 4, hb, LK_BKEY[10]);
        cv_text(8, 89, &LK_F_SMALL, "drone I", ci);
        {
            const char *n2 = xm == DR_XMOD_CUT ? "drone II cutoff" : "drone II";
            cv_text(232 - text_w(&LK_F_SMALL, n2), 89, &LK_F_SMALL, n2, cii);   /* over its sound, at the right */
        }
        for (g = 0; g < 2; g++) {              /* each drone's sound now, small, scaled to its own peak */
            float m = 0.05f;
            for (i = 0; i < DR_SCOPE_N; i++)
                m = fm_maxf(m, fm_fabsf(dr_scope[g][i]));
            for (i = 0; i < 34; i++)
                y[i] = (int16_t)(116 - (int32_t)(dr_scope[g][i * DR_SCOPE_N / 34] / m * 5.0f));
            lk_plot(g ? 166 : 8, y, g ? 34 : 30, g ? cii : ci);
        }
        if (xm >= DR_XMOD_FM) {                /* drone I to drone II, both ways when both are bent; no amount bar,
                                                * X-mod's value says it (Evan, 2026-11-02) */
            lk_harrow(70, 110, 88, 1, ci);
            lk_harrow(70, 119, 88, 0, cii);
        } else if (xm) {                       /* off: no arrow (Evan, 2026-11-03) */
            lk_harrow(70, 115, 88, 1, ci);
        }
    }
}

/* ---------------------------------------------------- Setup 1's picture --- */
/* a row per time (the board, GLO 130:1852; the spec, 2026-11-01): its title, the knob's word bold ("Time to fade in /
 * out on PLAY"), and a symbol across the row ending where its knob is: the knob's whole range is the row (Evan,
 * 2026-11-07: turned all the way, it reaches the end), so the change takes the row's first part and the rest is its
 * new setting. Fade one thick line up from silence (the board's); Morph and Scene a crossfade, a out as b comes in (he
 * liked Morph's better than Scene's straight lines). The knob being turned marks its time with a dot where the change
 * ends; on a MIDI clock Scene's knob is its bars */
static float lk_ease(float u) { u = fm_clampf(u, 0.0f, 1.0f); return u * u * (3.0f - 2.0f * u); }
static void lk_pic_setup1(void)
{
    static const char *const PRE[3] = {"Time to ", "Time to ", "Time to switch "},
                             *const BOLD[3] = {"fade", "morph", "scenes"},
                             *const POST[3] = {" in / out on PLAY", " keys and LFOs", ""};
    const float frac[3] = {(float)proj.par[P_FADE] * 0.01f, (float)proj.par[P_MORPH] * 0.01f,
                           dr_ext_spb > 0.0f ? (float)proj.par[P_SCENE_BARS] / (float)(DR_NBARS - 1)
                                             : (float)proj.par[P_SCENE_T] * 0.01f};
    int r, i, s;
    for (r = 0; r < 3; r++) {
        const int32_t base = 16 + 40 * r, top = base + 7, bot = base + 21;
        const int32_t xe = 20 + (int32_t)(fm_clampf(frac[r], 0.0f, 1.0f) * 200.0f + 0.5f);
        int16_t y[101];
        int32_t x = 12;                        /* words at 18, the knob's in bold (19: the one bold near it); the
                                                * longest, morph's, 215 px */
        x = cv_text(x, base - LK_B(LK_F_SMALL), &LK_F_SMALL, PRE[r], LK_LABEL);
        x = cv_text(x, base - LK_B(FONT_B), &FONT_B, BOLD[r], LK_INK);
        cv_text(x, base - LK_B(LK_F_SMALL), &LK_F_SMALL, POST[r], LK_LABEL);
        for (s = r ? 0 : 1; s < 2; s++) {     /* s 0: a going out; 1: b coming in (Fade's rise) */
            for (i = 0; i <= 100; i++) {
                const int32_t xi = 20 + 2 * i;
                const float u = xi >= xe ? 1.0f : lk_ease((float)(xi - 20) / (float)(xe > 20 ? xe - 20 : 1));
                y[i] = (int16_t)(s ? bot - (int32_t)((float)(bot - top) * u + 0.5f) : top + (int32_t)((float)(bot - top) * u + 0.5f));
            }
            lk_plot(20, y, 101, s ? LK_WHITE : LK_UNLIT);
            if (!r) {                          /* Fade: one thick line, 3 px */
                for (i = 0; i <= 100; i++)
                    y[i]++;
                lk_plot(20, y, 101, LK_WHITE);
            }
        }
        if (r) {
            cv_text(10, top - LK_B(LK_F_SMALL) + 5, &LK_F_SMALL, "a", LK_LABEL);   /* beside where each starts */
            cv_text(10, bot - LK_B(LK_F_SMALL) + 5, &LK_F_SMALL, "b", LK_WHITE);
        }
        if (lit(r))                            /* the knob's time: where the change ends */
            cv_round(xe - 3, top - 3 + (r ? 0 : 1), 7, 7, 3, LK_WHITE);
    }
}

/* ---------------------------------------------------- Setup 2's picture --- */
/* the board (GLO 130:1913; every Tuning's ring 301:17): Tuning and Tune as one ring over KNOB 1-2, an octave clockwise,
 * a tick a note (dr_tun_c), Tune turning it away from the fixed notch at the top (440), A named on its tick; the
 * Tuning's 1/1 named on its (LMY on C, Harrison and Partch on G, Harmonics on A; Just and Shruti on drone I, the ring
 * turned to put it at the top); inside, its notes and its 1/1. The drones on it as dots where they sound, each one's
 * nearest note lit in its color, beads from drone I up to drone II; under it the interval, a ratio on the pure ones
 * when it's simple (both up to 32), else its name. Stereo: the two drones' waves upright, L and R as far apart as it
 * puts them. Pressure: what a press gives, light .. firm, and the key pressed now on it */
#define LK_RING RGB(119, 114, 138)
static void lk_circle(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* 1 px, the midpoint way */
{
    int32_t x = r, y = 0, e = 1 - r;
    while (x >= y) {
        cv_rect(cx + x, cy + y, 1, 1, c); cv_rect(cx - x, cy + y, 1, 1, c);
        cv_rect(cx + x, cy - y, 1, 1, c); cv_rect(cx - x, cy - y, 1, 1, c);
        cv_rect(cx + y, cy + x, 1, 1, c); cv_rect(cx - y, cy + x, 1, 1, c);
        cv_rect(cx + y, cy - x, 1, 1, c); cv_rect(cx - y, cy - x, 1, 1, c);
        y++;
        if (e < 0) {
            e += 2 * y + 1;
        } else {
            x--;
            e += 2 * (y - x) + 1;
        }
    }
}
static float lk_wrap12(float c) { return c - 1200.0f * fm_floorf(c * (1.0f / 1200.0f)); }
static void lk_polar(int32_t cx, int32_t cy, float c, float r, int32_t *x, int32_t *y)   /* c cents clockwise from the
                                                                                         * top */
{
    const float a = c * (6.2831853f / 1200.0f);
    *x = cx + (int32_t)(r * fm_sinf(a) + (fm_sinf(a) < 0.0f ? -0.5f : 0.5f));
    *y = cy - (int32_t)(r * fm_cosf(a) + (fm_cosf(a) < 0.0f ? -0.5f : 0.5f));
}
static void lk_spoke(int32_t cx, int32_t cy, float c, float r0, float r1, int w, uint16_t col)
{
    int32_t x0, y0, x1, y1;
    lk_polar(cx, cy, c, r0, &x0, &y0);
    lk_polar(cx, cy, c, r1, &x1, &y1);
    cv_line(x0, y0, x1, y1, col);
    if (w > 1) {                               /* 2 px: beside it, across the spoke */
        const int up = fm_fabsf(fm_sinf(c * (6.2831853f / 1200.0f))) < 0.7f;
        cv_line(x0 + !up, y0 + up, x1 + !up, y1 + up, col);
    }
}
static int lk_simple(int c, int *p, int *q)    /* the simplest p/q (both up to 32) within 4 cents of c > 0 */
{
    int a, b, best = 99;
    for (b = 1; b <= 32; b++)
        for (a = b + 1; a <= 32; a++) {
            const float rc = 1200.0f * fm_log2f((float)a / (float)b);
            if (fm_fabsf(rc - (float)c) <= 4.0f && a + b < best) {
                best = a + b;
                *p = a;
                *q = b;
            }
        }
    return best < 99;
}
static const char *const LK_NOTE12[12] = {"A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#"};
static void lk_pic_setup2(void)
{
    const int t = (unsigned)proj.par[P_TUNING] < DR_NTUNING ? proj.par[P_TUNING] : DR_TUN_12;
    const int n = dr_tun_n(t), moves = DR_TUN_MOVES(t), pure = t >= DR_TUN_JUST;
    const float tc = 1200.0f * fm_log2f(1.0f + (float)proj.par[P_TUNE] * (1.0f / 4400.0f));
    const int32_t cx = 52, cy = 52, R = 34;     /* the ring, its ticks out to R + 9, A at R + 12: clear of the
                                                * header and the interval */
    float pc[2];
    int g, i, k;
    char tx[24];
    for (g = 0; g < 2; g++) {                  /* the pitches as set, Shift's move in them */
        const int set = g && proj.par[P_PLINK] ? proj.par[DR_GP(0, GP_PITCH)] + proj.par[P_PLINK_OFF]
                                               : proj.par[DR_GP(g, GP_PITCH)];
        pc[g] = (float)set + dr_gest_pitch(g) * 100.0f;
    }
    {   /* the ring, its notes, A, the 1/1, the drones */
        const float rot = moves ? -pc[0] + 6900.0f : tc;   /* cents from A to the ring's angle */
        int near[2];
        lk_circle(cx, cy, R, LK_RING);
        if (!moves) {                          /* the fixed notch: A at 440 */
            cv_rect(cx - 2, cy - R - 13, 5, 1, LK_LABEL);   /* a small triangle, point down, over the top */
            cv_rect(cx - 1, cy - R - 12, 3, 1, LK_LABEL);
            cv_rect(cx, cy - R - 11, 1, 1, LK_LABEL);
        }
        for (g = 0; g < 2; g++) {              /* each drone's nearest note */
            const float d = lk_wrap12(pc[g] - 6900.0f + (moves ? 6900.0f - pc[0] : 0.0f));
            float bd = 1e9f;
            near[g] = 0;
            for (k = 0; k < n; k++) {
                const float e = fm_fabsf(lk_wrap12(d - (float)dr_tun_c(t, k) + 600.0f) - 600.0f);
                if (e < bd) {
                    bd = e;
                    near[g] = k;
                }
            }
        }
        for (k = 0; k < n; k++) {
            const float c = (float)dr_tun_c(t, k) + (moves ? 0.0f : tc);
            const int lit0 = near[0] == k, lit1 = near[1] == k;
            if (lit0 || lit1)
                lk_spoke(cx, cy, c, (float)R + 2.0f, (float)R + 9.0f, 2, LK_DRONE[lit1 && !lit0 ? 1 : 0]);
            else
                lk_spoke(cx, cy, c, (float)R + 3.0f, (float)R + (n > 24 ? 6.0f : 8.0f), 1, LK_NEUTRAL);
        }
        if (!moves) {                          /* A on its tick */
            int32_t ax, ay;
            lk_polar(cx, cy, tc, (float)R + 10.0f, &ax, &ay);   /* beside its tick's tip, right of the notch, a
                                                * little off the ring (Evan, 2026-11-07) */
            cv_text(ax + 6, ay - LK_B(LK_F_SMALL) / 2 - 3, &LK_F_SMALL, "A", LK_WHITE);
        }
        {   /* the 1/1 on its tick (none on the equal ones) */
            const int one = t == DR_TUN_LMY ? 294 : t == DR_TUN_HARRISON || t == DR_TUN_PARTCH ? 996 : 0;
            if (pure && t != DR_TUN_HARM && !moves) {
                int32_t ox, oy;
                lk_polar(cx, cy, (float)one + tc, (float)R + 22.0f, &ox, &oy);   /* off the ring (Evan) */
                ox = ox < 12 ? 12 : ox > 110 ? 110 : ox;   /* on the screen (Harrison's, Partch's at the left) */
                text_c(ox, oy - LK_B(LK_F_TAG) / 2 - 2, &LK_F_TAG, "1/1", LK_NEUTRAL);
            }
            /* inside: its notes, its 1/1 */
            itoa_u((uint32_t)n, tx);
            scopy(tx + (n >= 10 ? 2 : 1), " notes");
            text_c(cx, pure ? cy - 16 : cy - 9, &LK_F_SMALL, tx, LK_WHITE);
            if (pure) {
                {
                    char *q = scopy(tx, "1/1 on ");
                    scopy(q, moves ? "I" : t == DR_TUN_HARM ? "A" : LK_NOTE12[(one + 50) / 100 % 12]);
                }
                text_c(cx, cy + 1, &LK_F_TAG, tx, LK_LABEL);
            }
        }
        {   /* beads from drone I up to drone II, inside the ring */
            const float a0 = lk_wrap12(pc[0] - 6900.0f + rot), span = lk_wrap12(pc[1] - pc[0]);
            const int nb = (int)(span * (24.0f / 1200.0f));
            for (i = 1; i < nb; i++) {
                int32_t bx, by;
                const float u = (float)i / (float)nb;
                lk_polar(cx, cy, a0 + span * u, (float)R - 5.0f, &bx, &by);
                cv_round(bx - 1, by - 1, 3, 3, 1, lk_mix(0x07060Bu, u < 0.5f ? 0xFF9A1Fu : 0x1FC8FFu, 0.45f + 0.25f * (u < 0.5f ? u : 1.0f - u) * 2.0f));
            }
            for (g = 1; g >= 0; g--) {         /* the drones where they sound */
                int32_t dx, dy;
                lk_polar(cx, cy, lk_wrap12(pc[g] - 6900.0f + rot), (float)R, &dx, &dy);
                cv_round(dx - 3, dy - 3, 7, 7, 3, LK_DRONE[g]);
            }
        }
    }
    {   /* the interval, drone II from drone I: a simple ratio on the pure ones, else its name */
        const int d = (int)(pc[1] - pc[0] + (pc[1] >= pc[0] ? 0.5f : -0.5f)), a = d < 0 ? -d : d;
        int p = 1, q = 1;
        int32_t w, x;
        if (pure && a > 4 && lk_simple(a, &p, &q)) {
            char *z = tx;
            itoa_u((uint32_t)p, z);
            while (*z)
                z++;
            *z++ = '/';
            itoa_u((uint32_t)q, z);
        } else {
            lk_interval(tx, d);
        }
        w = text_w(&LK_F_DRONE, tx) + (a > 4 ? 14 : 0);
        x = cv_text(cx - w / 2, LK_PIC_H - 2 - (int32_t)LK_F_DRONE.h, &LK_F_DRONE, tx, LK_DRONE[1]);
        if (a > 4)
            lk_arrow(x + 4, LK_PIC_H - 2 - (int32_t)LK_F_DRONE.h + LK_B(LK_F_DRONE) - 1, 13, d > 0, LK_DRONE[1]);
    }
    {   /* STEREO: each drone's wave upright, as far apart as it puts them */
        const float wid = (float)proj.par[P_WIDTH] * 0.01f;
        for (g = 0; g < 2; g++) {
            const int32_t xc = 138 + (g ? 1 : -1) * (int32_t)(2.0f + 15.0f * wid + 0.5f);
            const float ph = (float)(plat_ms() % 4000u) * (1.0f / 4000.0f) + 0.5f * (float)g;
            int16_t xs[38];
            int s;
            for (i = 0; i < 38; i++)           /* a smooth wave down the picture, slowly running (the board's) */
                xs[i] = (int16_t)(xc + (int32_t)(5.0f * fm_sinf(6.2831853f * ((float)i / 18.5f + ph)) + 0.5f));
            for (s = 0; s < 3; s++) {          /* three strands, the light one in front */
                const uint16_t c = lk_ramp(g, (float)s / 2.0f);
                const int32_t o = 3 * (2 - s);
                for (i = 1; i < 38; i++) {
                    cv_line(xs[i - 1] + o, 22 + 2 * (i - 1), xs[i] + o, 22 + 2 * i, c);
                    cv_line(xs[i - 1] + o + 1, 22 + 2 * (i - 1), xs[i] + o + 1, 22 + 2 * i, c);
                }
            }
            text_c(138 + (g ? 1 : -1) * (int32_t)(8.0f + 9.0f * wid + 0.5f) + 3, 98, &LK_F_TAG, g ? "R" : "L",
                   LK_LABEL);                 /* under them, apart even at mono */
        }
    }
    {   /* PRESSURE: light .. firm, the key pressed now on it */
        int16_t y[28];
        int best = -1, raw = 0, s;
        for (i = 0; i < NSCENES; i++) {
            const int pr = plat_pressure(WHITE_K[i]);
            if (pr > raw + 2) {
                raw = pr;
                best = i;
            }
        }
        for (i = 0; i < 28; i++)
            y[i] = (int16_t)(96 - (int32_t)(press_curve((float)i / 27.0f) * 72.0f + 0.5f));
        for (s = 0; s < 3; s++) {              /* three strands 3 px apart down, the light one in front at 2 px, the
                                                * two behind 1 px (on a slope 2 px ones blend into a band) */
            const uint16_t c = s == 2 ? LK_WHITE : lk_ramp(LK_G, 0.3f + 0.35f * (float)s);
            const int32_t o = 3 * (2 - s);
            if (s == 2) {
                lk_plot(176, y, 28, c);
                continue;
            }
            for (i = 1; i < 28; i++)
                cv_line(176 + 2 * (i - 1), y[i - 1] + o, 176 + 2 * i, y[i] + o, c);
        }
        if (best >= 0) {                       /* "[A3] 62%": the key by its note, what its press gives */
            static const char *const W[7] = {"F", "G", "A", "B", "C", "D", "E"};
            const float x = (float)raw * (1.0f / 127.0f), v = press_curve(x);
            char *z = scopy(tx, "[");
            z = scopy(z, W[best % 7]);
            itoa_u((uint32_t)(3 + (best + 4) / 7), z);
            while (*z)
                z++;
            z = scopy(z, "] ");
            itoa_u((uint32_t)(v * 100.0f + 0.5f), z);
            while (*z)
                z++;
            scopy(z, "%");
            lk_ktext(232 - lk_ktext_w(&LK_F_SMALL, tx), 0, &LK_F_SMALL, tx, LK_WHITE);
            cv_round(176 + (int32_t)(x * 54.0f) - 3, 96 - (int32_t)(v * 72.0f) - 3, 7, 7, 3, LK_WHITE);
        }
        cv_text(168, 98, &LK_F_TAG, "light", LK_LABEL);
        cv_text(232 - text_w(&LK_F_TAG, "firm"), 98, &LK_F_TAG, "firm", LK_LABEL);
    }
}

/* ------------------------------------------------------ the Swell's picture --- */
/* two bars of the Swell, four breaths, as the engine shapes them (dr_pulse_wave: the first longer with Swing), the level
 * down the picture in dB (the top full, the bottom -30 dB or silence: the dip is the Depth). Five 2 px strands in the
 * Swell's coral ramp: the breath just before and after the dot, meeting at the dot and fanning out with distance from
 * it, the fan 0.045 / (1 + 2.2 Hz) of a bar (wide when slow, nearly one line when fast, so it doesn't blur); a white dot
 * where the Swell is now. Off: one gray line, still, no dot (the spec, 2026-10-31; the board, ARP 262:4) */
#define LK_SW_N 109                            /* 2 px a point: x 12 .. 228, two bars (54 px a breath) */
#define LK_SW_S 5                              /* strands, 2 px as on every page (Evan, 2026-11-07: they were 1 px,
                                                * thinner and rougher than the rest; five, as Drones 2) */
static int16_t lk_swell_y(float ph, int32_t top, int32_t h)
{
    const float g = fm_maxf(dr_pulse_wave(ph), 1e-4f), db = fm_log2f(g) * 6.0206f;
    return (int16_t)(top + (int32_t)(fm_clampf(-db * (1.0f / 30.0f), 0.0f, 1.0f) * (float)h + 0.5f));
}
static void lk_pic_swell(void)
{
    const int on = proj.par[P_PULSE] != 0;
    static float last;                         /* two bars across (Evan, 2026-11-07): which one the dot is in, the
                                                * screen's own count, flipped as the phase starts over */
    static int bar;
    const float ph = dr_pulse_ph, fan = 0.09f / (1.0f + 2.2f * dr_pulse_hz(-1));   /* in phase: a bar is 2 */
    float dot;
    const int32_t x0 = 12, top = 7, h = 100;
    int16_t y[LK_SW_N];
    int s, i;
    if (ph < last - 1.0f)
        bar ^= 1;
    last = ph;
    dot = ph + 2.0f * (float)bar;
    for (s = on ? 0 : LK_SW_S - 1; s < LK_SW_S; s++) {   /* the coral one at the back, the cream one in front */
        const float o = ((float)s - 0.5f * (LK_SW_S - 1)) * (2.0f / (LK_SW_S - 1));
        const uint16_t c = on ? lk_mix(0xFF8F73u, 0xFFF0E6u, (float)s / (float)(LK_SW_S - 1)) : LK_UNLIT;
        for (i = 0; i < LK_SW_N; i++) {
            const float p = 4.0f * (float)i / (float)(LK_SW_N - 1);
            y[i] = lk_swell_y(on ? p + o * fan * fm_fabsf(p - dot) * 0.5f : p, top, h);
        }
        lk_plot(x0, y, LK_SW_N, c);
    }
    if (on) {
        const int32_t px = x0 + (int32_t)(dot * 0.5f * (float)(LK_SW_N - 1) + 0.5f), py = lk_swell_y(dot, top, h);
        cv_round(px - 3, py - 3, 7, 7, 3, LK_INK);
    }
}

static uint32_t lk_mix888(uint32_t a, uint32_t b, float t)   /* 0xRRGGBB a -> b, as 0xRRGGBB */
{
    uint32_t c = 0;
    int sh;
    for (sh = 16; sh >= 0; sh -= 8)
        c |= (uint32_t)((float)(a >> sh & 255u) + ((float)(b >> sh & 255u) - (float)(a >> sh & 255u)) * t) << sh;
    return c;
}

/* an ellipse's ring, 1 px a line, w lines thick (out from rx, ry): all of it, or its upper (half 1) or lower (2) half */
static void lk_ring(int32_t cx, int32_t cy, float rx, float ry, int half, int w, uint16_t c)
{
    static const float CS[25] = {1.0f, 0.9659f, 0.8660f, 0.7071f, 0.5f, 0.2588f, 0.0f, -0.2588f, -0.5f, -0.7071f,
                                 -0.8660f, -0.9659f, -1.0f, -0.9659f, -0.8660f, -0.7071f, -0.5f, -0.2588f, 0.0f, 0.2588f,
                                 0.5f, 0.7071f, 0.8660f, 0.9659f, 1.0f};
    const int i0 = half == 2 ? 12 : 0, i1 = half == 1 ? 12 : 24;
    int i, k;
    for (k = 0; k < w; k++) {
        const float ax = rx + (float)k, ay = ry + (float)k;
        for (i = i0; i < i1; i++) {             /* 15 degrees a step: sin is cos a quarter later */
            const float s0 = CS[(i + 18) % 24], s1 = CS[(i + 19) % 24];
            cv_line(cx + (int32_t)(CS[i] * ax + (CS[i] < 0.0f ? -0.5f : 0.5f)),
                    cy - (int32_t)(s0 * ay + (s0 < 0.0f ? -0.5f : 0.5f)),
                    cx + (int32_t)(CS[i + 1] * ax + (CS[i + 1] < 0.0f ? -0.5f : 0.5f)),
                    cy - (int32_t)(s1 * ay + (s1 < 0.0f ? -0.5f : 0.5f)), c);
        }
    }
}

/* ------------------------------------------------------ the Delay's picture --- */
/* the echoes as ripples out from the sound (a full ring, white) on a time axis that opens out near it (x 42 +
 * 60 ln(1 + t / 0.1 s): 2 s at x 224, so short and long times both read); each repeat a half-ring as big as it's still
 * loud (48 sqrt(wet fb^k), a quarter as wide), the left channel's opening up in the FX blue and the right's down in aqua,
 * each dimmer as it ages, drawn while over -26 dB; Drive thickens the lines. Wide: the right at 0.75 of the time;
 * Ping-pong: odd repeats left, even right; Grain jittered, Self stretching (as the engine). Past Repeats 85 they stop
 * shrinking: runaway, in the header (the spec, 2026-10-31; the board, FX 266:4) */
static float lk_dly_x(float ms) { return 42.0f + 60.0f * fm_logf(1.0f + ms * 0.01f); }
static void lk_pic_delay(void)
{
    const int ty = (unsigned)proj.par[P_DLY_TYPE] < DR_NDLY ? proj.par[P_DLY_TYPE] : DR_DLY_TAPE;
    const float ms = dr_ext_spb > 0.0f && dr_dly_ms > 0.0f ? dr_dly_ms
                                                          : 70.0f * fm_exp2f((float)proj.par[P_DLY_TIME] * 0.026845f);
    const float fb = fm_minf((float)proj.par[P_DLY_REPEATS] * 0.0115f, 1.0f), mix = (float)proj.par[P_DLY_MIX] * 0.01f;
    const float wet = mix > 0.0f ? 0.25f + 0.75f * mix : 0.0f;
    const int w = (int)(2.0f + (float)proj.par[P_DLY_DRIVE] * 0.015f + 0.5f);   /* 2 px as every page, 3 from Drive 34,
                                                                                  * 4 at 100 (Evan, 2026-11-07) */
    const int32_t mid = 61;
    static const uint32_t C[2][2] = {{0xB4D9FFu, 0x1F6BFFu}, {0xB8FFF0u, 0x3CBFAFu}};   /* new, old: L blue, R aqua */
    int side, k;
    char sw[8];
    cv_text(8, 18, &LK_F_SMALL, "L", LK_LABEL);
    cv_text(8, 89, &LK_F_SMALL, "R", LK_LABEL);
    lk_ring(42, mid, 12.0f, 48.0f, 0, w, LK_INK);   /* the sound */
    for (side = 0; side < 2; side++) {
        float a = 1.0f;
        const float step = ms * (side && ty == 1 ? 0.75f : 1.0f);
        for (k = 1; k < 40; k++) {
            const int heard = ty != 2 || (k & 1) != side;   /* ping-pong: odd repeats on the left, even on the right */
            const float t = step * (float)k;
            const float x = lk_dly_x(t) + (float)echo_shift(ty, k);
            float hgt, age;
            a *= fb;
            hgt = 48.0f * fm_sqrtf(wet * a);
            if (x > 224.0f || wet * a < 0.0025f)   /* off the axis, or under -26 dB */
                break;
            if (!heard)
                continue;
            age = fm_minf((float)(k - 1) * 0.1f, 1.0f);
            lk_ring((int32_t)(x + 0.5f), side ? mid + 1 : mid, hgt * 0.25f, hgt, side ? 2 : 1, w,
                    lk_mix(0x07060Bu, lk_mix888(C[side][0], C[side][1], age), 1.0f - 0.4f * age));
        }
    }
    scopy(sw, "2 s");                          /* over the echoes */
    cv_rect(227 - text_w(&LK_F_SMALL, sw), 80, text_w(&LK_F_SMALL, sw) + 4, (int32_t)LK_F_SMALL.h - 2, LK_BG);
    cv_text(229 - text_w(&LK_F_SMALL, sw), 79, &LK_F_SMALL, sw, LK_LABEL);
}

/* ----------------------------------------------------- the Reverb's picture --- */
/* the tail as rings on a time axis that opens out near the start (x 62 + 44 ln(1 + t / 1 s): 40 s at the right), so
 * every Length from 0.3 s to 40 s draws differently: a ring at 0 and then each 1.75 times later, each as tall as the
 * tail is then (its dB falling in a line to -60 at the Length), none once it has died away, the first as tall as the mix
 * (12 + 46 mix px); as wide as the room (Room narrow, Cathedral wide); new ones pale, older ones the FX blue, drained
 * toward gray by Damp; 2 px, as every page (Evan, 2026-11-07). Inf: all one height, "freeze" in the header. At the left what goes in (Into), lit when the
 * reverb hears it (the spec, 2026-10-31, round 2; the board, FX 268:2) */
static void lk_pic_reverb(void)
{
    static const float WIDE[DR_NREV] = {0.16f, 0.24f, 0.324f, 0.27f};   /* Room, Hall, Cathedral, Chorus hall */
    const int ty = (unsigned)proj.par[P_REV_TYPE] < DR_NREV ? proj.par[P_REV_TYPE] : 2, into = proj.par[P_DLY_ROUTE];
    const float len = rev_secs(proj.par[P_REV_LEN]), mix = (float)proj.par[P_REV_MIX] * 0.01f;
    const float damp = (float)proj.par[P_REV_DAMP] * 0.01f, r0 = 12.0f + 46.0f * mix;
    const int32_t mid = 62;
    float t = 0.0f;
    int k;
    char tx[8];
    for (k = 0; k < 9; k++, t = k == 1 ? 0.42f : t * 1.75f) {
        const float x = 84.0f + 37.7f * fm_logf(1.0f + t), a = len <= 0.0f ? 1.0f : 1.0f - t / len;
        const float ry = r0 * a, age = (float)k / 6.0f;
        uint32_t c;
        if (x > 222.0f || ry < 2.0f)           /* off the axis, or died away */
            break;
        c = lk_mix888(lk_mix888(0xC9F6FFu, 0x507FCEu, fm_minf(age, 1.0f)), 0x3A3646u, fm_minf(damp * age, 0.85f));
        lk_ring((int32_t)(x + 0.5f), mid, 6.0f + WIDE[ty] * ry, ry, 0, 2, mix > 0.0f ? lk_mix(0x07060Bu, c, 1.0f)
                                                                                    : LK_UNLIT);
    }
    cv_text(12, 38, &LK_F_SMALL, "echoes", mix > 0.0f && into != 2 ? LK_INK : LK_LABEL);   /* Into: Both, Echoes, */
    cv_text(12, 50, &LK_F_SMALL, "drones", mix > 0.0f && into != 1 ? LK_INK : LK_LABEL);   /* Drones */
    scopy(tx, "40 s");                         /* over the rings */
    cv_rect(227 - text_w(&LK_F_SMALL, tx), 80, text_w(&LK_F_SMALL, tx) + 4, (int32_t)LK_F_SMALL.h - 2, LK_BG);
    cv_text(229 - text_w(&LK_F_SMALL, tx), 79, &LK_F_SMALL, tx, LK_LABEL);
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
    case V_PULSE: lk_pic_swell(); break;
    case V_SETUP: lk_pic_setup1(); break;
    case V_SETUP2: lk_pic_setup2(); break;
    case V_FX: lk_pic_delay(); break;
    case V_FX2: lk_pic_reverb(); break;
    default: lk_pic_drones1(); break;
    }
    if ((ui.view == V_FX || ui.view == V_FX2) && !msg) {   /* KNOB 4's second job, on the rule above Mix */
        char t[16];
        const int ps = sel_job(ui.view, 3), held = (int)(ui.btn >> B_SEL & 1u);
        if (held) {                            /* SEL held: the job it returns to, "mix 45" */
            char *q = scopy(t, "mix ");
            itoa_u((uint32_t)proj.par[knob_own(3)], q);
        } else {
            dr_param_text(ps, proj.par[ps], t);
        }
        lk_sel_tag(3, t, held);
    }
    if (msg)
        lk_banner();
    cv_blit(0, LK_PIC_Y);
}

/* ---------------------------------------------------------- values row --- */
/* four columns of 56, each its name (20, else 18, else 17), its value (26 turning, 21 at rest, the unit smaller,
 * shrinking when long) on one baseline, and its meter; the knob turning in its drone's outline */
/* THE METERS' ONE FAMILY (Evan, 2026-11-07): every plain amount a row of 8 dots of 5 px, each page in its own colors (the
 * cells, pills, stripes and rulers before it); the meaningful ones keep their own: a caution's red, the centered rows,
 * the crossfaders, one of n, X-Wave's ticks */
static void lk_dotrow(int32_t cx, int32_t y, float frac, uint32_t c0, uint32_t c1, int red, int gray)   /* red: the last
                                                                                                  * n red, the value
                                                                                                  * rounding up into
                                                                                                  * them; gray: lit
                                                                                                  * but unheard */
{
    const float v = fm_clampf(frac, 0.0f, 1.0f) * 8.0f;
    int i, n = red && v > (float)(8 - red) ? (int)(v + 0.999f) : (int)(v + 0.5f);
    for (i = 0; i < 8; i++) {
        const int32_t x = cx - 25 + (int32_t)((float)i * 6.5f + 0.5f);
        const int r = red && i >= 8 - red;
        if (i < n) {
            cv_round(x, y, 5, 5, 2, r ? LK_RED : gray ? LK_UNLIT : lk_mix(c0, c1, (float)i / 7.0f));
        } else {
            cv_round(x, y, 5, 5, 2, r ? LK_RED : LK_UNLIT);
            cv_round(x + 1, y + 1, 3, 3, 1, LK_BG);
        }
    }
}
static void lk_cdots_c(int32_t cx, int32_t y, float frac, uint32_t c0, uint32_t c1)   /* 9 dots lit from the middle
                                                                                     * toward the value, c0 at the
                                                                                     * middle to c1 at the ends */
{
    const int k = (int)((fm_clampf(frac, 0.0f, 1.0f) - 0.5f) * 8.0f + (frac >= 0.5f ? 0.5f : -0.5f));
    const int a = k < 0 ? 4 + k : 4, b = k < 0 ? 4 : 4 + k;
    int i;
    for (i = 0; i < 9; i++) {
        const int32_t x = cx - 26 + (int32_t)((float)i * 5.75f + 0.5f);
        if (i >= a && i <= b) {
            cv_round(x, y, 5, 5, 2, lk_mix(c0, c1, (float)(i < 4 ? 4 - i : i - 4) / 4.0f));
        } else {
            cv_round(x, y, 5, 5, 2, LK_UNLIT);
            cv_round(x + 1, y + 1, 3, 3, 1, LK_BG);
        }
    }
}
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

/* the Swell's meter: 9 pills of 5 in its coral ramp, unlit ones outlined; centred (Shape): from the middle one toward
 * gate (left) or ramp (right), even lighting the middle alone (the spec; the board, ARP 262:4) */
static void lk_dotrow(int32_t cx, int32_t y, float frac, uint32_t c0, uint32_t c1, int red, int gray);
static void lk_cdots_c(int32_t cx, int32_t y, float frac, uint32_t c0, uint32_t c1);
static void lk_pills(int32_t cx, int32_t y, float frac, int centred)   /* ARP's: a dot row in the Swell's coral, Shape's
                                                                       * from the middle (one meter family: Evan,
                                                                       * 2026-11-07, "dots, cells and pills look the
                                                                       * same") */
{
    if (centred)
        lk_cdots_c(cx, y, frac, 0xFF8F73u, 0xFFF0E6u);
    else
        lk_dotrow(cx, y, frac, 0xFF8F73u, 0xFFF0E6u, 0, 0);
}

static void lk_cells(int32_t cx, int32_t y, float frac, uint32_t c0, uint32_t c1, int caution)   /* ENV's: a dot row
                                                                                          * in its colors; caution
                                                                                          * (Feedback): the last two
                                                                                          * red */
{
    lk_dotrow(cx, y, frac, c0, c1, caution ? 2 : 0, 0);
}

static void lk_stripes(int32_t cx, int32_t y, float frac, int g, int gray)   /* EDIT's: a dot row in the drone's
                                                                                * ramp (was a stripe gauge), gray when
                                                                                * unheard */
{
    (void)y;
    lk_dotrow(cx, 70, frac, LK_RAMP[g][0], LK_RAMP[g][1], 0, gray);
}

static void lk_ticks4(int32_t cx, int32_t y)    /* under X-Wave's gauge: sine, triangle, saw, square */
{
    int k;
    for (k = 0; k < 4; k++)
        cv_rect(cx - 24 + (int32_t)(k * 46 / 3), y, 1, 3, LK_LABEL);
}

static void lk_cdots(int32_t cx, int32_t y, float frac)   /* Tune's: from the middle, in the neutral ramp */
{
    lk_cdots_c(cx, y, frac, LK_RAMP[LK_G][0], LK_RAMP[LK_G][1]);
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

static void lk_one_of_dots(int32_t cx, int32_t y, int n, int at, int g)   /* one dot of n lit (Mods' Source, Wander's
                                                                        * Scale: Evan, 2026-11-07, dots, not segments) */
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
    if (p == P_PULSE_RATE) {                   /* below 1 Hz the seconds a breath ("5.9s"), Hz above (the spec) */
        const float hz = dr_pulse_hz(v);
        if (hz < 1.0f) {
            const float sec = 1.0f / hz;
            const int d = sec < 10.0f ? (int)(sec * 10.0f + 0.5f) : (int)(sec + 0.5f) * 10;
            char *q = t;
            itoa_u((uint32_t)(d / 10), q);
            while (*q)
                q++;
            if (sec < 10.0f) {
                *q++ = '.';
                *q++ = (char)('0' + d % 10);
            }
            scopy(q, "s");
        }
        return;
    }
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
    if (p == P_CLK_SYNC) {                     /* what the clock is doing (the board's Sync states, 300:7) */
        if (!v) {
            scopy(t2, "free time");
        } else if (clk.following) {
            q = t2;
            itoa_u((uint32_t)clk.shown, q);
            while (*q)
                q++;
            scopy(q, " BPM");
        } else if (dr_ext_spb > 0.0f) {
            q = scopy(t2, "keeps ");
            itoa_u((uint32_t)(60.0f * DR_SR / dr_ext_spb + 0.5f), q);
        } else {
            scopy(t2, "no clock");
        }
        return 1;
    }
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
    if (p == P_PULSE_SHAPE) {                  /* the number large, its word small under it as a label ("96" over
                                                * "ramp"; gate is 100, even 0): the picture shows the shape (Evan,
                                                * 2026-11-03) */
        const int a = v < 0 ? -v : v;
        scopy(t2, v <= -100 ? "gate" : v < 0 ? "pulse" : v ? "ramp" : "even");
        itoa_u((uint32_t)(a > 100 ? 100 : a), t);
        return 1;
    }
    if (p == P_LFO_RMOD_SRC) {                 /* "LFO" over "I+II" (and "I", "II": it doesn't jump; Evan, 2026-11-03) */
        t[3] = 0;
        scopy(t2, t + 4);
        return 3;
    }
    return 0;
}

/* SEQ's ruler: ten ticks, lit tall, unlit short, a white mark at the value; centred (Shift's) lit from the middle */
static void lk_ruler(int32_t cx, int32_t y, float frac, int g, int centred)   /* SEQ's: a dot row (was a ruler),
                                                                                 * Shift's from the middle */
{
    const uint32_t c0 = g == LK_G ? 0xF2EEF8u : LK_RAMP[g][0], c1 = g == LK_G ? 0xF2EEF8u : LK_RAMP[g][1];
    if (centred)
        lk_cdots_c(cx, y, frac, c1, c0);
    else
        lk_dotrow(cx, y, frac, c0, c1, 0, 0);
}

/* SELECT, ALGORITHM or PRESETS turning (ui.glab): its name and value across the whole values row in one violet
 * outline (the spec); the page's picture stays (Evan, 2026-11-02: the old page showed behind it) */
static int lk_readout(void)
{
    const char *q;
    uint32_t h = hash(2166136261u, 0x4C47u);
    const felucca_font_t *fb;
    if (plat_ms() >= ui.glab_until)
        return 0;
    for (q = ui.glab[0]; *q; q++)
        h = hash(h, (uint32_t)*q);
    for (q = ui.glab[1]; *q; q++)
        h = hash(h, (uint32_t)*q << 8);
    if (h == ui.sig[2])
        return 1;
    ui.sig[2] = h;
    cv_begin(240, LK_VAL_H, LK_BG);
    cv_rect(8, 0, 224, 1, LK_RULE);
    cv_round(9, 4, 222, 81, 5, LK_VIOLET);
    cv_round(10, 5, 220, 79, 4, LK_BG);
    text_c(120, 9, &FONT_T20, ui.glab[0], LK_VIOLET);
    fb = text_w(&FONT_N1, ui.glab[1]) <= 210 ? &FONT_N1 : &FONT_N2;
    text_c(120, 9 + (int32_t)FONT_T20.h + (62 - (int32_t)FONT_T20.h - (int32_t)fb->h) / 2, fb, ui.glab[1], LK_WHITE);
    cv_blit(0, LK_VAL_Y);
    return 1;
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
    h = hash(h, (uint32_t)clk.shown | (uint32_t)clk.following << 12 | (uint32_t)(dr_ext_spb > 0.0f) << 13);   /* Sync */
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
        const uint16_t acc = ui.view == V_PULSE ? LK_SWELL : LK_DRONE[g];   /* the Swell's in its coral */
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
            cv_round(x + 1, 4, 54, 81, 5, acc);
            cv_round(x + 2, 5, 52, 79, 4, LK_BG);
        }
        name = second ? (p == P_SUBW_0 || p == P_SUBW_1 ? "Sub wave" : sel_job_name(p))
             : p == P_DLY_MIX || p == P_REV_MIX ? "Mix"   /* the page says which (the spec) */
             : card_name(p);
        fn = lk_name_font(name);
        text_c(cx, 7 + (int32_t)(FONT_T20.h - fn->h) / 2, fn, name, on || second ? acc : LK_LABEL);
        if (ui.view == V_LFO)                  /* the sends: each name underlined in its drone's color */
            cv_rect(cx - 22, 28, 44, 1, LK_DRONE[g]);
        lk_text(p, v, t);
        mode = second ? 0 : lk_line2(p, v, t, t2);
        if (mode == 3) {                       /* two lines at the value's size */
            const felucca_font_t *f2 = text_w(&FONT_N2, t) <= 58 && text_w(&FONT_N2, t2) <= 58 ? &FONT_N2 : &FONT_U1;
            text_c(cx, 27, f2, t, LK_WHITE);
            text_c(cx, 27 + (int32_t)f2->h - 4, f2, t2, LK_WHITE);
            if (ui.view == V_PULSE)
                lk_pills(cx, 70, hi > lo ? (float)(v - lo) / (float)(hi - lo) : 0.0f, p == P_PULSE_SHAPE);
            else
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
            static const felucca_font_t *const BIG[6] = {&FONT_N1, &FONT_N2, &FONT_U1, &FONT_U2, &FONT_XS, &FONT_U3},
                                         *const SMALL[6] = {&FONT_U1, &FONT_U2, &FONT_U2, &FONT_U2, &FONT_U2, &FONT_U2};
            static const uint8_t TUN_STEP[6] = {0, 1, 2, 4, 3, 5};   /* the Tuning's: 26, 21, 16, 15, 13, 12, to fit 52
                                                                      * px ("Harrison" 15, "Harmonics" 12; Evan,
                                                                      * 2026-11-07: a little smaller) */
            const int tun = p == P_TUNING;
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
            for (;; f++) {                     /* as large as fits: 26 turning, 21 at rest, else 16, else 13 (a long
                                                * name: "Cathedral", "Chorus hall", the board's size) */
                const int k = tun ? TUN_STEP[f] : f;
                fb = BIG[k];
                fs = SMALL[k];
                wb = text_w(fb, big);
                ws = *unit ? sep + text_w(fs, unit) : 0;
                if (wb + ws <= (f == 0 ? 50 : tun ? 52 : 58) || f == (tun ? 5 : 3))   /* 26 inside the outline; 21
                                                                 * may touch the column's edges (a note and its cents,
                                                                 * "Ab3 +17") */
                    break;
            }
            y = 9 + (int32_t)FONT_T20.h + (62 - (int32_t)FONT_T20.h - (int32_t)fb->h) / 2;
            if (mode == 1 && p != P_CLK_SYNC)  /* raised for the line under it (Sync's: in the meters' place, its
                                                * value on the others' line; Evan, 2026-11-07) */
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
            text_c(cx, p == P_CLK_SYNC ? 78 - LK_B(LK_F_TAG) : y + (int32_t)fb->h - 6,
                   text_w(&LK_F_TAG, t2) <= 54 ? &LK_F_TAG : &FONT_U2, t2,
                   p == P_CLK_SYNC && !clk.following ? LK_LABEL : LK_WHITE);   /* Sync: the tempo white, else gray;
                                                                                * "keeps 120" smaller to fit */
        if (p == P_CLK_SYNC)                   /* no meter: its line says it (the board) */
            continue;
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
            if (p == P_REV_TYPE)               /* the reverb's type: four segments, one lit */
                lk_one_of(cx, 70, hi - lo + 1, v - lo, g);
            else if (p == P_TUNING) {          /* a marker on a track: its place in the list (the spec) */
                cv_rect(cx - 25, 72, 50, 1, LK_UNLIT);
                cv_round(cx - 25 + (int32_t)(frac * 50.0f + 0.5f) - 3, 69, 7, 7, 3, LK_WHITE);
            } else if (p == P_TUNE)            /* a dot row from the middle, 440 there */
                lk_cdots(cx, 70, frac);
            else if (p == P_WIDTH)             /* Stereo a slider, mono .. wide */
                lk_xfader(cx, 70, frac, LK_G);
            else if (ui.view == V_PULSE)       /* ARP's pills; Shape's from the middle */
                lk_pills(cx, 70, frac, p == P_PULSE_SHAPE);
            else if (p == P_WSCALE)            /* Scale's dots, one-word names too (Evan: Dorian had a ruler) */
                lk_one_of_dots(cx, 72, hi - lo + 1, v - lo, g);
            else if (ui.view == V_PARTNER || ui.view == V_MOVES)   /* SEQ's rulers; Shift's from the middle */
                lk_ruler(cx, 70, frac, g, p == P_SHIFT_MOVE);
            else if (gp == GP_NCOLOR && ui.view == V_TONE)   /* a crossfade, white to pink (Evan, 2026-11-02) */
                lk_xfade_col(cx, 70, frac);
            else if (ui.view == V_LIFE || ui.view == V_TONE)   /* ENV's square cells */
                lk_cells(cx, 70, frac, LK_RAMP[g][0], LK_RAMP[g][1], p == P_FEEDBACK);
            else if (second)
                lk_one_of(cx, 70, hi - lo + 1, v - lo, g);
            else if (gp == GP_MIX)
                lk_xfader(cx, 70, frac, g);
            else if (gp == GP_WAVE) {
                lk_stripes(cx, 70, frac, g, 0);
                lk_ticks4(cx, 77);             /* under the dots: sine, triangle, saw, square */
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
    if (!lk_readout())
        lk_values();
    return 1;
}
