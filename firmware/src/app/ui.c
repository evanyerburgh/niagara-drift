/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift UI: the control map (Figma "Niagara Drift control map: draft 1"). Two drones, after the CPM DS-2.
 * Adapted from FoMni's ui.c: its drawing helpers, knob acceleration and three-band screen. Part of the
 * unity build (after gfx.c and project.c); the host simulator includes it too.
 *
 * Always live:
 *   KNOB 1-2       drone I's two values on the page shown, KNOB 3-4 drone II's; the Play page (HOME):
 *                  Pitch and Cutoff. A pitch is continuous, 27 Hz .. 1.6 kHz, a slow full turn an octave;
 *                  hold SEL for a cent a detent. With PITCH LINK on, KNOB 3 moves drone II's offset.
 *   SELECT         LFO rate      PRESETS   LFO Wave I (left)      ALGORITHM   LFO Wave II (right)
 *   PLAY           the drone fades in / out      MASTER   volume      SAVE   saves
 * One button away (tap: the knobs stay on its page, pressed again its second page; hold: they switch
 * only while it's held, and the page you were on comes back when it's let go):
 *   HOME Play, Play 2 (Reso, Level) · ENV Tone (Drive, Osc mix), Tone 2 (Noise, Colour) · EDIT Osc (X-Wave, PW) · LFO LFO
 *   (depths), Mod (Rate mod, Mod by, X-mod amount, Delay LFO depth) · SEQ Partner · GLO Setup, Setup 2
 *   (Morph, Tune, Scene switch) (global). The top bar shows a dot for each of a button's pages.
 *   OCT- / OCT+    MOVEMENT: every LFO depth down / up together, 10 % a tap (held: on and on), 0 .. 200 %,
 *                  gliding; lit while above (OCT+) or below (OCT-) 100 %
 * White keys: 16 scenes. Tap: recall (every value morphs there over the Scene switch time; a knob turned stops
 *   it); hold (a visit): there while held, back to where it was when let go (a knob turned: it stays);
 *   hold SAVE and tap: store. Lit: the slot holds a scene; blinking slowly: the sound is that scene (stored or
 *   recalled, and no knob, switch or LFO control touched since).
 * Black keys, lit when on (or on the second choice), following the page:
 *   F#3 G#3 A#3   C#4 D#4   F#4 G#4 A#4   C#5 D#5   F#5. Normally, by meaning: drone I (Partner, Sub
 *   (round: -1, -2 oct, 1/3, 1/5), Filter (round: LP, BP, HP)) | global (Pitch lock, LFO reset) | drone II
 *   (the same) | links (Pitch link, Hard sync) | Cross-mod. On the LFO page: Osc wave, Osc LFO -> X-Wave, Filter LFO -> Reso | Filter wave,
 *   LFO reset | Osc wave, Osc LFO -> X-Wave, Filter LFO -> Reso | Filter wave, Delay wave | Cross-mod. A
 *   wave key goes round I, II, I+II (lit for II, winking for both). LFO reset (either layer): a tap restarts
 *   the LFO; held half a second, it locks the LFO to the Pulse's bars (restarting each bar; lit while
 *   locked), held again, free.
 * ARP            tap: the SWELL on / off (lit, in time); hold: its page (Rate, Depth, Shape, Swing); two
 *                quick taps: its page, to stay
 * REC            the MOTION LOOP: tap to record what the knobs do, tap again to loop it from the start,
 *                tap again to stop (everything stays where it is); up to 5 minutes (lit: blinking recording,
 *                steady looping)
 *   FX             the effects: Delay time, Repeats, Delay mix, Reverb mix; again: Reverb type
 *                  (the delay LFO's tap: the LFO page's key in drone II's filter group)
 * Saving: SAVE, and by itself a few seconds after a change while stopped (a flash write stops the audio
 * for a moment). MIDI in: notes on channel 1 tune drone I, on channel 2 drone II. */

/* ------------------------------------------------------------- pages --- */
/* the pages (2026-10-10, from the review: what's played together sits together, no half-empty pages) */
enum { V_PLAY, V_PLAY2, V_TONE, V_TONE2, V_OSC, V_LFO, V_LFO2, V_PARTNER, V_SETUP, V_SETUP2, V_PULSE, V_FX, V_FX2, NVIEWS };
static const char *const VIEW_NAME[NVIEWS] = {"Play 1", "Play 2", "Tone 1", "Tone 2", "Osc", "LFO depths", "LFO mods", "Partner",
                                              "Setup 1", "Setup 2", "Swell", "Effects 1", "Effects 2"};
/* what each knob moves on each page: a drone's parameter (g 0 or 1, GP_*), a global one (g G, P_*), or
 * nothing (g NONE) */
#define G 2
#define NONE 3
typedef struct {
    uint8_t g, p;
} knob_t;
static const knob_t VIEW_KNOB[NVIEWS][4] = {
    {{0, GP_PITCH}, {0, GP_CUTOFF}, {1, GP_PITCH}, {1, GP_CUTOFF}},
    {{0, GP_RESO}, {0, GP_LEVEL}, {1, GP_RESO}, {1, GP_LEVEL}},
    {{0, GP_DRIVE}, {0, GP_MIX}, {1, GP_DRIVE}, {1, GP_MIX}},
    {{0, GP_NOISE}, {0, GP_NCOLOR}, {1, GP_NOISE}, {1, GP_NCOLOR}},
    {{0, GP_WAVE}, {0, GP_WIDTH}, {1, GP_WAVE}, {1, GP_WIDTH}},
    {{0, GP_OLFO_DEPTH}, {0, GP_FLFO_DEPTH}, {1, GP_OLFO_DEPTH}, {1, GP_FLFO_DEPTH}},
    {{G, P_LFO_RMOD}, {G, P_LFO_RMOD_SRC}, {G, P_XMOD_AMT}, {G, P_DLY_LFO_DEPTH}},
    {{0, GP_INTERVAL}, {0, GP_DETUNE}, {1, GP_INTERVAL}, {1, GP_DETUNE}},
    {{G, P_GLIDE}, {G, P_FADE}, {G, P_DRIFT}, {G, P_WIDTH}},
    {{G, P_MORPH}, {G, P_TUNE}, {G, P_SCENE_T}, {G, P_JUST}},
    {{G, P_PULSE_RATE}, {G, P_PULSE_DEPTH}, {G, P_PULSE_SHAPE}, {G, P_PULSE_SWING}},
    {{G, P_DLY_TIME}, {G, P_DLY_REPEATS}, {G, P_DLY_MIX}, {G, P_REV_MIX}},
    {{G, P_REV_TYPE}, {G, P_SPEAKER}, {G, P_FEEDBACK}, {G, P_CHAR}},
};
/* the page buttons: each shows its first page, pressed again the next */
static const struct {
    uint8_t btn, first, n;
} PAGE_BTN[] = {{B_HOME, V_PLAY, 2}, {B_ENV, V_TONE, 2}, {B_EDIT, V_OSC, 1}, {B_LFO, V_LFO, 2},
                {B_SEQ, V_PARTNER, 1}, {B_GLO, V_SETUP, 2}, {B_FX, V_FX, 2},
                {B_ARP, V_PULSE, 1}};                                    /* ARP: a tap toggles */
#define NPAGE_BTN (sizeof PAGE_BTN / sizeof PAGE_BTN[0])
#define HOLD_MS 400u                   /* a page button held this long (or a knob turned) is a peek */

/* ------------------------------------------------------------ switches --- */
/* the black keys (key numbers, F3 = 0) and what each does: normally, and on the LFO page */
static const uint8_t BLACK_K[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t WHITE_K[NSCENES] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
enum { S_NONE, S_PARTNER, S_SUBOCT, S_HP, S_OTAP, S_ODEST, S_FTAP, S_FDEST, S_PLINK, S_SYNC, S_XMOD, S_LFORESET, S_DTAP,
       S_PLOCK };
typedef struct {
    uint8_t s, g;
} sw_t;
/* normally, by meaning: drone I (partner, sub, filter) | the global two (pitch lock, LFO reset) | drone II |
 * the links (pitch link, hard sync) | cross-mod */
static const sw_t SW_NORMAL[11] = {{S_PARTNER, 0}, {S_SUBOCT, 0}, {S_HP, 0}, {S_PLOCK, 0}, {S_LFORESET, 0},
                                   {S_PARTNER, 1}, {S_SUBOCT, 1}, {S_HP, 1}, {S_PLINK, 1}, {S_SYNC, 1},
                                   {S_XMOD, 1}};
static const sw_t SW_LFO[11] = {{S_OTAP, 0}, {S_ODEST, 0}, {S_FDEST, 0}, {S_FTAP, 0}, {S_LFORESET, 0},
                                {S_OTAP, 1}, {S_ODEST, 1}, {S_FDEST, 1}, {S_FTAP, 1}, {S_DTAP, 1},
                                {S_XMOD, 1}};

/* the screen's colours: dark, glowing (Evan, 2026-10-09; Figma "Screen concepts", concept D) */
#define K_BG RGB(10, 12, 16)                 /* near black */
#define K_PANEL RGB(18, 22, 28)              /* the pictures' panels */
#define K_CARD RGB(21, 27, 35)               /* a lit value, a banner */
#define K_GRID RGB(28, 34, 43)               /* bars' tracks, lines */
#define K_FAINT RGB(58, 65, 76)
#define K_DIM RGB(107, 115, 128)
#define K_TEXT RGB(233, 230, 223)            /* ink */
#define K_TEXT2 RGB(186, 184, 178)           /* values at rest */
#define K_LFO RGB(183, 145, 255)
#define K_FX RGB(159, 180, 255)
#define K_PULSE RGB(255, 122, 168)
#define K_SETUP RGB(201, 196, 186)
#define K_GOLD RGB(255, 196, 92)             /* messages without a drone */
static const uint16_t K_DRONE[DR_NGROUPS] = {RGB(255, 176, 74), RGB(69, 227, 211)};   /* amber, cyan */
static const uint16_t K_DRONE_DIM[DR_NGROUPS] = {RGB(92, 66, 34), RGB(30, 84, 80)};   /* their glow */

static struct {
    uint8_t view;
    uint8_t playing;                   /* PLAY on: the drone faded in, or fading in */
    uint32_t btn, keys, btn_used;
    uint32_t enc_t[NE];
    int8_t hold_btn;                   /* the page button held (B_*), or -1 */
    uint8_t hold_from, hold_cycle, hold_turned;
    uint32_t hold_t;
    uint8_t dirty;                     /* the project changed since the last save */
    uint32_t act_t;
    int8_t save_q;                     /* a save waiting for the dip (playing): -2 none, -1 the sound, N scene N */
    uint32_t save_t;                   /* ... asked then */
    uint32_t oct_next;                 /* OCT- / OCT+ held: when the next Movement step comes */
    uint32_t arp_t;                    /* the last ARP tap (two quick taps: stay on the Swell page) */
    uint8_t v2_out;                    /* drone II faded out on its own (SEL + PLAY) */
    uint32_t rst_t;                    /* when the LFO reset key went down (a hold locks the LFO to the pulse) */
    uint8_t rst_done;                  /* that hold has acted */
    char msg[2][24];
    uint32_t msg_until;
    uint16_t msg_col;                  /* the banner's colour */
    char glab[2][16];                  /* SELECT / ALGORITHM / PRESETS turned: their name and value (top bar) */
    uint32_t glab_until;
    uint32_t touch_t[4];               /* each knob: lit until then (each its own timer: two hands) */
    int8_t morph;                      /* the scene being morphed to (VISIT_BACK: back from a visit), or -1 */
    int8_t visit;                      /* the white key held (a visit, if it's held long), or -1 */
    uint32_t visit_t;                  /* ... since */
    int16_t visit_back[DR_NPARAMS];    /* ... and the sound before it: where letting go morphs back to */
    int8_t scene_at;                   /* the scene the sound is (stored or recalled, no hand on it since), or -1:
                                        * its white key blinks (Evan, on the FM-1, 2026-10-14) */
    int8_t visit_from;                 /* ... before a visit: where letting go comes back to */
    uint8_t loop;                      /* the motion loop: L_IDLE, L_REC, L_PLAY */
    uint32_t loop_t0, loop_len;        /* recording: its start; playing: this pass's start, the length */
    uint16_t loop_n, loop_i;           /* events recorded, the next to play */
    int16_t loop_from[DR_NPARAMS];     /* every value when the recording began */
    uint32_t morph_t0, morph_ms;
    int16_t morph_from[DR_NPARAMS];
    uint32_t frame;
    uint32_t sig[3];                   /* what each band shows: redrawn when it changes */
    uint32_t splash_t0;                /* the start's animation: since */
    uint8_t splash_done;
} ui;

/* a message: a banner across the top of the picture for a moment (the knobs keep the bottom) */
static void say_c(const char *a, const char *b, uint16_t col);
static void say(const char *a, const char *b) { say_c(a, b, 0); }
static void say_c(const char *a, const char *b, uint16_t col)
{
    ui.msg_col = col;
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
}
void ui_say(const char *a, const char *b) { say(a, b); }

static void mark_dirty(void) { ui.dirty = 1; }
int ui_dirty(void) { return ui.dirty != 0; }

static void itoa_u(uint32_t v, char *b)
{
    char t[12];
    int n = 0;
    do {
        t[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static char *scopy(char *d, const char *s)          /* copies, returns the end */
{
    while ((*d = *s++))
        d++;
    return d;
}

/* ------------------------------------------------------------ values --- */
static void param_set(int p, int v)
{
    const dr_param_t *i = dr_param_info(p);
    v = v < i->lo ? i->lo : v > i->hi ? i->hi : v;
    if (v == proj.par[p])
        return;
    proj.par[p] = (int16_t)v;
    dr_set(p, v);
    mark_dirty();
}

static void loop_note(int p, int v, uint32_t slide);  /* the motion loop notes knob moves (below) */

/* the parameter knob i moves on the page shown, or -1 (on the Play page, KNOB 3 is the link offset while
 * PITCH LINK is on) */
static int knob_param(int i)
{
    knob_t k = VIEW_KNOB[ui.view][i];
    if (k.g == NONE)
        return -1;
    if (k.g == G)
        return k.p;
    if (k.g == 1 && k.p == GP_PITCH && proj.par[P_PLINK])
        return P_PLINK_OFF;
    return DR_GP(k.g, k.p);
}

/* knob acceleration by speed (FoMni's): a slow turn is one step a detent, a quick one up to 6 */
static uint8_t accel_off;              /* the host simulator's "spin": exact steps */
static int32_t accel(int role, int32_t s, int range)
{
    uint32_t now = plat_ms(), dt = now - ui.enc_t[role], a = (uint32_t)(s < 0 ? -s : s), m;
    ui.enc_t[role] = now;
    if (range <= 24 || !a || accel_off)
        return s;
    if (a > 1)
        dt /= a;
    m = dt < 15u ? 6u : dt < 30u ? 4u : dt < 60u ? 2u : 1u;
    return s * (int32_t)m;
}

/* a pitch knob (or the link offset). A detent is 60 cents (a slow full turn of 20 detents: an octave),
 * more when turned quickly, on a smooth curve (up to 2x at 70 detents a second: Evan, on the FM-1, 4x went
 * too far); with SEL held, 1 cent.
 * Each change slides over the time since the last detent (15..600 ms): the next one is due about then,
 * so a turn at any speed, even a slow uneven drag, is one continuous slide, as an analog knob's; a
 * first detent after a pause slides over 300 ms. (At 250 ms, detents a third of a second apart left
 * 60-cent stairs.) */
#define PITCH_CENTS 60
static void pitch_turn(int g, int p, int role, int32_t s)
{
    uint32_t now = plat_ms(), dt = now - ui.enc_t[role], a = (uint32_t)(s < 0 ? -s : s), slide;
    int cents;
    if (proj.par[P_PLOCK]) {                           /* locked against a bumped knob */
        say_c("PITCH LOCKED", "KEY 4", K_GOLD);
        return;
    }
    ui.enc_t[role] = now;
    if (a > 1)
        dt /= a;                                       /* several detents since the last look */
    /* the glide: the time since the last detent (when the next is due, so a steady turn never stops),
     * 15 .. 600 ms; after a pause (a first detent), 300 ms */
    slide = accel_off ? 15u : dt < 15u ? 15u : dt > 600u ? 300u : dt;
    if (ui.btn >> B_SEL & 1u) {                        /* fine: SEL held */
        ui.btn_used |= 1u << B_SEL;
        cents = (int)s;
    } else {
        float v = 1000.0f / (float)(dt ? dt : 1u), x = fm_clampf((v - 10.0f) * (1.0f / 60.0f), 0.0f, 1.0f);
        cents = (int)s * (int)((float)PITCH_CENTS * (accel_off ? 1.0f : 1.0f + x * x) + 0.5f);
    }
    dr_slide(g, (int)slide);
    param_set(p, proj.par[p] + cents);
    loop_note(p, proj.par[p], slide);
}

/* a continuous knob (cutoff, resonance, X-Wave, a depth, a level...: a range over 24 steps), as a pitch
 * knob: a detent is a step, more when turned quickly (up to 4x, on the same smooth curve), and each glides
 * over the time since the last detent (15..600 ms; 300 after a pause), so a slow turn is a sweep */
static void cont_turn(int p, int role, int32_t s)
{
    uint32_t now = plat_ms(), dt = now - ui.enc_t[role], a = (uint32_t)(s < 0 ? -s : s), slide;
    int steps;
    ui.enc_t[role] = now;
    if (a > 1)
        dt /= a;
    slide = accel_off ? 15u : dt < 15u ? 15u : dt > 600u ? 300u : dt;
    {
        float v = 1000.0f / (float)(dt ? dt : 1u), x = fm_clampf((v - 10.0f) * (1.0f / 60.0f), 0.0f, 1.0f);
        steps = (int)s * (int)(accel_off ? 1.0f : 1.0f + 3.0f * x * x + 0.5f);
    }
    {   /* cutoff and resonance run 0..1000: a detent is 10, with SEL held 1 (fine, as a pitch's cent) */
        int gp = p < P_XMOD ? p % GP_N : -1;
        if (gp == GP_CUTOFF || gp == GP_RESO) {
            if (ui.btn >> B_SEL & 1u) {
                ui.btn_used |= 1u << B_SEL;
                steps = (int)s;
            } else {
                steps *= 10;
            }
        }
    }
    dr_pslide(p, (int)slide);
    param_set(p, proj.par[p] + steps);
    loop_note(p, proj.par[p], slide);
}

static void knob_turn(int i, int32_t e)
{
    int p = knob_param(i);
    knob_t k = VIEW_KNOB[ui.view][i];
    if (p < 0)
        return;
    if (k.p == GP_PITCH && k.g < G)
        pitch_turn(k.g, p, EN_K1 + i, e);
    else if (dr_param_info(p)->hi - dr_param_info(p)->lo > 24)
        cont_turn(p, EN_K1 + i, e);
    else {
        param_set(p, proj.par[p] + accel(EN_K1 + i, e, dr_param_info(p)->hi - dr_param_info(p)->lo));
        loop_note(p, proj.par[p], 0);
    }
}

/* ------------------------------------------------------------ switches --- */
static const sw_t *sw_layer(void) { return ui.view == V_LFO || ui.view == V_LFO2 ? SW_LFO : SW_NORMAL; }

/* a route's wave as the key shows it: I 0 (dark), II 1 (lit), I+II 2 (lit, winking) */
static int tap_lit(int v) { return v == DR_LFO_BOTH ? 2 : v == DR_LFO_II; }

static int sw_get(sw_t w)                              /* 1: lit (on, or the second choice); 2: a third */
{
    const int16_t *p = &proj.par[DR_GP(w.g, 0)];
    switch (w.s) {
    case S_PARTNER: return p[GP_PARTNER] != 0;
    case S_SUBOCT: return p[GP_SUBOCT] >= 3 ? 2 : p[GP_SUBOCT] == 2;   /* -2 oct lit; a subharmonic winks */
    case S_PLOCK: return proj.par[P_PLOCK] != 0;
    case S_HP: return p[GP_MODE] == DR_HP ? 1 : p[GP_MODE] == DR_BP ? 2 : 0;   /* HP lit, BP winking */
    case S_OTAP: return tap_lit(p[GP_OLFO]);
    case S_ODEST: return p[GP_OLFO_DEST] == DR_TO_XWAVE;
    case S_FDEST: return p[GP_FLFO_DEST] == DR_TO_RESO;
    case S_FTAP: return tap_lit(p[GP_FLFO]);
    case S_PLINK: return proj.par[P_PLINK] != 0;
    case S_SYNC: return proj.par[P_SYNC] != 0;
    case S_XMOD: return proj.par[P_XMOD] != 0;
    case S_DTAP: return tap_lit(proj.par[P_DLY_LFO]);
    default: return 0;
    }
}

/* PITCH LINK on: drone II keeps its pitch, as drone I's + the offset; off: it keeps it as its own */
static void link_set(int on)
{
    int p1 = proj.par[DR_GP(0, GP_PITCH)], p2 = proj.par[DR_GP(1, GP_PITCH)];
    if (on) {
        param_set(P_PLINK_OFF, p2 - p1);
    } else {
        int now = p1 + proj.par[P_PLINK_OFF];
        param_set(DR_GP(1, GP_PITCH), now);
    }
    param_set(P_PLINK, on);
}

static void sw_toggle(sw_t w)
{
    static const char *const DR[2] = {"I ", "II "};
    static const char *const WAVES[4] = {"OFF", "WAVE I", "WAVE II", "WAVES I+II"};
    char a[24];
    const char *what = 0, *st = 0;
    int on = !sw_get(w), base = DR_GP(w.g, 0), tp = -1, nx;
    /* a route's key goes round: Wave I, Wave II, both (I+II), Wave I */
    if (w.s == S_OTAP || w.s == S_FTAP || w.s == S_DTAP)
        tp = w.s == S_OTAP ? base + GP_OLFO : w.s == S_FTAP ? base + GP_FLFO : P_DLY_LFO;
    nx = tp < 0 ? 0 : proj.par[tp] == DR_LFO_I ? DR_LFO_II : proj.par[tp] == DR_LFO_II ? DR_LFO_BOTH : DR_LFO_I;
    char *q = scopy(a, DR[w.g]);
    switch (w.s) {
    case S_PARTNER: param_set(base + GP_PARTNER, on); what = "PARTNER"; st = on ? "ON" : "OFF"; break;
    case S_SUBOCT: {                           /* round: -1 oct, -2 oct, 1/3, 1/5 */
        static const char *const SUB[5] = {"", "-1 OCT", "-2 OCT", "1/3", "1/5"};
        int v = proj.par[base + GP_SUBOCT] % 4 + 1;
        param_set(base + GP_SUBOCT, v);
        what = "SUB";
        st = SUB[v];
        break;
    }
    case S_PLOCK: param_set(P_PLOCK, on); q = a; what = "PITCH LOCK"; st = on ? "ON" : "OFF"; break;
    case S_HP: {                               /* round: LP, BP, HP */
        static const char *const MODE[3] = {"LP", "BP", "HP"};
        int m = (proj.par[base + GP_MODE] + 1) % 3;
        param_set(base + GP_MODE, m);
        what = "FILTER";
        st = MODE[m];
        break;
    }
    case S_OTAP: param_set(tp, nx); what = "OSC LFO"; st = WAVES[nx]; break;
    case S_ODEST: param_set(base + GP_OLFO_DEST, on); what = "OSC LFO TO"; st = on ? "X-WAVE" : "PITCH"; break;
    case S_FDEST: param_set(base + GP_FLFO_DEST, on); what = "FILTER LFO TO"; st = on ? "RESONANCE" : "CUTOFF"; break;
    case S_FTAP: param_set(tp, nx); what = "FILTER LFO"; st = WAVES[nx]; break;
    case S_PLINK: link_set(on); q = a; what = "PITCH LINK"; st = on ? "ON" : "OFF"; break;
    case S_SYNC: param_set(P_SYNC, on); q = a; what = "HARD SYNC"; st = on ? "ON" : "OFF"; break;
    case S_XMOD: param_set(P_XMOD, on); q = a; what = "I > II CUTOFF"; st = on ? "ON" : "OFF"; break;
    case S_LFORESET: dr_lfo_reset(); q = a; what = "LFO"; st = "RESET"; break;
    case S_DTAP: param_set(tp, nx); q = a; what = "DELAY LFO"; st = WAVES[nx]; break;
    default: return;
    }
    scopy(q, what);
    say_c(a, st, w.s == S_LFORESET || w.s == S_DTAP ? RGB(183, 145, 255) : w.g ? RGB(69, 227, 211) : RGB(255, 176, 74));
}

/* ------------------------------------------------------------ buttons --- */
static void set_view(int v)
{
    int i;
    ui.view = (uint8_t)v;
    for (i = 0; i < 4; i++)
        ui.touch_t[i] = 0;
}

static int page_btn(int b)                             /* the index in PAGE_BTN, or -1 */
{
    uint32_t i;
    for (i = 0; i < NPAGE_BTN; i++)
        if (PAGE_BTN[i].btn == b)
            return (int)i;
    return -1;
}
static int on_pages(int i) { return ui.view >= PAGE_BTN[i].first && ui.view < PAGE_BTN[i].first + PAGE_BTN[i].n; }

/* a page button pressed: its first page at once, unless already on its pages (then the next on a tap) */
static void page_down(int b)
{
    int i = page_btn(b);
    ui.hold_btn = (int8_t)b;
    ui.hold_t = plat_ms();
    ui.hold_turned = 0;
    ui.hold_from = ui.view;
    ui.hold_cycle = (uint8_t)on_pages(i);
    if (!ui.hold_cycle)
        set_view(PAGE_BTN[i].first);
}
/* let go: after a hold (or a knob turned while held), back where it was; after a tap, stay (or step on) */
static void page_up(int b)
{
    int i = page_btn(b), peek;
    if (ui.hold_btn != b)
        return;
    peek = ui.hold_turned || plat_ms() - ui.hold_t >= HOLD_MS;
    if (b == B_ARP) {                                  /* ARP: a tap is the pulse on / off; its page only */
        if (!peek) {                                   /* a tap: the swell on / off; two quick taps: */
            uint32_t now = plat_ms();                  /* its page, to stay (the first tap undone) */
            param_set(P_PULSE, !proj.par[P_PULSE]);
            if (now - ui.arp_t < 350u) {
                ui.arp_t = 0;
                set_view(V_PULSE);
                ui.hold_btn = -1;
                say("SWELL", "PAGE");
                return;
            }
            ui.arp_t = now;
            say("SWELL", proj.par[P_PULSE] ? "ON" : "OFF");
        }
        if (!ui.hold_cycle)
            set_view(ui.hold_from);
        ui.hold_btn = -1;
        return;
    }
    if (peek) {
        if (!ui.hold_cycle)
            set_view(ui.hold_from);
    } else if (ui.hold_cycle) {
        set_view(PAGE_BTN[i].first + (ui.view - PAGE_BTN[i].first + 1) % PAGE_BTN[i].n);
    }
    ui.hold_btn = -1;
}

static float morph_ms(void) { return 20.0f * fm_exp2f((float)proj.par[P_MORPH] * 0.105507f); }

/* OCT- / OCT+: the MOVEMENT, a step (10 %) a tap, gliding; held, a step every 120 ms after 400 ms. Shown in
 * the top bar as a knob off the page is */
static void movement_step(int dir)
{
    char t[12];
    int v = proj.par[P_MOVE] + 5 * dir;
    v = v < 0 ? 0 : v > 100 ? 100 : v;
    dr_pslide(P_MOVE, 150);
    param_set(P_MOVE, v);
    ui.scene_at = -1;
    loop_note(P_MOVE, v, 150);
    dr_param_text(P_MOVE, v, t);
    scopy(ui.glab[0], "MOVEMENT");
    scopy(ui.glab[1], t);
    ui.glab_until = plat_ms() + 1500u;
    ui.oct_next = plat_ms() + 400u;
}

/* -------------------------------------------------------- motion loop --- */
enum { L_IDLE, L_REC, L_PLAY };
#define LOOP_MAX 4096                  /* events */
#define LOOP_MS 300000u                /* the longest loop: 5 minutes */
typedef struct {
    uint16_t t;                        /* ms into the loop / 5 (5 minutes fit) */
    uint8_t p, slide;                  /* the parameter; its glide, ms / 2 */
    int16_t v;
} loop_ev_t;
static loop_ev_t loop_ev[LOOP_MAX] OM_POOL;
static uint64_t loop_used[(DR_NPARAMS + 63) / 64];     /* the parameters the loop moves */

/* a knob moved a value: noted while recording. Detents of the same knob within LOOP_JOIN of an event (a quick
 * turn: a detent every ~15 ms) join it, its slide reaching on to this one, so the 4096 events last (a busy
 * two-handed recording filled them in 1-2 minutes; the loop then held still) */
#define LOOP_JOIN 20u                  /* 100 ms, in the events' 5 ms units */
static void loop_note(int p, int v, uint32_t slide)
{
    loop_ev_t *e;
    uint16_t t = (uint16_t)((plat_ms() - ui.loop_t0) / 5u);
    int i;
    if (ui.loop != L_REC || ui.loop_n >= LOOP_MAX)
        return;
    for (i = ui.loop_n - 1; i >= 0 && i >= ui.loop_n - 8; i--) {
        e = &loop_ev[i];
        if (t - e->t >= LOOP_JOIN)
            break;
        if (e->p == p) {
            uint32_t sl = (uint32_t)(t - e->t) * 5u + slide;
            e->v = (int16_t)v;
            e->slide = (uint8_t)(sl > 510u ? 255u : sl / 2u);
            return;
        }
    }
    e = &loop_ev[ui.loop_n++];
    e->t = t;
    e->p = (uint8_t)p;
    e->v = (int16_t)v;
    e->slide = (uint8_t)(slide > 510u ? 255u : slide / 2u);
    loop_used[p / 64] |= 1ull << (p % 64);
}

static int loop_moves(int p) { return (int)(loop_used[p / 64] >> (p % 64) & 1u); }

static void loop_apply(int p, int v, uint32_t slide)
{
    if (p == DR_GP(0, GP_PITCH) || p == DR_GP(1, GP_PITCH))
        dr_slide(p / GP_N, (int)slide);
    else if (p == P_PLINK_OFF)
        dr_slide(1, (int)slide);
    else if (slide)
        dr_pslide(p, (int)slide);
    param_set(p, v);
}

static void loop_rewind(void)                  /* the start of a pass: what the loop moves, back to how it */
{                                              /* began, gliding over the Morph time (no seam) */
    int p;
    uint32_t back = (uint32_t)morph_ms();
    for (p = 0; p < DR_NPARAMS; p++)
        if (loop_moves(p))
            loop_apply(p, ui.loop_from[p], back < 15u ? 15u : back);
    ui.loop_i = 0;
}

static void loop_button(void)                  /* REC: record, loop, stop */
{
    uint32_t now = plat_ms();
    int p;
    switch (ui.loop) {
    case L_IDLE:
        for (p = 0; p < DR_NPARAMS; p++)
            ui.loop_from[p] = proj.par[p];
        for (p = 0; p < (int)(sizeof loop_used / sizeof loop_used[0]); p++)
            loop_used[p] = 0;
        ui.loop_n = 0;
        ui.loop_t0 = now;
        ui.loop = L_REC;
        say("MOTION LOOP", "RECORDING");
        break;
    case L_REC:
        if (!ui.loop_n) {
            ui.loop = L_IDLE;
            say("MOTION LOOP", "NOTHING MOVED");
            break;
        }
        ui.loop_len = now - ui.loop_t0;
        ui.loop = L_PLAY;
        ui.loop_t0 = now;
        loop_rewind();
        say("MOTION LOOP", "PLAYING");
        break;
    default:
        ui.loop = L_IDLE;                      /* stopped: everything stays where it is */
        say("MOTION LOOP", "STOPPED");
        break;
    }
}

static void loop_run(void)                     /* every millisecond: recording's limit, playing's events */
{
    uint32_t now = plat_ms();
    if (ui.loop == L_REC && (now - ui.loop_t0 >= LOOP_MS || ui.loop_n >= LOOP_MAX)) {
        int full = ui.loop_n >= LOOP_MAX;
        loop_button();                         /* 5 minutes, or its memory full: it starts looping by itself */
        if (full)
            say("MOTION LOOP", "FULL: PLAYING");
    }
    if (ui.loop != L_PLAY)
        return;
    if (now - ui.loop_t0 >= ui.loop_len) {     /* the end of a pass: around again */
        ui.loop_t0 += ui.loop_len;
        loop_rewind();
    }
    while (ui.loop_i < ui.loop_n && (uint32_t)loop_ev[ui.loop_i].t * 5u <= now - ui.loop_t0) {
        const loop_ev_t *e = &loop_ev[ui.loop_i++];
        loop_apply(e->p, e->v, (uint32_t)e->slide * 2u);
    }
}

/* ------------------------------------------------------------- scenes --- */
static int scene_step(int p)                           /* a value that steps (a switch, a list): it flips */
{
    const dr_param_t *in = dr_param_info(p);
    return in->hi - in->lo <= 9;
}

/* settings of the room, not the sound: a scene doesn't carry them (the speaker and the stereo spread, which
 * depend on speaker or headphones; the tuning to the band; the pitch lock; how long things take) */
static int scene_room(int p)
{
    return p == P_FADE || p == P_TUNE || p == P_SPEAKER || p == P_WIDTH || p == P_PLOCK || p == P_MORPH ||
           p == P_SCENE_T;
}

/* into the flash now: n = -1 the sound (SAVE), else scene n (just stored) */
static void save_now(int n)
{
    char t[12] = "SCENE ";
    int ok = project_save() == 0;
    if (ok)
        ui.dirty = 0;
    if (n < 0) {
        say(ok ? "SAVED" : "NOT SAVED", ok ? 0 : "FLASH ERROR");
    } else {
        itoa_u((uint32_t)n + 1u, t + 6);
        say(t, ok ? "STORED" : "NOT STORED");
    }
}

/* a save: while the drone sounds, the output dips out first (the write stops the audio for a few ms, measured
 * on the FM-1: under one 5.8 ms block), then comes back; Evan, 2026-10-14: no more waiting for PLAY to stop */
static void save_ask(int n)
{
    if (!ui.playing && dr_fade <= 0.0f) {
        save_now(n);
        return;
    }
    if (ui.save_q != -2)                               /* one waiting already: it covers this one too */
        n = ui.save_q >= 0 ? ui.save_q : n;
    ui.save_q = (int8_t)n;
    ui.save_t = plat_ms();
    dr_duck(1);
}

static void save_run(void)                             /* each look at the input: the dip out, the write */
{
    if (ui.save_q == -2 || (dr_duck_level > 0.0f && plat_ms() - ui.save_t < 100u))
        return;
    save_now(ui.save_q);
    ui.save_q = -2;
    dr_duck(0);
}

static void scene_store(int n)                         /* SAVE held + a white key: and into the flash */
{
    int k;
    for (k = 0; k < DR_NPARAMS; k++)
        proj.scene[n][k] = proj.par[k];
    proj.scene_used |= (uint16_t)(1u << n);
    ui.scene_at = (int8_t)n;
    ui.dirty = 1;
    save_ask(n);
}

static float fade_s(void) { return 0.05f * fm_exp2f((float)proj.par[P_FADE] * 0.102288f); }
static float scene_s(void) { return 0.1f * fm_exp2f((float)proj.par[P_SCENE_T] * 0.115507f); }

static void scene_recall(int n)                        /* a white key: morph there over the Fade time */
{
    char t[12] = "SCENE ";
    int k;
    itoa_u((uint32_t)n + 1u, t + 6);
    if (!(proj.scene_used >> n & 1u)) {
        say(t, "EMPTY");
        return;
    }
    for (k = 0; k < DR_NPARAMS; k++)
        ui.morph_from[k] = proj.par[k];
    ui.morph = (int8_t)n;
    ui.scene_at = (int8_t)n;
    ui.morph_t0 = plat_ms();
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f);
    say(t, 0);
}

/* a visit let go: morph back to the sound before it, over the Scene switch time */
#define VISIT_BACK NSCENES
static void scene_back(void)
{
    int k;
    for (k = 0; k < DR_NPARAMS; k++)
        ui.morph_from[k] = proj.par[k];
    ui.morph = VISIT_BACK;
    ui.scene_at = ui.visit_from;
    ui.morph_t0 = plat_ms();
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f);
    say("BACK", 0);
}

/* the morph, each frame: smooth values glide, steps flip half way; the Fade itself isn't recalled */
static void scene_morph(void)
{
    const int16_t *to;
    float x;
    int k;
    if (ui.morph < 0)
        return;
    to = ui.morph == VISIT_BACK ? ui.visit_back : proj.scene[ui.morph];
    x = ui.morph_ms ? (float)(plat_ms() - ui.morph_t0) / (float)ui.morph_ms : 1.0f;
    if (x > 1.0f)
        x = 1.0f;
    for (k = 0; k < DR_NPARAMS; k++) {
        int v;
        if (scene_room(k))                             /* the room's settings aren't a scene's */
            continue;
        if (scene_step(k))
            v = x >= 0.5f ? to[k] : ui.morph_from[k];
        else
            v = ui.morph_from[k] + (int)((float)(to[k] - ui.morph_from[k]) * x + (to[k] > ui.morph_from[k] ? 0.5f : -0.5f));
        if (v == proj.par[k])
            continue;
        if (k == DR_GP(0, GP_PITCH) || k == DR_GP(1, GP_PITCH))
            dr_slide(k / GP_N, 20);                    /* a frame's step of pitch, slid */
        else if (k == P_PLINK_OFF)
            dr_slide(1, 20);
        param_set(k, v);
    }
    if (x >= 1.0f)
        ui.morph = -1;
}

static void button(int b)                              /* the other buttons, on release */
{
    switch (b) {
    case B_SAVE:
        save_ask(-1);
        break;
    case B_REC:
        loop_button();
        break;
    default: break;                                    /* SEL: a modifier (fine pitch) */
    }
}

/* MIDI in (USB and the TRS jack): a note on channel 1 tunes drone I to it, on channel 2 drone II */
static void midi_in(void)
{
    uint32_t pkt;
    while (plat_midi_in(&pkt)) {
        uint32_t st = (pkt >> 8) & 0xFFu, n = (pkt >> 16) & 0x7Fu, v = pkt >> 24;
        if ((st == 0x90u || st == 0x91u) && v) {
            int g = (int)(st & 1u), c = (int)n * 100;
            if (g == 1 && proj.par[P_PLINK])
                param_set(P_PLINK_OFF, c - proj.par[DR_GP(0, GP_PITCH)]);
            else
                param_set(DR_GP(g, GP_PITCH), c);
        }
    }
}

static void global_turn(int role, int p, const char *name, int32_t e)
{
    char t[12];
    const dr_param_t *in = dr_param_info(p);
    ui.scene_at = -1;                          /* a hand on the sound */
    if (in->hi - in->lo > 24)
        cont_turn(p, role, e);                 /* the LFO rate glides as a knob on the page does */
    else {
        param_set(p, proj.par[p] + accel(role, e, in->hi - in->lo));
        loop_note(p, proj.par[p], 0);
    }
    dr_param_text(p, proj.par[p], t);
    {   /* not on the page: a label in the top bar */
        uint32_t i;
        for (i = 0; i < 15u && name[i]; i++)
            ui.glab[0][i] = name[i];
        ui.glab[0][i] = 0;
        for (i = 0; i < 15u && t[i]; i++)
            ui.glab[1][i] = t[i];
        ui.glab[1][i] = 0;
        ui.glab_until = plat_ms() + 1500u;
    }
    ui.act_t = plat_ms();
}

static void input(void)
{
    uint32_t btn = plat_buttons(), keys = plat_keys(), ch, i;
    int32_t e;
    save_run();
    ch = btn ^ ui.btn;
    ui.btn = btn;
    for (i = 0; i < NB; i++) {
        uint32_t m = 1u << i;
        if (!(ch & m))
            continue;
        if (page_btn((int)i) >= 0) {                   /* page buttons: on the press and the release */
            if (btn & m)
                page_down((int)i);
            else
                page_up((int)i);
        } else if (btn & m) {
            ui.btn_used &= ~m;
            if (i == B_OCTDN || i == B_OCTUP) {        /* the MOVEMENT, on the press (and held) */
                ui.btn_used |= m;
                movement_step(i == B_OCTUP ? 1 : -1);
            }
            if (i == B_PLAY && (btn >> B_SEL & 1u)) {  /* SEL + PLAY: drone II alone, out / in */
                ui.btn_used |= 1u << B_SEL;
                ui.v2_out = (uint8_t)!ui.v2_out;
                dr_voice(1, !ui.v2_out);
                say_c("DRONE II", ui.v2_out ? "OUT" : "IN", K_DRONE[1]);
            } else if (i == B_PLAY) {                  /* PLAY on the press */
                static uint8_t seeded;
                if (!seeded) {                         /* the random ones from when the hand first pressed
                                                        * PLAY: a new wander each time it's switched on */
                    seeded = 1;
                    dr_seed(plat_ms());
                }
                ui.playing = (uint8_t)!ui.playing;
                dr_play(ui.playing);
            }
        } else if (!(ui.btn_used & m) && i != B_PLAY) {
            button((int)i);
        }
    }
    if ((btn & (1u << B_OCTDN | 1u << B_OCTUP)) && (int32_t)(plat_ms() - ui.oct_next) >= 0) {
        movement_step(btn >> B_OCTUP & 1u ? 1 : -1);   /* held: on and on */
        ui.oct_next = plat_ms() + 120u;
    }
    ch = keys ^ ui.keys;                               /* the black keys: switches, on the press */
    ui.keys = keys;
    for (i = 0; i < 11u; i++) {
        sw_t w = sw_layer()[i];
        if ((ch & keys) >> BLACK_K[i] & 1u) {
            sw_toggle(w);                              /* LFO reset: at once, on the press */
            if (w.s != S_LFORESET)
                ui.scene_at = -1;
            if (w.s == S_LFORESET) {
                ui.rst_t = plat_ms();
                ui.rst_done = 0;
            }
        } else if (w.s == S_LFORESET && (keys >> BLACK_K[i] & 1u) && !ui.rst_done &&
                   plat_ms() - ui.rst_t >= HOLD_MS + 100u) {
            int on = !proj.par[P_LFO_LOCK];            /* held: the LFO locked to the pulse's bars, or free */
            param_set(P_LFO_LOCK, on);
            ui.scene_at = -1;
            say_c("LFO TO SWELL", on ? (proj.par[P_PULSE] ? "LOCKED" : "LOCKED (SWELL OFF)") : "FREE", K_LFO);
            ui.rst_done = 1;
        }
    }
    for (i = 0; i < NSCENES; i++) {                    /* the white keys: scenes (SAVE held: store) */
        if ((ch & keys) >> WHITE_K[i] & 1u) {
            if (btn >> B_SAVE & 1u) {
                ui.btn_used |= 1u << B_SAVE;           /* SAVE let go: not a plain save */
                scene_store((int)i);
            } else {
                int k;
                for (k = 0; k < DR_NPARAMS; k++)       /* where a visit comes back to */
                    ui.visit_back[k] = proj.par[k];
                ui.visit_from = ui.scene_at;
                scene_recall((int)i);
                ui.visit = ui.morph == (int8_t)i ? (int8_t)i : -1;
                ui.visit_t = plat_ms();
            }
        } else if ((ch & ~keys) >> WHITE_K[i] & 1u && ui.visit == (int8_t)i) {
            ui.visit = -1;                             /* let go: a tap stays; a hold was a visit: */
            if (plat_ms() - ui.visit_t >= HOLD_MS)     /* back to the sound before it, as it came */
                scene_back();
        }
    }
    if (btn || keys || ch)
        ui.act_t = plat_ms();
    loop_run();
    midi_in();
    if ((e = plat_enc(EN_SELECT)) != 0)
        global_turn(EN_SELECT, P_LFO_RATE, "LFO RATE", e);
    if ((e = plat_enc(EN_PRESET)) != 0)              /* PRESETS (left): Wave I, ALGORITHM (right): Wave II */
        global_turn(EN_PRESET, P_LFO_W1, "WAVE I", e);
    if ((e = plat_enc(EN_ALGO)) != 0)
        global_turn(EN_ALGO, P_LFO_W2, "WAVE II", e);
    for (i = 0; i < 4; i++)
        if ((e = plat_enc(EN_K1 + (int)i)) != 0) {
            knob_turn((int)i, e);
            ui.scene_at = -1;                          /* the sound is no longer the scene's */
            ui.morph = -1;                             /* a hand on a knob: the morph stops where it is */
            ui.visit = -1;                             /* ... and a visit held becomes a stay (no way back) */
            if (ui.hold_btn >= 0)
                ui.hold_turned = 1;                    /* a peek: let go of the button, back to the page */
            ui.touch_t[i] = plat_ms() + 1200u;
            ui.act_t = plat_ms();
        }
}

/* ------------------------------------------------------------- screen --- */
/* Concept D (Figma "Screen concepts"): a thin top bar (the page, badges, a label for SELECT / ALGORITHM /
 * PRESETS, the LFO's lights); the picture in the top two-thirds, drone I on the left over KNOB 1-2, drone
 * II on the right over KNOB 3-4; the page's four values in the bottom third, each lighting while its
 * knob moves (its own timer: two hands, two lit). A switch shows as a banner across the top of the
 * picture. Nothing stays on screen that isn't the page's or moving. */
#define BAR_H 16
#define PIC_Y 16
#define PIC_H 139
#define VAL_Y 155
#define VAL_H 85
#define SCOPE_EVERY 3u                       /* the picture: every 3rd frame (~20 a second; the LCD is slow) */

static uint32_t hash(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

/* a rectangle with rounded corners (radius r, up to 6) */
static void cv_round(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t c)
{
    static const uint8_t IN[7][6] = {{0}, {1}, {1, 0}, {2, 1, 0}, {2, 1, 0, 0}, {3, 2, 1, 0, 0}, {4, 2, 1, 1, 0, 0}};
    int32_t j;
    if (r > 6)
        r = 6;
    if (r > h / 2)
        r = h / 2;
    cv_rect(x, y + r, w, h - 2 * r, c);
    for (j = 0; j < r; j++) {
        int32_t d = IN[r][j];
        cv_rect(x + d, y + j, w - 2 * d, 1, c);
        cv_rect(x + d, y + h - 1 - j, w - 2 * d, 1, c);
    }
}
static void cv_frame(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a 1 px outline */
{
    cv_rect(x + 2, y, w - 4, 1, c);
    cv_rect(x + 2, y + h - 1, w - 4, 1, c);
    cv_rect(x, y + 2, 1, h - 4, c);
    cv_rect(x + w - 1, y + 2, 1, h - 4, c);
    cv_rect(x + 1, y + 1, 1, 1, c);
    cv_rect(x + w - 2, y + 1, 1, 1, c);
    cv_rect(x + 1, y + h - 2, 1, 1, c);
    cv_rect(x + w - 2, y + h - 2, 1, 1, c);
}

static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

/* centred in w pixels: a word too wide for them has its letters a pixel closer */
static void text_fit(int32_t cx, int32_t y, int32_t w, const felucca_font_t *f, const char *s, uint16_t c)
{
    if (text_w(f, s) > w)
        cv_track = -1;
    text_c(cx, y, f, s, c);
    cv_track = 0;
}

/* the largest face a value fits in w pixels in (the large one only while lit: it has no lower case) */
static const felucca_font_t *fit_face(const char *s, int32_t w, int large)
{
    static const felucca_font_t *const F[3] = {&FONT_B, &FONT_S, &FONT_XS};
    const char *q = s;
    int i, lower = 0;
    for (; *q; q++)
        lower |= *q >= 'a' && *q <= 'z';
    if (large && !lower && text_w(&FONT_M, s) <= w)
        return &FONT_M;
    for (i = 0; i < 2; i++)
        if (text_w(F[i], s) <= w)
            return F[i];
    return F[2];
}

/* a knob's name on its card; the few too long for one, shorter there (the manual has the full ones) */
static const char *card_name(int p)
{
    const char *n = p == P_PLINK_OFF ? "Link ofs" : dr_param_info(p)->name;
    if (n[0] == 'S' && n[1] == 'c' && n[2] == 'e')
        return "Scenes";                       /* Scene switch */
    if (n[0] == 'X' && n[1] == '-' && n[2] == 'm')
        return "X-mod";                        /* X-mod amt */
    return n;
}

static int fade_q(void) { return (int)(dr_fade * 20.0f + 0.5f); }   /* the fade in 20 steps */

static int lit(int i) { return plat_ms() < ui.touch_t[i]; }          /* knob i moving (or just moved) */

/* a tap's light: dark at -1, bright at 1, in 8 steps; unlit when the tap is off */
static int lfo_q(int i) { return proj.par[i ? P_LFO_W2 : P_LFO_W1] ? (int)((dr_lfo[i] + 1.0f) * 3.5f + 0.5f) : -1; }
static uint16_t blend(uint16_t a, uint16_t b, int q, int n)          /* a -> b, q of n */
{
    int r = ((a >> 11) * (n - q) + (b >> 11) * q) / n, g = (((a >> 5) & 63) * (n - q) + ((b >> 5) & 63) * q) / n,
        bl = ((a & 31) * (n - q) + (b & 31) * q) / n;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

/* the page's colour (its name; global values' accents) */
static uint16_t page_col(void)
{
    switch (ui.view) {
    case V_LFO: case V_LFO2: return K_LFO;
    case V_FX: case V_FX2: return K_FX;
    case V_PULSE: return K_PULSE;
    case V_SETUP: case V_SETUP2: return K_SETUP;
    default: return K_DIM;
    }
}

/* the top bar: the page's name, the loop's and pulse's badges, PLAY's hint while stopped (or the label of
 * SELECT / ALGORITHM / PRESETS while one turns), the LFO's two lights */
static void draw_bar(void)
{
    int gl = plat_ms() < ui.glab_until;
    uint32_t rec_s = ui.loop == L_REC ? (plat_ms() - ui.loop_t0) / 1000u : 0u;   /* REC: how long so far */
    uint32_t h = hash(hash(hash(hash(2166136261u ^ rec_s, ui.view | ui.playing << 5 | ui.dirty << 6 | ui.loop << 7 |
                                                       (uint32_t)proj.par[P_PULSE] << 9 | (uint32_t)gl << 10 |
                                                       (uint32_t)ui.v2_out << 12),
                                (uint32_t)fade_q()),
                           (uint32_t)(lfo_q(0) + 1) | (uint32_t)(lfo_q(1) + 1) << 4),
                      gl ? (uint32_t)ui.glab[1][0] << 16 | (uint32_t)ui.glab[1][1] << 8 | ui.glab[1][2] |
                               (uint32_t)ui.glab[1][3] << 24 | (uint32_t)ui.glab[0][0] << 4
                         : 0);
    int32_t x, i, rx = 216;
    int visiting = ui.visit >= 0 && plat_ms() - ui.visit_t >= HOLD_MS;          /* a scene key held: a visit */
    h = hash(h, (uint32_t)(visiting ? ui.visit + 1 : 0));
    if (h == ui.sig[0])
        return;
    ui.sig[0] = h;
    cv_begin(240, BAR_H, K_BG);
    {   /* the page's name, in capitals */
        char t[20];
        const char *n = VIEW_NAME[ui.view];
        for (i = 0; n[i] && i < 19; i++)
            t[i] = (char)(n[i] >= 'a' && n[i] <= 'z' ? n[i] - 32 : n[i]);
        t[i] = 0;
        x = cv_text(6, 0, &FONT_B, t, page_col() == K_DIM ? K_TEXT2 : page_col());
    }
    for (i = 0; i < (int32_t)NPAGE_BTN; i++)   /* a button's pages: a dot each, this one lit ("tap again") */
        if (on_pages((int)i) && PAGE_BTN[i].n > 1) {
            int32_t k;
            for (k = 0; k < PAGE_BTN[i].n; k++)
                cv_round(x + 5 + 8 * k, 6, 5, 5, 2,
                         ui.view == PAGE_BTN[i].first + k ? (page_col() == K_DIM ? K_TEXT2 : page_col()) : K_FAINT);
            x += 5 + 8 * PAGE_BTN[i].n;
        }
    if (ui.dirty)
        cv_round(x + 4, 6, 5, 5, 2, K_GOLD);   /* not yet written: amber (grey, it read as a page dot) */
    x += 12;
    if (ui.loop != L_IDLE) {                  /* REC and its time ("REC 1:05"; it stops at 5:00), or LOOP */
        char t[12] = "REC ", *q = t + 4;
        if (ui.loop == L_REC) {
            itoa_u(rec_s / 60u, q);
            while (*q)
                q++;
            *q++ = ':';
            *q++ = (char)('0' + rec_s % 60u / 10u);
            *q++ = (char)('0' + rec_s % 10u);
            *q = 0;
        }
        x = cv_text(x, 2, &FONT_XS, ui.loop == L_REC ? t : "LOOP", K_DRONE[0]) + 6;
    }
    if (proj.par[P_PULSE] && ui.view != V_PULSE)  /* on its own page, its name says it */
        x = cv_text(x, 2, &FONT_XS, "SWELL", K_PULSE) + 6;
    if (visiting) {                            /* "VISIT 3": let go and it goes back */
        char t[10] = "VISIT ";
        itoa_u((uint32_t)ui.visit + 1u, t + 6);
        x = cv_text(x, 2, &FONT_XS, t, K_GOLD) + 6;
    }
    if (ui.v2_out)                             /* drone II faded out on its own (SEL + PLAY) */
        cv_text(x, 2, &FONT_XS, "II OUT", K_DRONE[1]);
    for (i = 0; i < 2; i++) {                  /* the LFO's rate lights, as the DS-2's: Wave I, Wave II */
        int q = lfo_q((int)i);
        cv_round(222 + 10 * i, 5, 7, 7, 3, q < 0 ? K_GRID : blend(K_GRID, K_LFO, q, 7));
    }
    if (gl) {                                  /* SELECT / ALGORITHM / PRESETS: a pill by the lights */
        char t[34];
        int32_t w;
        char *q = scopy(t, ui.glab[0]);
        *q++ = ' ';
        *q++ = ' ';
        scopy(q, ui.glab[1]);
        w = text_w(&FONT_XS, t) + 12;
        cv_round(rx - w, 1, w, BAR_H - 2, 6, K_CARD);
        cv_frame(rx - w, 1, w, BAR_H - 2, K_LFO);
        cv_text(rx - w + 6, 2, &FONT_XS, t, K_LFO);
    } else if (!ui.playing) {                  /* stopped: say what PLAY does (the panel has no label) */
        const char *st = fade_q() > 0 ? "FADING OUT" : "PLAY to start";
        cv_text(rx - text_w(&FONT_XS, st), 2, &FONT_XS, st, K_DIM);
    } else if (fade_q() < 20) {
        cv_text(rx - text_w(&FONT_XS, "FADING IN"), 2, &FONT_XS, "FADING IN", K_DIM);
    }
    cv_blit(0, 0);
}

/* a drone's waveform in its panel: its scope, glowing (a dim line around a bright one); its height is
 * its level (it fades with PLAY and its level) */
static void draw_scope(int g, int32_t x0, int32_t y0, int32_t w, int32_t h)
{
    int32_t i, mid = y0 + h / 2, py = mid, half = h / 2 - 4;
    const float k = (float)half / 0.6f;       /* a drone at full level (with sub and drive) peaks near 0.6 */
    cv_round(x0, y0, w, h, 4, K_PANEL);
    for (i = 0; i < DR_SCOPE_N && i < w - 8; i++) {
        int32_t yy = mid - (int32_t)fm_clampf(dr_scope[g][i] * k, (float)-half, (float)half), x = x0 + 4 + i;
        if (i) {
            cv_line(x - 1, py - 1, x, yy - 1, K_DRONE_DIM[g]);
            cv_line(x - 1, py + 1, x, yy + 1, K_DRONE_DIM[g]);
            cv_line(x - 1, py, x, yy, K_DRONE[g]);
        }
        py = yy;
    }
    cv_text(x0 + 5, y0 + 3, &FONT_S, g ? "II" : "I", K_DRONE[g]);
}

/* ---------------------------------------------------- the page pictures --- */
/* each page draws what its knobs do, in the picture: a drone's page in two panels (drone I | drone II),
 * as on Play; a page of both drones' settings across one panel. Drawn ~20 times a second like the
 * waveforms, so whatever moves (a meter, a beat, the cross-mod) moves as it sounds. */
#define PAN_W 112
#define PAN_H (PIC_H - 10)
static int32_t pan_x(int g) { return g ? 122 : 6; }
static int gpar(int g, int p) { return proj.par[DR_GP(g, p)]; }
static uint16_t dim_of(uint16_t c) { return blend(K_PANEL, c, 1, 3); }

/* a glowing polyline through n points (x0 + i * dx, y[i]): all the glow first, then the line, so the
 * glow never covers the line */
static void plot(int32_t x0, int32_t dx, const int16_t *y, int n, uint16_t c)
{
    uint16_t d = dim_of(c);
    int i;
    for (i = 1; i < n; i++) {
        cv_line(x0 + (i - 1) * dx, y[i - 1] - 1, x0 + i * dx, y[i] - 1, d);
        cv_line(x0 + (i - 1) * dx, y[i - 1] + 1, x0 + i * dx, y[i] + 1, d);
    }
    for (i = 1; i < n; i++)
        cv_line(x0 + (i - 1) * dx, y[i - 1], x0 + i * dx, y[i], c);
}

/* a number with one decimal: "0.6", "12.5" */
static char *dec1(float v, char *b)
{
    int t = (int)(v * 10.0f + 0.5f);
    itoa_u((uint32_t)(t / 10), b);
    while (*b)
        b++;
    *b++ = '.';
    *b++ = (char)('0' + t % 10);
    *b = 0;
    return b;
}

/* the X-WAVE's four waves at phase ph (0..1), as the engine's: a falling saw, a square of pulse width pw */
static float wave_at(int w, float pw, float ph)
{
    switch (w) {
    case DR_SINE: return fm_sinf(6.2831853f * ph);
    case DR_TRI: return ph < 0.25f ? 4.0f * ph : ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f;
    case DR_SAW: return 1.0f - 2.0f * ph;
    default: return ph < pw ? 1.0f : -1.0f;
    }
}
/* the X-WAVE at x (0..100): a wave, or two blended (within 2 of a wave: that wave, as the engine) */
static float xwave_at(int x, float pw, float ph)
{
    int s = x * 3, wi = (s + 2) / 100;
    float wf = (float)(s - 100 * wi) * 0.01f, a;
    if (wi >= 3 || wf <= 0.02f)
        return wave_at(wi > 3 ? 3 : wi, pw, ph);
    a = wave_at(wi, pw, ph);
    return a + (wave_at(wi + 1, pw, ph) - a) * wf;
}
static float pw_of(int g) { return 0.5f - 0.45f * (float)gpar(g, GP_WIDTH) * 0.01f; }

/* a wave across a box: cycles of the X-WAVE x, starting at phase ph0; amp of the half-height */
static void draw_xwave(int g, int x, int32_t x0, int32_t yc, int32_t w, int32_t half, float cycles, float ph0,
                       uint16_t c)
{
    int16_t y[PAN_W];
    int i, n = (int)(w / 2) + 1;
    float pw = pw_of(g);
    if (n > PAN_W)
        n = PAN_W;
    for (i = 0; i < n; i++) {
        float ph = ph0 + cycles * (float)i / (float)(n - 1);
        ph -= (float)(int)ph;
        y[i] = (int16_t)(yc - (int32_t)(xwave_at(x, pw, ph) * (float)half));
    }
    plot(x0, 2, y, n, c);
}

/* a drone's filter, as the engine's (12 dB state-variable): its response in dB at f, on the cutoff knob's
 * own scale (0..100 = 20 Hz .. 18 kHz); cut, the cutoff there (moved, for the cross-mod and the LFO) */
static float filt_db(int g, float cut, float f)
{
    int mode = gpar(g, GP_MODE);
    float r = (float)gpar(g, GP_RESO) * 0.001f, k = fm_maxf(2.0f - 2.1f * r, 0.06f), rin = 1.0f - 0.6f * r * r;
    float rr = fm_exp2f((f - cut) * 0.0981378f), r2 = rr * rr, d = (1.0f - r2) * (1.0f - r2) + k * k * r2, m2;
    if (mode == DR_LP) {                       /* as the engine: rin of the filter + (1 - rin) of a one-pole an
                                                * octave under the cutoff (the bass makeup), summed as complex */
        float re = rin * (1.0f - r2) / d, im = -rin * k * rr / d, q = 1.0f + 4.0f * r2;
        re += (1.0f - rin) / q;
        im -= (1.0f - rin) * 2.0f * rr / q;
        m2 = re * re + im * im;
    } else {
        m2 = (mode == DR_BP ? r2 : r2 * r2) * rin * rin / d;
    }
    return 3.0103f * fm_log2f(fm_maxf(m2, 1e-9f));
}

/* ... its curve across 20 Hz .. 18 kHz, +18 dB at the top, -36 at the bottom */
static void draw_filter(int g, float cut, int32_t x0, int32_t y0, int32_t w, int32_t h, uint16_t c)
{
    int16_t y[PAN_W];
    int i, n = (int)(w / 2) + 1;
    int32_t z = y0 + h * 18 / 54, cx;
    if (n > PAN_W)
        n = PAN_W;
    cv_rect(x0, z, w, 1, K_GRID);                                     /* 0 dB */
    cx = x0 + (int32_t)(fm_clampf(cut, 0.0f, 100.0f) * (float)w * 0.01f);
    cv_rect(cx, y0, 1, h, K_GRID);                                    /* the cutoff */
    for (i = 0; i < n; i++) {
        float db = filt_db(g, cut, 100.0f * (float)i / (float)(n - 1));
        y[i] = (int16_t)(z - (int32_t)(fm_clampf(db, -36.0f, 18.0f) * (float)h / 54.0f));
    }
    plot(x0, 2, y, n, c);
}

/* Play 2: each drone's resonance (its filter's curve, the peak growing) and its level (a live meter) */
static void pic_play2(int g, int32_t x0, int32_t y0)
{
    int32_t mw = PAN_W - 16, fw;
    float l = dr_level[g], db = l > 1e-4f ? 6.0206f * fm_log2f(l) : -99.0f;
    char t[12];
    draw_filter(g, dr_cut[g], x0 + 4, y0 + 22, PAN_W - 8, 52, K_DRONE[g]);
    cv_round(x0 + 8, y0 + 88, mw, 8, 3, K_GRID);                       /* the level, -48 .. 0 dBFS */
    fw = (int32_t)(fm_clampf((db + 48.0f) / 48.0f, 0.0f, 1.0f) * (float)mw);
    if (fw > 3)
        cv_round(x0 + 8, y0 + 88, fw, 8, 3, K_DRONE[g]);
    if (db > -60.0f) {
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
    text_c(x0 + PAN_W / 2, y0 + 101, &FONT_S, t, K_TEXT2);
}




/* Tone 2: each drone's noise as it leaves the filter: its colour's slope (0 dB/oct white, -3 pink, level
 * at 100 Hz) through the filter's curve (dim behind it, moving with the LFO as Play 2's), as low as its level
 * is; its colour's name, and its level against the oscillators */
static void pic_tone2(int g, int32_t x0, int32_t y0)
{
    int16_t y[PAN_W];
    int i, nv = gpar(g, GP_NOISE), cv = gpar(g, GP_NCOLOR), n = (PAN_W - 8) / 2 + 1;
    int32_t gx = x0 + 4, gy = y0 + 22, gh = 52, z = gy + gh * 18 / 54;
    float slope = cv ? -3.0f : 0.0f, share = 20.0f * 0.30103f * fm_log2f(fm_maxf((float)nv * 0.005f, 1e-3f));
    char t[16], *q;
    draw_filter(g, dr_cut[g], gx, gy, PAN_W - 8, gh, K_DRONE_DIM[g]);
    if (nv > 0) {
        for (i = 0; i < n; i++) {
            float f = 100.0f * (float)i / (float)(n - 1);
            float db = share + slope * (f * 0.0981378f - 2.3219281f) + filt_db(g, dr_cut[g], f);   /* 100 Hz: 0 */
            y[i] = (int16_t)(z - (int32_t)(fm_clampf(db, -36.0f, 18.0f) * (float)gh / 54.0f));
        }
        plot(gx, 2, y, n, K_DRONE[g]);
    }
    scopy(t, cv ? "pink" : "white");
    cv_text(x0 + PAN_W - 6 - text_w(&FONT_S, t), y0 + 3, &FONT_S, t, nv ? K_DRONE[g] : K_DIM);
    if (!nv)                                   /* its level against the oscillators: "noise -12 dB" */
        scopy(t, "no noise");
    else {
        int db = (int)(-share + 0.5f);
        q = scopy(t, db ? "noise -" : "noise 0 dB");
        if (db) {
            itoa_u((uint32_t)db, q);
            while (*q)
                q++;
            scopy(q, " dB");
        }
    }
    text_c(x0 + PAN_W / 2, y0 + PAN_H - 18, &FONT_S, t, nv ? K_TEXT2 : K_DIM);
}

static void pic_osc(int g, int32_t x0, int32_t y0)
{
    static const char *const W[4] = {"Sin", "Tri", "Saw", "Sqr"};
    int x = gpar(g, GP_WAVE), k, sq = x * 3 > 200;
    int32_t tx0 = x0 + 12, tw = PAN_W - 24;
    char t[16], *q;
    draw_xwave(g, x, x0 + 6, y0 + 42, PAN_W - 12, 22, 1.0f, 0.0f, K_DRONE[g]);
    for (k = 0; k < 4; k++)                    /* where the X-WAVE sits: sine .. square */
        text_c(tx0 + tw * k / 3, y0 + 70, &FONT_XS, W[k], K_DIM);
    cv_rect(tx0, y0 + 88, tw, 2, K_GRID);
    cv_round(tx0 + tw * x / 100 - 3, y0 + 86, 7, 7, 3, K_DRONE[g]);
    /* the pulse width (KNOB 2 / 4): how high the square is, or that there's no square to shape */
    if (sq) {
        q = scopy(t, "PW ");
        itoa_u((uint32_t)(int)(pw_of(g) * 100.0f + 0.5f), q);
        while (*q)
            q++;
        scopy(q, "% high");
    } else {
        scopy(t, "PW: no square");
    }
    text_c(x0 + PAN_W / 2, y0 + 100, &FONT_S, t, sq ? K_TEXT2 : K_DIM);
}


static void pic_partner(int g, int32_t x0, int32_t y0)
{
    int on = gpar(g, GP_PARTNER) != 0, iv = gpar(g, GP_INTERVAL), x = gpar(g, GP_WAVE);
    float semi = (float)DR_INTERVALS[(unsigned)iv < DR_NINTERVALS ? iv : 3];
    float hz = 8.1757989f * fm_exp2f((dr_pitch[g] + semi) / 12.0f);   /* the partner's pitch */
    float beats = hz * (fm_exp2f((float)gpar(g, GP_DETUNE) * 0.003f / 12.0f) - 1.0f), ph;
    char t[24], *q;
    /* the main oscillator, and under it the partner: its cycles at the interval, sliding against the
     * main one at the beating rate (shown at most 2 a second) */
    draw_xwave(g, x, x0 + 6, y0 + 36, PAN_W - 12, 14, 2.0f, 0.0f, K_DRONE[g]);
    ph = (float)(plat_ms() % 100000u) * 0.001f * fm_minf(beats, 2.0f);
    if (on)
        draw_xwave(g, x, x0 + 6, y0 + 76, PAN_W - 12, 14, 2.0f * fm_exp2f(semi / 12.0f), ph - (float)(int)ph,
                   blend(K_PANEL, K_DRONE[g], 2, 3));
    dr_param_text(DR_GP(g, GP_INTERVAL), iv, t);
    cv_text(x0 + PAN_W - 6 - text_w(&FONT_S, t), y0 + 3, &FONT_S, t, on ? K_DRONE[g] : K_DIM);
    if (!on)
        scopy(t, "partner off");
    else if (beats < 0.05f)
        scopy(t, "no beating");
    else {
        q = scopy(t, "beats ");
        q = dec1(fm_minf(beats, 99.0f), q);
        scopy(q, "/s");
    }
    text_c(x0 + PAN_W / 2, y0 + PAN_H - 18, &FONT_S, t, K_DIM);
}

/* --- the pages of both drones' settings: one panel across --- */
#define WIDE_W 228

static void text_r(int32_t xr, int32_t y, const char *s, uint16_t c) { cv_text(xr - text_w(&FONT_S, s), y, &FONT_S, s, c); }

/* Pulse: the gate over its two beats, twice (as set: depth, shape, swing), where it is now, ON / OFF */
static void pic_pulse(int32_t y0)
{
    int on = proj.par[P_PULSE] != 0, i, n = 105;
    int16_t y[105];
    int32_t top = y0 + 26, h = PAN_H - 56;
    uint16_t c = on ? K_PULSE : K_FAINT;
    char t[16];
    for (i = 0; i < n; i++)
        y[i] = (int16_t)(top + h - (int32_t)(dr_pulse_wave(4.0f * (float)i / (float)(n - 1)) * (float)h));
    cv_rect(12, top + h, 208, 1, K_GRID);
    for (i = 1; i < 4; i++)                    /* the beats */
        cv_rect(12 + 52 * i, top, 1, h, K_GRID);
    plot(12, 2, y, n, c);
    if (on) {                                  /* now: a dot on the line, across both bars (it went over */
        static float last;                     /* the first only, then jumped back) */
        static uint8_t odd;
        float ph = dr_pulse_ph;
        int32_t px, py;
        if (ph < last - 1.0f)                  /* a new bar: the other half of the picture */
            odd ^= 1u;
        last = ph;
        px = 12 + (int32_t)((ph + 2.0f * (float)odd) * 52.0f);
        py = top + h - (int32_t)(dr_pulse_wave(ph) * (float)h);
        cv_round(px - 3, py - 3, 7, 7, 3, K_TEXT);
    }
    dr_param_text(P_PULSE_RATE, proj.par[P_PULSE_RATE], t);
    cv_text(12, y0 + 3, &FONT_S, t, c);
    text_r(222, y0 + 3, on ? "ON" : "OFF", on ? K_PULSE : K_DIM);
    text_c(120, y0 + PAN_H - 18, &FONT_S,
           !on ? "tap ARP to start the swell" : proj.par[P_LFO_LOCK] ? "the LFO restarts each bar" : "the swell, over two bars",
           proj.par[P_LFO_LOCK] && on ? K_LFO : K_DIM);
}

/* LFO: the routing as a picture. Each wave on the left (I above, II below: one cycle of its shape, a dot
 * where the LFO is in it now), the five destinations on the right (drone I's oscillator and filter, drone
 * II's, the delay's time), and a line from each wave to what it moves: bright when its depth moves it, dim
 * when routed at depth 0; a route on both waves has two lines. The rate at the bottom */
#define LFO_WX0 12                             /* the waves: x 12 .. 100 */
#define LFO_WW 88
static void pic_lfo(int32_t y0)
{
    static const int32_t WY[2] = {38, 92};     /* the waves' middles (from y0) */
    int tap, d;
    char t[24];
    for (tap = 0; tap < 2; tap++) {
        int sh = proj.par[tap ? P_LFO_W2 : P_LFO_W1], i, n = LFO_WW / 2 + 1;
        int32_t yc = y0 + WY[tap], half = 12;
        int16_t y[LFO_WW / 2 + 1];
        uint16_t c = sh ? K_LFO : K_FAINT;
        /* one cycle across (random: this cycle, its phases 1..2), and a dot where the LFO is in it now */
        float r0 = sh == DR_LFO_RANDOM ? 1.0f : 0.0f;
        for (i = 0; i < n; i++)
            y[i] = (int16_t)(yc - (int32_t)(dr_lfo_wave(tap, sh, r0 + 0.9999f * (float)i / (float)(n - 1)) * (float)half));
        plot(LFO_WX0, 2, y, n, c);
        if (sh) {
            float ph = dr_lfo_ph;
            int32_t px = LFO_WX0 + (int32_t)(ph * (float)LFO_WW);
            int32_t py = yc - (int32_t)(dr_lfo_wave(tap, sh, r0 + ph) * (float)half);
            cv_round(px - 3, py - 3, 7, 7, 3, K_TEXT);
        }
        cv_text(LFO_WX0, yc - 32, &FONT_S, tap ? "II" : "I", c);
        dr_param_text(tap ? P_LFO_W2 : P_LFO_W1, sh, t);
        cv_text(LFO_WX0 + 16, yc - 32, &FONT_S, t, sh ? K_TEXT2 : K_DIM);
    }
    /* the destinations, and the lines to them */
    for (d = 0; d < 5; d++) {
        int g = d / 2, route, depth, w;
        uint16_t col;
        int32_t ny = y0 + 6 + d * 24, ay = ny + 8, ax;
        if (d < 4) {
            int osc = d % 2 == 0;
            route = gpar(g, osc ? GP_OLFO : GP_FLFO);
            depth = gpar(g, osc ? GP_OLFO_DEPTH : GP_FLFO_DEPTH);
            scopy(scopy(t, g ? "II " : "I "), osc ? (gpar(g, GP_OLFO_DEST) == DR_TO_XWAVE ? "wave" : "pitch")
                                                  : (gpar(g, GP_FLFO_DEST) == DR_TO_RESO ? "reso" : "cutoff"));
            col = K_DRONE[g];
        } else {
            route = proj.par[P_DLY_LFO];
            depth = proj.par[P_DLY_LFO_DEPTH];
            scopy(t, "delay");
            col = K_FX;
        }
        ax = 222 - text_w(&FONT_S, t) - 6;
        text_r(222, ny, t, route == DR_LFO_OFF ? K_FAINT : depth ? col : blend(K_PANEL, col, 1, 2));
        for (w = 0; w < 2; w++) {                  /* a line from each wave the route takes */
            int sh = proj.par[w ? P_LFO_W2 : P_LFO_W1];
            if (route != w + 1 && route != DR_LFO_BOTH)
                continue;
            {
                uint16_t lc = !sh || !depth ? blend(K_PANEL, col, 1, 3) : col;
                int32_t sx = LFO_WX0 + LFO_WW + 4, sy = y0 + WY[w];
                if (sh && depth)                   /* a glow either side, as the plots */
                    cv_line(sx, sy + 1, ax, ay + 1, dim_of(col));
                cv_line(sx, sy, ax, ay, lc);
                cv_round(ax - 1, ay - 2, 5, 5, 2, lc);
            }
        }
    }
    {   /* the rate; locked to the swell, the lock beside it */
        int lk = proj.par[P_LFO_LOCK] != 0;
        int32_t x;
        dr_param_text(P_LFO_RATE, proj.par[P_LFO_RATE], scopy(t, lk ? (proj.par[P_PULSE] ? "locked " : "lock (no swell) ") : "rate "));
        x = cv_text(LFO_WX0, y0 + PAN_H - 18, &FONT_S, t, lk ? K_PULSE : K_LFO);
        if (proj.par[P_MOVE] != 50) {          /* the Movement (OCT- / OCT+) when it isn't as set */
            dr_param_text(P_MOVE, proj.par[P_MOVE], scopy(t, "move "));
            cv_text(x + 10, y0 + PAN_H - 18, &FONT_S, t, K_TEXT2);
        }
    }
}

/* LFO mods: a picture for each of its knobs. Top: the rate mod, the wave that moves the rate drawn as it now
 * runs (bright) over its plain shape (dim), and how far the rate swings (KNOB 1-2). Bottom left: cross-mod,
 * drone II's filter curve moving with drone I's sound, as far as its amount (KNOB 3). Bottom right: the
 * delay's time wobbling on its LFO wave, as far as its depth (KNOB 4; both scaled by the Movement) */
static void pic_lfo2(int32_t y0)
{
    int w[2] = {proj.par[P_LFO_W1], proj.par[P_LFO_W2]}, src = proj.par[P_LFO_RMOD_SRC], d = proj.par[P_LFO_RMOD];
    float rm = (float)d * 0.03f, mv = (float)proj.par[P_MOVE] * 0.02f, ph = 0.0f;
    const float rk = dr_lfo_rmod_k(rm, src, w[0], w[1]);   /* the engine's scale: two cycles still fit */
    int16_t yw[105], yp[105];
    int i, n = 105;
    char t[32], *q;
    /* --- the rate mod (top) --- */
    for (i = 0; i < n; i++) {                  /* 2 plain cycles across: the source wave warped, and plain */
        float v0 = dr_lfo_wave(0, w[0], ph), v1 = dr_lfo_wave(1, w[1], ph);
        float pl = 2.0f * (float)i / (float)(n - 1);
        float s = src == DR_LFO_BOTH ? fm_clampf(v0 + v1, -1.0f, 1.0f) : src == DR_LFO_II ? v1 : v0;
        float sp = src == DR_LFO_BOTH ? fm_clampf(dr_lfo_wave(0, w[0], pl) + dr_lfo_wave(1, w[1], pl), -1.0f, 1.0f)
                 : dr_lfo_wave(src == DR_LFO_II ? 1 : 0, w[src == DR_LFO_II ? 1 : 0], pl);
        yw[i] = (int16_t)(y0 + 38 - (int32_t)(s * 15.0f));
        yp[i] = (int16_t)(y0 + 38 - (int32_t)(sp * 15.0f));
        ph += (2.0f / (float)(n - 1)) * rk * fm_exp2f(rm * s);
        ph -= 2.0f * (float)(int)(ph * 0.5f);
    }
    if (d)
        plot(12, 2, yp, n, K_GRID);
    plot(12, 2, yw, n, d ? K_LFO : K_FAINT);
    cv_text(12, y0 + 2, &FONT_S, "RATE", d ? K_LFO : K_DIM);
    if (d) {                                   /* how far the rate swings: "x0.6 .. x1.7" */
        q = scopy(t, "x");
        q = dec1(rk * fm_exp2f(-fm_fabsf(rm)), q);
        q = scopy(q, " .. x");
        dec1(rk * fm_exp2f(fm_fabsf(rm)), q);
    } else {
        scopy(t, "off");
    }
    text_r(222, y0 + 2, t, d ? K_TEXT2 : K_DIM);
    cv_rect(12, y0 + 62, 210, 1, K_GRID);
    cv_rect(118, y0 + 68, 1, PAN_H - 72, K_GRID);
    /* --- cross-mod (bottom left): drone II's filter moving with drone I's sound --- */
    {
        int on = proj.par[P_XMOD] != 0;
        float a = (float)proj.par[P_XMOD_AMT] * 0.01f, cut = dr_cut[1];
        float sft = on ? dr_scope[0][(ui.frame * 7u) % DR_SCOPE_N] * 12.0f * a * a / 0.0981378f : 0.0f;
        if (on && a > 0.0f)
            draw_filter(1, cut, 14, y0 + 86, 96, 36, K_DRONE_DIM[1]);
        draw_filter(1, cut + sft, 14, y0 + 86, 96, 36, on ? K_DRONE[1] : K_FAINT);
        cv_text(12, y0 + 66, &FONT_S, on ? "I > II cut" : "x-mod off", on ? K_DRONE[1] : K_DIM);
    }
    /* --- the delay's time on its LFO (bottom right) --- */
    {
        int tap = proj.par[P_DLY_LFO], dd = proj.par[P_DLY_LFO_DEPTH];
        int w1 = tap != DR_LFO_II ? w[0] : 0, w2 = tap >= DR_LFO_II ? w[1] : 0;
        float dep = (float)dd * 0.01f * fm_fabsf((float)dd * 0.01f) * 0.25f * mv;   /* as the engine */
        int16_t yd[49];
        int32_t yc = y0 + 102;
        for (i = 0; i < 49; i++) {             /* the time, +-25 % full height, two cycles */
            float p2 = 2.0f * (float)i / 48.0f, v = dr_lfo_wave(0, w1, p2) + dr_lfo_wave(1, w2, p2);
            yd[i] = (int16_t)(yc - (int32_t)fm_clampf(v * dep / 0.25f * 16.0f, -18.0f, 18.0f));
        }
        cv_rect(126, yc, 96, 1, K_GRID);
        plot(126, 2, yd, 49, dd && (w1 || w2) ? K_FX : K_FAINT);
        q = scopy(t, "delay ");
        if (dd && (w1 || w2)) {
            q = scopy(q, "+-");
            itoa_u((uint32_t)(dep * 100.0f + 0.5f), q);
            while (*q)
                q++;
            scopy(q, "%");
        } else {
            scopy(q, "still");
        }
        cv_text(126, y0 + 66, &FONT_S, t, dd ? K_FX : K_DIM);
    }
}

/* Setup 2: the Morph (a switch's move from one setting to the next) and the master tune. The move ends as
 * far across as the knob is turned (0.02 s at the switch .. 30 s at the right edge: time in octaves, as the
 * knob is), so every turn moves it alike; ticks at 1 s and 10 s */
static void pic_setup2(int32_t y0)
{
    const int32_t xs = 12 + 12, len = 108 * proj.par[P_MORPH] / 100;
    int16_t y[61];
    int i, tn = proj.par[P_TUNE];
    char t[16], *q;
    for (i = 0; i < 61; i++) {                 /* from the switch, low to high */
        int32_t x = 12 + 2 * i;
        float v = x <= xs ? 0.0f : fm_minf((float)(x - xs) / (float)(len > 0 ? len : 1), 1.0f);
        y[i] = (int16_t)(y0 + 84 - (int32_t)(v * 50.0f));
    }
    cv_rect(12, y0 + 85, 120, 1, K_GRID);
    cv_rect(xs, y0 + 22, 1, 64, K_GRID);      /* the switch */
    cv_rect(xs + 58, y0 + 82, 1, 6, K_GRID);  /* 1 s, 10 s: where the knob puts them */
    cv_rect(xs + 92, y0 + 82, 1, 6, K_GRID);
    cv_text(xs + 58 - text_w(&FONT_XS, "1s") / 2, y0 + 90, &FONT_XS, "1s", K_DIM);
    cv_text(xs + 92 - text_w(&FONT_XS, "10s") / 2, y0 + 90, &FONT_XS, "10s", K_DIM);
    plot(12, 2, y, 61, K_SETUP);
    cv_text(12, y0 + 3, &FONT_S, "MORPH", K_SETUP);
    dr_param_text(P_MORPH, proj.par[P_MORPH], t);
    cv_text(12, y0 + 89, &FONT_S, t, K_TEXT2);
    cv_text(12, y0 + 109, &FONT_XS, "buttons & keys glide", K_DIM);
    cv_rect(146, y0 + 10, 1, PAN_H - 30, K_GRID);
    cv_text(156, y0 + 3, &FONT_S, "TUNE", K_SETUP);
    {   /* a needle: -50 .. +50 cents */
        int32_t cx = 190, nx = cx + tn * 30 / 50;
        cv_rect(cx - 30, y0 + 60, 61, 1, K_GRID);
        cv_rect(cx, y0 + 52, 1, 17, K_FAINT);
        cv_round(nx - 3, y0 + 57, 7, 7, 3, K_SETUP);
    }
    q = t;
    if (tn > 0)
        *q++ = '+';
    else if (tn < 0)
        *q++ = '-';
    itoa_u((uint32_t)(tn < 0 ? -tn : tn), q);
    while (*q)
        q++;
    scopy(q, " cents");
    text_c(190, y0 + 89, &FONT_S, t, K_TEXT2);
    dr_param_text(P_SCENE_T, proj.par[P_SCENE_T], scopy(t, "scenes "));   /* KNOB 3: a scene's morph */
    text_c(190, y0 + 109, &FONT_XS, t, K_DIM);
}

/* the reverb's tail, as measured (to -60 dB after a burst, QA batch 3; doubling the -30 dB time had made
 * the long ones short, and the Chorus hall no longer than the Hall) and its pre-delay: an envelope over 5 s,
 * as high as h */
static const float RV_TAIL_S[DR_NREV] = {0.7f, 2.6f, 5.7f, 6.1f};
static const float RV_PRE_S[DR_NREV] = {0.004f, 0.008f, 0.025f, 0.025f};
static void draw_tail(int type, int32_t x0, int32_t yb, int32_t w, int32_t h, uint16_t c)
{
    int16_t y[115];
    int i, n = (int)(w / 2) + 1;
    float T = RV_TAIL_S[(unsigned)type < DR_NREV ? type : 2];
    if (n > 115)
        n = 115;
    for (i = 0; i < n; i++) {                  /* dB falls in a straight line: -60 dB at T */
        float s = 5.0f * (float)i / (float)(n - 1) - RV_PRE_S[(unsigned)type < DR_NREV ? type : 2];
        float a = s < 0.0f ? 0.0f : fm_exp2f(-s / T * 9.966f);   /* 2^-9.966 = -60 dB */
        y[i] = (int16_t)(yb - (int32_t)(a * (float)h));
    }
    plot(x0, 2, y, n, c);
}

/* Effects: the delay's echoes (the dry sound, then each repeat as loud as Repeats leaves it, darker each
 * time) over 2 s; the reverb's tail under it, as high as its mix */
static void pic_fx(int32_t y0)
{
    float ms = 70.0f * fm_exp2f((float)proj.par[P_DLY_TIME] * 0.026845f);
    float fb = fm_minf((float)proj.par[P_DLY_REPEATS] * 0.0115f, 1.0f), a = 1.0f;
    float mix = (float)proj.par[P_DLY_MIX] * 0.01f, rmix = (float)proj.par[P_REV_MIX] * 0.01f;
    int32_t base = y0 + 66, h = 38, k;
    char t[24], *q;
    cv_rect(12, base, 208, 1, K_GRID);
    cv_rect(12, base - h, 3, h, K_FAINT);      /* the dry sound */
    for (k = 1; k < 40; k++) {
        int32_t x = 12 + (int32_t)(ms * (float)k * 0.104f);    /* 208 px = 2 s */
        int32_t bh;
        a *= fb;
        if (x > 218)
            break;
        bh = (int32_t)(a * mix * (float)h);
        if (bh < 1)
            break;
        cv_rect(x, base - bh, 3, bh, blend(K_FX, K_PANEL, k > 6 ? 6 : (int)k, 9));   /* darker each time */
    }
    q = scopy(t, "DELAY  ");
    dr_param_text(P_DLY_TIME, proj.par[P_DLY_TIME], q);
    cv_text(12, y0 + 3, &FONT_S, t, mix > 0.0f ? K_FX : K_DIM);
    if (proj.par[P_DLY_REPEATS] > 85)
        text_r(222, y0 + 3, "runaway", K_GOLD);
    q = scopy(t, "REVERB  ");
    scopy(q, DR_REV_NAME[(unsigned)proj.par[P_REV_TYPE] < DR_NREV ? proj.par[P_REV_TYPE] : 0]);
    cv_text(12, y0 + 74, &FONT_S, t, rmix > 0.0f ? K_FX : K_DIM);
    cv_rect(12, y0 + 122, 208, 1, K_GRID);
    if (rmix > 0.0f)
        draw_tail(proj.par[P_REV_TYPE], 12, y0 + 122, 208, (int32_t)(30.0f * rmix) + 1, K_FX);
}

/* Effects 2: the reverb type's tail and length (left, KNOB 1), or, while the speaker turns (KNOB 2), its low
 * cut: the response from 20 Hz to 1 kHz; the feedback loop's level, live (right top, KNOB 3); the oscillators'
 * character, a saw from clean to vintage (right bottom, KNOB 4) */
static void pic_fx2(int32_t y0)
{
    int ty = proj.par[P_REV_TYPE], fb = proj.par[P_FEEDBACK], ch = proj.par[P_CHAR], sp = proj.par[P_SPEAKER], i;
    char t[24], *q;
    if (lit(1)) {                              /* SPEAKER: 12 dB an octave under its corner, 0 dB .. -30 dB */
        float fc = 40.0f * fm_exp2f((float)sp * 0.0258496f);
        int16_t y[51];
        for (i = 0; i < 51; i++) {
            float f = 20.0f * fm_exp2f((float)i * (5.643856f / 50.0f)), r4 = f / fc;   /* 20 Hz .. 1 kHz */
            float db = sp ? 10.0f * 0.30103f * fm_log2f(r4 * r4 * r4 * r4 / (1.0f + r4 * r4 * r4 * r4)) : 0.0f;
            y[i] = (int16_t)(y0 + 34 - (int32_t)(fm_maxf(db, -30.0f) * 2.0f));
        }
        cv_text(12, y0 + 3, &FONT_S, "SPEAKER", sp ? K_FX : K_DIM);
        cv_rect(12, y0 + 94, 100, 1, K_GRID);
        plot(12, 2, y, 51, sp ? K_FX : K_FAINT);
        cv_text(12, y0 + 100, &FONT_S, sp ? "low cut" : "off: all the bass", K_TEXT2);
    } else {
        cv_text(12, y0 + 3, &FONT_S, "REVERB", K_FX);
        cv_rect(12, y0 + 94, 100, 1, K_GRID);
        draw_tail(ty, 12, y0 + 94, 100, 60, K_FX);
        q = scopy(t, DR_REV_NAME[(unsigned)ty < DR_NREV ? ty : 0]);
        q = scopy(q, "  ");
        q = dec1(RV_TAIL_S[(unsigned)ty < DR_NREV ? ty : 2], q);
        scopy(q, " s");
        cv_text(12, y0 + 100, &FONT_S, t, K_TEXT2);
    }
    cv_rect(120, y0 + 10, 1, PAN_H - 30, K_GRID);
    {   /* FEEDBACK: a bar, the loop's level now */
        int32_t bw = (int32_t)(fm_clampf(dr_fb, 0.0f, 1.0f) * 94.0f);
        cv_text(128, y0 + 3, &FONT_S, "FEEDBACK", fb ? K_FX : K_DIM);
        cv_round(128, y0 + 24, 94, 8, 3, K_GRID);
        if (fb && bw > 3)
            cv_round(128, y0 + 24, bw, 8, 3, dr_fb > 0.6f ? K_GOLD : K_FX);
    }
    {   /* CHARACTER: two cycles of a saw as the oscillators now make it (bent, rounded) */
        float c = (float)ch * 0.01f, r = 0.0f, k2 = 1.0f - 0.85f * c;
        int16_t y[48];
        for (i = 0; i < 48 + 8; i++) {         /* 8 to settle the rounding */
            float ph = (float)(i % 24) / 24.0f, s = 1.0f - 2.0f * ph;
            s += 0.25f * c * s * s - 0.083f * c;
            r += (s - r) * k2;
            if (i >= 8)
                y[i - 8] = (int16_t)(y0 + 84 - (int32_t)(r * 16.0f));
        }
        plot(128, 2, y, 48, ch ? K_TEXT2 : K_FAINT);
        cv_text(128, y0 + 46, &FONT_S, ch ? "CHARACTER" : "clean", ch ? K_TEXT2 : K_DIM);
    }
}

/* Setup: a small sketch for each knob: the glide, the fade's curve, the drift, how wide the drones sit */
static void pic_setup(int32_t y0)
{
    float glide = (float)proj.par[P_GLIDE] * 0.01f, fade = fade_s();
    float drift = (float)proj.par[P_DRIFT] * 0.08f, wid = (float)proj.par[P_WIDTH] * 0.01f;
    int32_t yb = y0 + 74, i, cx;
    int16_t y[25];
    char t[16], *q;
    /* GLIDE: a step in pitch, then the slide to it (straight up when 0) */
    glide = 10.0f * glide * glide;
    for (i = 0; i < 25; i++) {
        float s = (float)i / 24.0f * 3.0f - 0.5f;   /* -0.5 .. 2.5 s */
        float v = s < 0.0f ? 0.0f : glide <= 0.0f ? 1.0f : 1.0f - fm_exp2f(-s / (glide * 0.25f) * 1.4427f);
        y[i] = (int16_t)(yb - (int32_t)(v * 40.0f));
    }
    plot(10, 2, y, 25, K_SETUP);
    if (glide <= 0.0f)
        scopy(t, "none");
    else
        scopy(dec1(glide, t), " s");
    cv_text(10, yb + 4, &FONT_XS, "Glide", K_DIM);
    cv_text(10, yb + 20, &FONT_S, t, K_TEXT2);
    /* FADE: PLAY's fade in, over its time (the sketch spans 0 .. 5 s) */
    for (i = 0; i < 25; i++) {
        float s = (float)i / 24.0f * 5.0f, v = fm_minf(s / fade, 1.0f);
        y[i] = (int16_t)(yb - (int32_t)(v * v * 40.0f));
    }
    plot(68, 2, y, 25, K_SETUP);
    scopy(dec1(fade, t), " s");
    cv_text(68, yb + 4, &FONT_XS, "Fade", K_DIM);
    cv_text(68, yb + 20, &FONT_S, t, K_TEXT2);
    /* DRIFT: a slow wander, as wide as it is set (+-8 cents at the top), moving */
    for (i = 0; i < 25; i++) {
        float s = (float)i * 0.35f + (float)(plat_ms() % 60000u) * 0.0008f;
        y[i] = (int16_t)(yb - 20 - (int32_t)(fm_sinf(s) * fm_sinf(s * 0.37f + 1.0f) * drift * 2.2f));
    }
    plot(126, 2, y, 25, K_SETUP);
    q = scopy(t, "+-");
    dec1(drift, q);
    for (q = t; *q; q++)
        ;
    scopy(q, "c");
    cv_text(126, yb + 4, &FONT_XS, "Drift", K_DIM);
    cv_text(126, yb + 20, &FONT_S, t, K_TEXT2);
    /* STEREO: drone I and II, as far apart as Width puts them */
    cx = 204;
    cv_rect(cx - 22, yb - 20, 45, 1, K_GRID);
    cv_round(cx - 3 - (int32_t)(wid * 20.0f), yb - 24, 9, 9, 4, K_DRONE[0]);
    cv_round(cx - 5 + (int32_t)(wid * 20.0f), yb - 24, 9, 9, 4, K_DRONE[1]);
    if (wid <= 0.0f)
        scopy(t, "mono");
    else {
        itoa_u((uint32_t)proj.par[P_WIDTH], t);
        for (q = t; *q; q++)
            ;
        scopy(q, "%");
    }
    cv_text(184, yb + 4, &FONT_XS, "Stereo", K_DIM);
    cv_text(184, yb + 20, &FONT_S, t, K_TEXT2);
}

/* the picture: the page's own (on Play, the two waveforms), a banner while a message shows. No interval
 * between the drones (Evan, 2026-10-10: tune by ear) */
static void draw_pic(void)
{
    int msg = plat_ms() < ui.msg_until;
    uint32_t h = hash(hash(2166136261u, (uint32_t)msg | ui.view << 2),
                      msg ? ui.msg_until : 0);
    if (h == ui.sig[1] && ui.frame % SCOPE_EVERY)
        return;                                /* the waveforms move: redrawn ~20 times a second */
    ui.sig[1] = h;
    cv_begin(240, PIC_H, K_BG);
    if (ui.view == V_PULSE || ui.view == V_LFO || ui.view == V_LFO2 || ui.view == V_FX || ui.view == V_FX2 ||
        ui.view == V_SETUP || ui.view == V_SETUP2) {
        cv_round(6, 4, WIDE_W, PAN_H, 4, K_PANEL);
        switch (ui.view) {
        case V_PULSE: pic_pulse(4); break;
        case V_LFO: pic_lfo(4); break;
        case V_LFO2: pic_lfo2(4); break;
        case V_FX: pic_fx(4); break;
        case V_FX2: pic_fx2(4); break;
        case V_SETUP2: pic_setup2(4); break;
        default: pic_setup(4); break;
        }
    } else {
        int g;
        for (g = 0; g < DR_NGROUPS; g++) {
            int32_t x0 = pan_x(g);
            switch (ui.view) {
            case V_PLAY2: case V_TONE2: case V_OSC: case V_PARTNER:
                cv_round(x0, 4, PAN_W, PAN_H, 4, K_PANEL);
                cv_text(x0 + 5, 7, &FONT_S, g ? "II" : "I", K_DRONE[g]);
                break;
            default:
                draw_scope(g, x0, 4, PAN_W, PAN_H);
            }
            switch (ui.view) {
            case V_PLAY2: pic_play2(g, x0, 4); break;
            case V_TONE2: pic_tone2(g, x0, 4); break;
            case V_OSC: pic_osc(g, x0, 4); break;
            case V_PARTNER: pic_partner(g, x0, 4); break;
            default: break;
            }
        }
    }
    if (msg) {                                 /* a banner across the top of the picture */
        uint16_t c = ui.msg_col ? ui.msg_col : K_GOLD;
        int32_t x;
        cv_round(6, 6, 228, 26, 6, K_CARD);
        cv_frame(6, 6, 228, 26, c);
        x = 120 - (text_w(&FONT_B, ui.msg[0]) + (ui.msg[1][0] ? 8 + text_w(&FONT_B, ui.msg[1]) : 0)) / 2;
        x = cv_text(x, 11, &FONT_B, ui.msg[0], c);
        if (ui.msg[1][0])
            cv_text(x + 8, 11, &FONT_B, ui.msg[1], K_TEXT);
    }
    cv_blit(0, PIC_Y);
}

/* the page's four values: KNOB 1-2 under drone I, KNOB 3-4 under drone II. At rest, quiet; while a knob
 * moves (and a moment after), its value lights: a card in its colour, the value large, its bar */
static void draw_values(void)
{
    uint32_t h = hash(2166136261u, ui.view | (uint32_t)proj.par[P_PLINK] << 8), i;
    for (i = 0; i < 4; i++) {
        int p = knob_param((int)i);
        h = hash(h, (p < 0 ? 0xFFFFu : (uint32_t)(uint16_t)proj.par[p]) | (uint32_t)lit((int)i) << 16 |
                        (uint32_t)(ui.btn >> B_SEL & 1u) << 17 | (uint32_t)(proj.par[P_PLOCK] != 0) << 18);
    }
    if (h == ui.sig[2])
        return;
    ui.sig[2] = h;
    cv_begin(240, VAL_H, K_BG);
    cv_rect(0, 0, 240, 1, K_GRID);
    cv_rect(120, 8, 1, 64, K_FAINT);           /* drone I | drone II */
    for (i = 0; i < 4; i++) {
        int p = knob_param((int)i), lo, hi, v, on = lit((int)i);
        knob_t k = VIEW_KNOB[ui.view][i];
        int32_t x = 6 + (int32_t)i * 58 + (i >= 2 ? 2 : 0), w = 54, cx = x + w / 2, bw;
        uint16_t accent = k.g < G ? K_DRONE[k.g] : k.p == P_XMOD_AMT ? K_DRONE[1] : page_col() == K_DIM ? K_TEXT : page_col();
        int is_pitch = k.p == GP_PITCH && k.g < G, locked = is_pitch && proj.par[P_PLOCK] && on;
        char t[16];
        if (p < 0)
            continue;
        if (locked)                            /* PITCH LOCK: a turned pitch knob does nothing, so its card */
            on = 0;                            /* stays dark (the banner says why) */
        lo = dr_param_info(p)->lo;
        hi = dr_param_info(p)->hi;
        v = proj.par[p];
        if (on) {
            cv_round(x - 2, 3, w + 4, 79, 6, K_CARD);
            cv_frame(x - 2, 3, w + 4, 79, accent);
        }
        text_fit(cx, 8, w + 2, &FONT_XS, card_name(p), on ? accent : K_DIM);
        if (p == P_PLINK_OFF) {                /* the link offset in semitones and cents: "+7.00" */
            int a = v < 0 ? -v : v;
            char *q = t;
            *q++ = v < 0 ? '-' : '+';
            itoa_u((uint32_t)(a / 100), q);
            while (*q)
                q++;
            *q++ = '.';
            *q++ = (char)('0' + a % 100 / 10);
            *q++ = (char)('0' + a % 10);
            *q = 0;
        } else {
            dr_param_text(p, v, t);
        }
        {   /* the value: large while lit, else as large as fits */
            const felucca_font_t *f = fit_face(t, w + 2, on);
            text_fit(cx, f == &FONT_M ? 24 : f == &FONT_XS ? 32 : 28, w + 2, f, t, on ? K_TEXT : K_TEXT2);
        }
        if (is_pitch) {                        /* a pitch: no bar; while it moves, how to fine-tune */
            if (on)
                text_c(cx, 56, &FONT_XS, ui.btn >> B_SEL & 1u ? "FINE" : "SEL: fine", K_LFO);
            else if (locked)
                text_c(cx, 56, &FONT_XS, "locked", K_DIM);
            continue;
        }
        bw = hi > lo ? (int32_t)(42 * (v - lo) / (hi - lo)) : 0;
        cv_round(cx - 21, 62, 42, 4, 2, K_GRID);
        if (hi - lo == 1) {                    /* two ways (White / Pink, Equal / Just): the half it's on */
            cv_round(cx - 21 + (v > lo ? 22 : 0), 62, 20, 4, 2, on ? accent : blend(K_GRID, accent, 1, 2));
        } else if (lo < 0) {                   /* centred values: a bar from the middle */
            int32_t m = cx - 21 + 42 * (0 - lo) / (hi - lo), e = cx - 21 + bw;
            cv_round(e < m ? e : m, 62, (e < m ? m - e : e - m) + 3, 4, 2, on ? accent : blend(K_GRID, accent, 1, 2));
        } else if (bw) {
            cv_round(cx - 21, 62, bw < 4 ? 4 : bw, 4, 2, on ? accent : blend(K_GRID, accent, 1, 2));
        }
    }
    cv_blit(0, VAL_Y);
}

static void leds(void)
{
    uint32_t b = 0, k = 0, i;
    for (i = 0; i < NPAGE_BTN; i++)                  /* the button of the page shown */
        if (on_pages((int)i))
            b |= 1u << PAGE_BTN[i].btn;
    if (ui.playing)
        b |= 1u << B_PLAY;
    if (proj.par[P_MOVE] > 50)                         /* the Movement off 100 %: the way it's gone */
        b |= 1u << B_OCTUP;
    else if (proj.par[P_MOVE] < 50)
        b |= 1u << B_OCTDN;
    if (proj.par[P_PULSE]) {                           /* ARP: in time with the swell, from the moment it's on: */
        float b1 = 1.0f + (float)proj.par[P_PULSE_SWING] * 0.01f, ph = dr_pulse_ph;   /* the first half */
        if ((ph < b1 ? ph / b1 : (ph - b1) / (2.0f - b1)) < 0.5f)                     /* of each beat */
            b |= 1u << B_ARP;
    }
    if (ui.loop == L_PLAY || (ui.loop == L_REC && (ui.frame / 8u) & 1u))
        b |= 1u << B_REC;                              /* REC: blinking recording, steady looping */
    for (i = 0; i < 11u; i++) {                      /* the black keys: the switches that are on */
        sw_t w = sw_layer()[i];
        int v = w.s == S_LFORESET ? (int)(ui.keys >> BLACK_K[i] & 1u) || proj.par[P_LFO_LOCK] != 0 : sw_get(w);
        if (v == 2)                                    /* a third state (both LFO waves): lit, winking */
            v = plat_ms() % 1200u < 1000u;
        if (v)
            k |= 1u << BLACK_K[i];
    }
    for (i = 0; i < NSCENES; i++)                    /* the white keys: the slots that hold a scene; the one */
        if ((proj.scene_used >> i & 1u) &&             /* the sound is, blinking slowly */
            (ui.scene_at != (int8_t)i || plat_ms() % 1000u < 600u))
            k |= 1u << WHITE_K[i];
    plat_leds(b, k);
}

/* AUTOSAVE: a few seconds after a change, while the drone is stopped and nothing is touched */
#define AUTOSAVE_QUIET 4000u
static void autosave(void)
{
    uint32_t now = plat_ms();
    if (!ui.dirty || ui.playing || dr_fade > 0.0f || ui.btn || ui.keys || now - ui.act_t < AUTOSAVE_QUIET)
        return;
    if (project_save() == 0)
        ui.dirty = 0;
}

void ui_init(void)
{
    memset(&ui, 0, sizeof ui);
    ui.hold_btn = -1;
    ui.morph = -1;
    ui.visit = -1;
    ui.scene_at = -1;
    ui.save_q = -2;
    ui.splash_t0 = plat_ms();
    lcd_fill(0, 0, 240, 240, K_BG);
}

/* ------------------------------------------------------------- the start --- */
/* As the web page: a red field, two thick bands (violet, and one that darkens what it crosses) drifting in from
 * the right and settling across the name, which turns pale pink where they cross it; it holds still for half a
 * second where it lands, then fades to the Play page slowly. About 3.5 s (the last 1.3 s the fade); any button
 * or key ends it at once (and does what it does). The drones stay silent until PLAY. Not in the host
 * simulator's scenarios, which start at once (the browser emulator has it; OM_SPLASH shows it on the host). */
#if defined(OM_HOST) && !defined(OM_WEB) && !defined(OM_SPLASH)
#define SPLASH_MS 0u
#else
#define SPLASH_MS 3500u
#endif
#define SPLASH_HOLD_AT 1700u                  /* where it lands: held still from here (Evan: half a second) */
#define SPLASH_HOLD 500u
#define SP_RED RGB(254, 0, 0)
#define SP_VIOLET RGB(118, 5, 149)
#define SP_DARK_RED RGB(58, 0, 0)             /* the dark band over the red (multiplied, as on the page) */
#define SP_DARK_VIOLET RGB(27, 0, 49)         /* ... over the violet */
#define SP_INK RGB(20, 0, 15)
#define SP_PALE RGB(255, 214, 240)
static int16_t sp_l[2][240], sp_r[2][240];    /* each band's edges, row by row */

static int sp_in(int32_t x, int32_t y)
{
    return (x >= sp_l[0][y] && x < sp_r[0][y]) || (x >= sp_l[1][y] && x < sp_r[1][y]);
}
static float sp_ease(float x) { x = fm_clampf(x, 0.0f, 1.0f); return 1.0f - (1.0f - x) * (1.0f - x); }

/* text whose colour depends on where it is: near-black on the red, pale over a band; q of 16 faded in */
static void sp_text(int32_t x, int32_t y, int32_t y0, const felucca_font_t *f, const char *s, int q, uint16_t out_c,
                    uint16_t in_c)
{
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t a = gd[gy * bpr + gx / 2u];
                int32_t px = x - f->pad + (int32_t)gx, sy = y + (int32_t)gy, py = sy - y0;
                a = (gx & 1u) ? (a & 15u) : (a >> 4);
                a = a * (uint32_t)q / 16u;
                if (!a || (uint32_t)px >= cv_w || py < 0 || py >= (int32_t)cv_h || sy < 0 || sy >= 240)
                    continue;
                {
                    uint16_t c = sp_in(px, sy) ? in_c : out_c, bg = swap16(cv_px[(uint32_t)py * cv_w + (uint32_t)px]);
                    cv_px[(uint32_t)py * cv_w + (uint32_t)px] = swap16(blend(bg, c, (int)a, 15));
                }
            }
        x += f->adv[gi];
    }
}

static void draw_splash(uint32_t t0)
{
    /* its own time stops for the hold: everything after it is as before, half a second later */
    const uint32_t t = t0 < SPLASH_HOLD_AT ? t0 : t0 < SPLASH_HOLD_AT + SPLASH_HOLD ? SPLASH_HOLD_AT : t0 - SPLASH_HOLD;
    const float tt = (float)t * 0.001f;
    const float in = sp_ease(tt / 1.4f), fade = fm_clampf(tt / 0.4f, 0.0f, 1.0f),
                out = fm_clampf(((float)t - (float)(SPLASH_MS - SPLASH_HOLD) + 1300.0f) / 1300.0f, 0.0f, 1.0f);
    const int lit = (int)(16.0f * fade * (1.0f - out) + 0.5f);        /* the whole picture, from black and back */
    const int tq = (int)(16.0f * fm_clampf((tt - 0.4f) / 0.8f, 0.0f, 1.0f) + 0.5f);   /* the name */
    const int32_t rise = (int32_t)(8.0f * (1.0f - sp_ease((tt - 0.4f) / 0.8f)));
    static const float LEN[2] = {0.9f, 0.8f}, PH[2] = {0.0f, 0.5f};
    static char ver[40];                                             /* "niagara-drift 0.1" (the build's version) */
    int32_t y, half, b;
    if (!ver[0]) {
        char *q = scopy(ver, "niagara-drift ");
        const char *v = OM_VERSION;
        for (; *v && q < ver + sizeof ver - 1; v++)
            *q++ = (char)(*v >= 'A' && *v <= 'Z' ? *v + 32 : *v);
        *q = 0;
    }
    for (b = 0; b < 2; b++)                                          /* the bands: from off the right, settling */
        for (y = 0; y < 240; y++) {
            float a = 6.2831853f * (float)y / (240.0f * LEN[b]) + PH[b] + tt * 0.6f;
            float cx = 240.0f * (1.15f - 0.6f * in + 0.12f * fm_sinf(a));
            float hw = 240.0f * 0.065f * (1.0f + 0.2f * fm_sinf(a * 1.4f + PH[b]));
            sp_l[b][y] = (int16_t)fm_clampf(cx - hw, -1.0f, 241.0f);
            sp_r[b][y] = (int16_t)fm_clampf(cx + hw, -1.0f, 241.0f);
        }
    for (half = 0; half < 2; half++) {                               /* the screen in two halves (the canvas) */
        const int32_t y0 = half * 120;
        cv_begin(240, 120, K_BG);
        for (y = 0; y < 120; y++) {
            int32_t sy = y0 + y, ol = sp_l[0][sy] > sp_l[1][sy] ? sp_l[0][sy] : sp_l[1][sy],
                    orr = sp_r[0][sy] < sp_r[1][sy] ? sp_r[0][sy] : sp_r[1][sy];
            cv_rect(0, y, 240, 1, SP_RED);
            cv_rect(sp_l[0][sy], y, sp_r[0][sy] - sp_l[0][sy], 1, SP_VIOLET);
            cv_rect(sp_l[1][sy], y, sp_r[1][sy] - sp_l[1][sy], 1, SP_DARK_RED);
            if (orr > ol)
                cv_rect(ol, y, orr - ol, 1, SP_DARK_VIOLET);
        }
        sp_text(14, 64 + rise, y0, &FONT_L, "NIAGARA", tq, SP_INK, SP_PALE);
        sp_text(14, 98 + rise, y0, &FONT_L, "DRIFT", tq, SP_INK, SP_PALE);
        sp_text(14, 214, y0, &FONT_S, ver, tq, SP_INK, SP_PALE);
        if (lit < 16) {                                              /* from black, and back to it */
            uint32_t i, n = cv_w * cv_h;
            for (i = 0; i < n; i++)
                cv_px[i] = swap16(blend(K_BG, swap16(cv_px[i]), lit, 16));
        }
        cv_blit(0, (uint32_t)y0);
    }
}

/* KEY PRESSURE, a test readout (v0.2): while a key is held, how hard, 0..100 %; nothing plays from it yet.
 * The lowest held key; the seven top keys (C#5 .. G5) sense no pressure. */
static void pressure_readout(void)
{
    static const char *const NAME[12] = {"F", "F#", "G", "G#", "A", "A#", "B", "C", "C#", "D", "D#", "E"};
    char a[24], b[24];
    uint32_t k, i, n;
    int p;
    if (!ui.keys)
        return;
    for (k = 0; !(ui.keys >> k & 1u); k++)
        ;
    p = plat_pressure(k);
    n = 0;
    for (i = 0; NAME[k % 12u][i]; i++)
        a[n++] = NAME[k % 12u][i];
    a[n++] = (char)('3' + (k + 5u) / 12u);
    a[n] = 0;
    if (p < 0) {
        say(a, "NO PRESSURE");
        return;
    }
    n = (uint32_t)(scopy(b, "PRESSURE ") - b);
    itoa_u((uint32_t)(p * 100 + 63) / 127u, b + n);
    for (; b[n]; n++)
        ;
    b[n++] = '%';
    b[n] = 0;
    say(a, b);
}

void ui_frame(void)
{
    input();
    pressure_readout();
    scene_morph();
    autosave();
    if (!ui.splash_done) {
        uint32_t t = plat_ms() - ui.splash_t0;
        if (ui.btn || ui.keys || t >= SPLASH_MS) {                    /* over, or a hand on the FM-1 */
            ui.splash_done = 1;
            ui.sig[0] = ui.sig[1] = ui.sig[2] = 0;                   /* every band drawn afresh */
            lcd_fill(0, 0, 240, 240, K_BG);
        } else {
            draw_splash(t);
            leds();
            ui.frame++;
            return;
        }
    }
    draw_bar();
    draw_pic();
    draw_values();
    leds();
    ui.frame++;
}

void ui_input_only(void) { input(); }
