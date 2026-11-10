/* SPDX-License-Identifier: GPL-3.0-only */
/* Niagara Drift UI: the control map (Figma "Niagara Drift control map: draft 1"). Two drones, after the CPM DS-2.
 * Adapted from FoMni's ui.c: its drawing helpers, knob acceleration and three-band screen. Part of the
 * unity build (after gfx.c and project.c); the host simulator includes it too.
 *
 * Always live:
 *   KNOB 1-2       drone I's two values on the page shown, KNOB 3-4 drone II's; the Drones page (HOME):
 *                  Pitch and Cutoff. A pitch is continuous, 2 Hz .. 1.6 kHz, a slow full turn an octave;
 *                  hold SEL for a cent a detent. With PITCH LINK on, KNOB 3 moves drone II's offset.
 *   SELECT         LFO rate      ALGORITHM   MOVEMENT: every LFO depth together, 0 .. 200 %
 *   PRESETS        GESTURE TIME: every gesture's speed, x0.1 .. x10 (v0.2)
 *   PLAY           the drone fades in / out      MASTER   volume      SAVE   saves
 * One button away (tap: the knobs stay on its page, pressed again its second page; hold: they switch
 * only while it's held, and the page you were on comes back when it's let go):
 *   HOME Drones (Level, Cutoff), Pitch (Pitch, Reso) · ENV Life (Feedback, Drift, Character, Age), Tone (Noise, Colour: white
 *   .. pink) · EDIT Osc (X-Wave, PW), Sub · Drive (Osc/Sub, Drive) · LFO LFO sends (each drone's two sends), Mods
 *   (Speed mod, Source, Delay mod, Cross-mod) · SEQ Partner, Pitch moves · FX Space (Repeats, Mix, Length, Mix),
 *   Details (Drive, Time, Damp, Type) · GLO Setup 1 (Fade, Morph, Scene time),
 *   Setup 2 (Tuning, Tune, Stereo, Pressure) (global). The top bar shows a dot for each of a
 *   button's pages.
 *   OCT- / OCT+    the LFO's Wave I / Wave II (LFO I and II on screen): a tap is the next shape, round and
 *                  round; SEL + OCT the one before; lit while its wave is up
 * White keys: 16 scenes. Tap: recall (every value morphs there over the Scene time; a knob turned stops
 *   it); hold (a visit): there while held, back to where it was when let go (a knob turned: it stays);
 *   hold SAVE and tap: store. Lit: the slot holds a scene; blinking slowly: the sound is that scene (stored or
 *   recalled, and no knob, switch or LFO control touched since).
 * Black keys, lit when on (or on the second choice), following the page:
 *   F#3 G#3 A#3   C#4 D#4   F#4 G#4 A#4   C#5 D#5   F#5. Normally, by meaning: drone I (Partner, Sub
 *   (round: -1, -2 oct, 1/3, 1/5, -3, -4 oct), Filter (round: LP, BP, HP)) | global (Pitch lock, LFO reset) | drone II
 * *   (the same) | links (Pitch link, Hard sync) | Cross-mod (round: off, cutoff, pitch). On the LFO page: Osc wave, Osc LFO -> X-Wave, Filter LFO -> Reso | Filter wave,
 *   LFO reset | Osc wave, Osc LFO -> X-Wave, Filter LFO -> Reso | Filter wave, Delay wave | Cross-mod. A
 *   wave key goes round I, II, I+II (lit for II, winking for both). LFO reset (either layer): a tap restarts
 *   the LFO; held half a second, it locks the LFO to the Pulse's bars (restarting each bar; lit while
 *   locked), held again, free.
 * ARP            tap: the SWELL page; tapped on it, the Swell on / off (lit, in time); hold: a peek
 * REC            the MOTION LOOP: tap to record what the knobs, gestures and scenes do, tap again to loop it from the start,
 *                tap again to stop (everything stays where it is); up to 5 minutes (lit: blinking recording,
 *                steady looping)
 *   FX             Delay: drive (after the delay, v0.3), time, repeats, mix, and with SEL held KNOB 4 the delay's
 *                  type; again: Reverb: length (to Inf), damp, type, mix, and with SEL held KNOB 4 what goes into
 *                  it (the Types page folded in: review 5, Evan 2026-10-28)
 *                  (the delay LFO's tap: the LFO page's key in drone II's filter group)
 *   ENV            Life: Feedback, Drift, Character, Age (first: review 5, 2026-10-28; reordered 2026-10-31); again: Tone: each
 *                  drone's noise and colour
 * Saving: SAVE, and by itself a few seconds after a change while stopped (a flash write stops the audio
 * for a moment). MIDI in: notes on channel 1 tune drone I, on channel 2 drone II. */

/* ------------------------------------------------------------- pages --- */
/* the pages (2026-10-10, from the review: what's played together sits together, no half-empty pages) */
enum { V_PLAY, V_PLAY2, V_LIFE, V_TONE, V_OSC, V_SOURCE, V_LFO, V_LFO2, V_PARTNER, V_MOVES, V_SETUP, V_SETUP2,
       V_PULSE, V_FX, V_FX2, NVIEWS };
static const char *const VIEW_NAME[NVIEWS] = {"Drones", "Pitch", "Life", "Tone", "Osc", "Sub · Drive", "LFO sends",
                                              "Mods", "Partner", "Pitch moves", "Setup 1", "Setup 2", "Swell", "Space", "Details"};
/* what each knob moves on each page: a drone's parameter (g 0 or 1, GP_*), a global one (g G, P_*), or
 * nothing (g NONE) */
#define G 2
#define NONE 3
typedef struct {
    uint8_t g, p;
} knob_t;
static const knob_t VIEW_KNOB[NVIEWS][4] = {
    /* HOME (v0.3, Evan, 2026-11-08: what drone players move most, each drone's level and cutoff, on the page HOME
     * opens; the pitch, set once and left, and the resonance behind it) */
    {{0, GP_LEVEL}, {0, GP_CUTOFF}, {1, GP_LEVEL}, {1, GP_CUTOFF}},
    {{0, GP_PITCH}, {0, GP_RESO}, {1, GP_PITCH}, {1, GP_RESO}},
    {{G, P_FEEDBACK}, {G, P_DRIFT}, {G, P_CHAR}, {G, P_AGE}},   /* the one turned most first (Evan, 2026-10-31) */
    {{0, GP_NOISE}, {0, GP_NCOLOR}, {1, GP_NOISE}, {1, GP_NCOLOR}},
    {{0, GP_WAVE}, {0, GP_WIDTH}, {1, GP_WAVE}, {1, GP_WIDTH}},
    {{0, GP_MIX}, {0, GP_DRIVE}, {1, GP_MIX}, {1, GP_DRIVE}},
    {{0, GP_OLFO_DEPTH}, {0, GP_FLFO_DEPTH}, {1, GP_OLFO_DEPTH}, {1, GP_FLFO_DEPTH}},
    {{G, P_LFO_RMOD}, {G, P_LFO_RMOD_SRC}, {G, P_DLY_LFO_DEPTH}, {G, P_XMOD_AMT}},   /* the LFO's three, then cross-mod */
    {{0, GP_INTERVAL}, {0, GP_DETUNE}, {1, GP_INTERVAL}, {1, GP_DETUNE}},
    {{G, P_SHIFT_MOVE}, {G, P_GLIDE}, {G, P_WANDER}, {G, P_WSCALE}},   /* Shift's move, Glide; Wander, its scale */
    /* GLO: the room's settings only, none in a sound (Evan, 2026-10-18): the times, then the unit and the room */
    {{G, P_FADE}, {G, P_MORPH}, {G, P_SCENE_T}, {G, P_CLK_SYNC}},   /* Sync on KNOB 4 (v0.3) */
    {{G, P_TUNING}, {G, P_TUNE}, {G, P_WIDTH}, {G, P_PRESS_SENS}},   /* Tuning and Tune side by side (2026-11-01) */
    {{G, P_PULSE_RATE}, {G, P_PULSE_DEPTH}, {G, P_PULSE_SHAPE}, {G, P_PULSE_SWING}},
    /* FX (v0.3, Evan, 2026-11-08): Space, the amounts played (the delay's repeats and mix, the reverb's length and
     * mix), the page FX opens; Details, what's set before (the delay's drive and time, the reverb's damp and type), with
     * SEL the delay's type over Time and what goes into the reverb over Type (sel_job) */
    {{G, P_DLY_REPEATS}, {G, P_DLY_MIX}, {G, P_REV_LEN}, {G, P_REV_MIX}},
    {{G, P_DLY_DRIVE}, {G, P_DLY_TIME}, {G, P_REV_DAMP}, {G, P_REV_TYPE}},
};
/* A KNOB's SECOND JOB with SEL held, on the pages that have one (its card shows both, the second "+ SEL"): the
 * choices set before playing, so a player who never finds them loses nothing. FX's KNOB 4: the delay's type, what goes
 * into the reverb; Sub · Drive's Osc mix knobs: each drone's sub wave (Evan, 2026-10-28) */
static int sel_job(int v, int i)
{
    if (v == V_FX2 && (i == 1 || i == 3))      /* Details: the delay's type by its Time, Into by the reverb's Type */
        return i == 1 ? P_DLY_TYPE : P_DLY_ROUTE;
    if (v == V_SOURCE && (i == 0 || i == 2))
        return i ? P_SUBW_1 : P_SUBW_0;
    return -1;
}
static int page_jobs(int v) { return sel_job(v, 0) >= 0 || sel_job(v, 1) >= 0 || sel_job(v, 3) >= 0; }
static const char *sel_job_name(int p) { return p == P_DLY_ROUTE ? "Into" : p == P_DLY_TYPE ? "Type" : "Wave"; }
/* the page buttons: each shows its first page, pressed again the next */
static const struct {
    uint8_t btn, first, n;
} PAGE_BTN[] = {{B_HOME, V_PLAY, 2}, {B_ENV, V_LIFE, 2}, {B_EDIT, V_OSC, 2}, {B_LFO, V_LFO, 2},
                {B_SEQ, V_PARTNER, 2}, {B_GLO, V_SETUP, 2}, {B_FX, V_FX, 2},
                {B_ARP, V_PULSE, 1}};                                    /* ARP: on its page, a tap toggles */
#define NPAGE_BTN (sizeof PAGE_BTN / sizeof PAGE_BTN[0])
#define HOLD_MS 400u                   /* a page button held this long (or a knob turned) is a peek */

/* ------------------------------------------------------------ switches --- */
/* the black keys (key numbers, F3 = 0) and what each does: normally, and on the LFO page */
static const uint8_t BLACK_K[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
/* the white keys: the first NPLAY (F3 .. F4) the scenes of the bank; the next four (G4 .. C5, which sense pressure
 * too) the pressure gestures; the top four (D5 .. G5, no pressure) the gestures (Weight, Shift, Clean, Wash); SEL + any
 * of those eight: a bank (KEY_BANK) */
#define NWHITE 16
static const uint8_t WHITE_K[NWHITE] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
static const char BANK_L[NBANKS] = {'A', 'B', 'C', 'P', 'D', 'E', 'F', 'P'};
/* SEL + the eight gesture keys (G4 .. C5, D5 .. G5, left to right): the banks in a row, A .. F, then the Presets'
 * two (Evan, 2026-11-07: 6 yours + 2 Presets, one row) */
static const uint8_t KEY_BANK[8] = {0, 1, 2, BANK_D, BANK_D + 1, BANK_D + 2, BANK_PRESETS, BANK_PRESETS2};
static const char *const BANK_N[NBANKS] = {"A", "B", "C", "PRESETS 1-8", "D", "E", "F", "PRESETS 9-16"};
static const char *const GESTURE[DR_NGTYPE] = {"WEIGHT", "SHIFT", "CLEAN", "WASH", "TICK", "CHORD", "SNARL",
                                               "STUTTER"};   /* D5-G5, then G4-C5: fixed */
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
/* the LFO pages: each drone's keys in one order, the pitch send's LFO and target, then the cutoff send's (Evan,
 * 2026-11-03) */
static const sw_t SW_LFO[11] = {{S_OTAP, 0}, {S_ODEST, 0}, {S_FTAP, 0}, {S_FDEST, 0}, {S_LFORESET, 0},
                                {S_OTAP, 1}, {S_ODEST, 1}, {S_FTAP, 1}, {S_FDEST, 1}, {S_DTAP, 1},
                                {S_XMOD, 1}};

/* who a message is from, for its banner's color (lk_msg_col in look.c): the drones (K_DRONE), the LFO (K_LFO), the
 * scenes, Presets, saves and the Swell (K_GOLD); 0 the system, neutral. The old screen's palette went with it (0.3's
 * new look, PR H); K_BG is the start's animation's black, the site's */
#define K_BG RGB(20, 0, 15)
#define K_LFO RGB(220, 188, 255)
#define K_GOLD RGB(255, 214, 240)
static const uint16_t K_DRONE[DR_NGROUPS] = {RGB(255, 176, 74), RGB(69, 227, 211)};

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
    uint32_t chg_t;                    /* the last change to the project, by a hand or not (a scene gliding, the loop) */
    uint32_t saved_t;                  /* the last autosave tried */
    int8_t save_q;                     /* a save waiting for the dip (something heard): -2 none, -1 the sound, -3 the
                                        * autosave, N scene N */
    uint32_t save_t;                   /* ... asked then */
    uint8_t page_last[8];              /* each page button's page last shown (0: its first): opened again there */
    uint32_t sel_t;                    /* when SEL went down (held 0.4 s: the black keys' map) */
    int8_t km_key;                     /* the black key touched while SEL is held (named, not switched), or -1 */
    uint8_t jack;                      /* the headphone jack as last read (1: in), and since when */
    uint32_t jack_t;
    uint32_t rst_t;                    /* when the LFO reset key went down (a hold locks the LFO to the pulse) */
    uint8_t rst_done;                  /* that hold has acted */
    char msg[2][24];
    uint32_t msg_until;
    uint16_t msg_col;                  /* the banner's colour */
    uint8_t msg_tip;                   /* the message is a one-time tip (drawn apart from a message in the new look) */
    char glab[2][16];                  /* SELECT / ALGORITHM / PRESETS turned: their name and value (top bar) */
    uint32_t glab_until;
    uint32_t touch_t[4];               /* each knob: lit until then (each its own timer: two hands) */
    uint8_t sel_lit;                   /* bit i: knob i last turned with SEL held, on a page where that's a second job */
    uint8_t sel_turned;                /* a second job turned since SEL went down (the key map stays away) */
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
    float loop_g0[DR_NGEST];           /* each gesture's level then */
    uint8_t loop_scenes;               /* the loop moves scenes: each pass starts from the whole sound it began on */
    uint32_t morph_t0, morph_ms;
    int16_t morph_from[DR_NPARAMS];
    int16_t morph_to[DR_NPARAMS];      /* the scene being morphed to, as it was when its key was pressed */
    int8_t press;                      /* the scene key held in a pressure crossfade (v0.2), or -1 */
    uint8_t press_done;                /* ... pressed through: the scene is taken */
    int16_t undo[DR_NPARAMS];          /* UNDO (SEL + SAVE, v0.3): the sound before the last scene tap or blend */
    int8_t undo_ok, undo_at;           /* ... there is one; the scene it was (-1: a sound of its own) */
    uint16_t tip_q;                    /* the one-time tips waiting for the screen (bit TIP_*) */
    char msg_next[24];                 /* a message's second part, shown after it (too long for one) */
    uint32_t press_t0, press_hi_t;     /* ... since; above the commit level since (0: not) */
    uint32_t press_ft;                 /* ... the last frame's time (its glide toward the pressure) */
    float press_x;                     /* ... how far into the scene, 0..1 */
    uint32_t flash_t;                  /* a scene taken: its key flashes from then */
    uint8_t g_held;                    /* the gesture keys held: bit g (Weight, Shift, Clean, Wash) */
    uint8_t g_rel;                     /* ... pressed to release a latch: the let-go does nothing more */
    uint32_t g_t0[DR_NGEST];           /* ... since */
    float g_out[DR_NGEST];             /* ... let go after a hold: how far in it was, and over how long (s, at Gesture
                                        * time x1) it goes: as long as it was held (Evan, 2026-10-18); 0: the swell */
    float g_out_s[DR_NGEST];
    float g_amt[DR_NGEST];             /* ... how far in each is, 0..1 */
    int16_t g_sent[DR_NGEST];          /* ... as the engine has it, 0..1000 */
    uint32_t g_last;                   /* the last swell step's time */
    uint64_t hands[(DR_NPARAMS + 63) / 64];   /* the values a hand has moved since a scene switch began: the
                                               * switch lets go of them (Evan, 2026-10-17: Movement fought it) */
    int16_t press_to[DR_NPARAMS];      /* ... the scene */
    uint32_t frame;
    uint32_t sig[3];                   /* what each band shows: redrawn when it changes */
    uint32_t splash_t0;                /* the start's animation: since */
    uint8_t splash_done;
} ui;

/* a message: a banner across the top of the picture for a moment (the knobs keep the bottom) */
static void say_c(const char *a, const char *b, uint16_t col);
static void say(const char *a, const char *b) { say_c(a, b, 0); }
static void sp_msg(const char *a, const char *b);
static void say_c(const char *a, const char *b, uint16_t col)
{
    ui.msg_col = col;
    ui.msg_tip = 0;
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
    ui.msg_next[0] = 0;                                /* a second part waiting belonged to the last one */
    sp_msg(a, b);                                      /* and to a screen reader (below) */
}
void ui_say(const char *a, const char *b) { say(a, b); }

static void mark_dirty(void) { ui.dirty = 1; ui.chg_t = plat_ms(); }

/* ONE-TIME TIPS (v0.3): the first time a player does a thing, a line on what's next to it, once (the project keeps
 * what's been seen, the player's, as the Gesture time is). A tip waits for the screen: after any message */
enum { TIP_BLEND, TIP_NAMES, TIP_GEST, TIP_FX, TIP_INTO, TIP_XMOD, TIP_CLOCK, TIP_UNDO, TIP_KN_LIFE, TIP_KN_MODS,
       TIP_KN_SETUP2, TIP_KEEP, TIP_KN_DRONES, TIP_KN_SPACE, NTIPS };
static const char *const TIP_TEXT[NTIPS] = {   /* one line, the banner's width (~23) */
    "PRESS HARDER: BLEND", "HOLD [SEL]: KEY NAMES", "HOLD: IN · TAP: LATCH", "TAP [FX]: TIME, TYPES",
    "[SEL]+KNOB 4: INTO REV", "[F#5]: PITCH, SNARL", "SWELL KNOB 1: DIVISION", "[SEL] + [SAVE]: UNDO",
    /* the knobs 0.3 moved (Evan, 2026-11-01), the first time a player from 0.2 opens their page (SEEN1_MOVED) */
    "FEEDBACK NOW KNOB 1", "X-MOD NOW KNOB 4", "TUNE NOW KNOB 2",
    "TAP [SEL]: KEEP IT",                      /* SEL glowing while a blend or a gesture can be kept */
    "PITCH: TAP [HOME] AGAIN", "DRIVE, TIME: TAP [FX]"};   /* the pages 0.3 rearranged (2026-11-08), as the knobs */
/* (a button or key in [ ]: the new look draws it in a box, the spec's rule 19; the old screen drops the brackets) */
#define TIP_MS 3500u
static int tip_seen(int t) { return proj.seen[t >> 3] >> (t & 7) & 1u; }
static void tip(int t)
{
    if (!tip_seen(t))
        ui.tip_q |= (uint16_t)(1u << t);
}
static void tips_run(void)                             /* each frame: a tip waiting, the screen free */
{
    int t;
    if (plat_ms() < ui.msg_until + (ui.msg_next[0] ? 0u : 300u))
        return;
    if (ui.msg_next[0]) {                              /* a message's second part, just after it */
        char t[sizeof ui.msg_next];
        memcpy(t, ui.msg_next, sizeof t);
        say_c(t, 0, ui.msg_col);
        return;
    }
    if (!ui.tip_q)
        return;
    for (t = 0; !(ui.tip_q >> t & 1u); t++)
        ;
    ui.tip_q &= (uint16_t)~(1u << t);
    if (tip_seen(t))
        return;
    proj.seen[t >> 3] |= (uint8_t)(1u << (t & 7));   /* saved with the next save (no write of its own) */
    say_c(TIP_TEXT[t], 0, K_GOLD);
    ui.msg_tip = 1;
    ui.msg_until = plat_ms() + TIP_MS;
}
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
/* SEL's fine tuning set on one Tuning, kept while a scene's blend takes the sound to another, so a blend that comes
 * back (a crossfade let go under half way, SEL + SAVE's undo) brings it back too; -1: none (the knob or SEL moved since)
 * (the review of #82) */
static int tun_keep_t = -1;
static int8_t tun_keep[3];

static void param_set(int p, int v)
{
    const dr_param_t *i = dr_param_info(p);
    v = v < i->lo ? i->lo : v > i->hi ? i->hi : v;
    if (v == proj.par[p])
        return;
    proj.par[p] = (int16_t)v;
    dr_set(p, v);
    if (p == P_TUNING)                                 /* a new Tuning, by the knob or with a sound: SEL's fine tuning
                                                        * let go, the knobs land on its own notes (Evan, 2026-11-07: an
                                                        * offset set against one Tuning's notes means nothing on
                                                        * another's) */
        memset(proj.tun_off, 0, sizeof proj.tun_off);
    mark_dirty();
}

static void loop_note(int p, int v, uint32_t slide);  /* the motion loop notes knob moves (below) */

/* the parameter knob i moves on the page shown, or -1 (on the Drones page, KNOB 3 is the link offset while
 * PITCH LINK is on) */
static int knob_own(int i)
{
    knob_t k = VIEW_KNOB[ui.view][i];
    if (k.g == NONE)
        return -1;
    if (k.g == G && dr_ext_spb > 0.0f)  /* on a MIDI clock (v0.3): the Swell's Rate its division, the delay's Time a note
                                         * value, the Scene time bars */
        return k.p == P_PULSE_RATE ? P_PULSE_DIV : k.p == P_DLY_TIME ? P_DLY_NOTE : k.p == P_SCENE_T ? P_SCENE_BARS : k.p;
    if (k.g == G)
        return k.p;
    if (k.g == 1 && k.p == GP_PITCH && proj.par[P_PLINK])
        return P_PLINK_OFF;
    return DR_GP(k.g, k.p);
}
static int knob_param(int i)                           /* ... or, SEL held, its second job */
{
    return (ui.btn >> B_SEL & 1u) && sel_job(ui.view, i) >= 0 ? sel_job(ui.view, i) : knob_own(i);
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

/* a pitch knob (or the link offset): a detent is the next step of the Tuning, up or down (2 when turned quickly, on a
 * smooth curve: 70 detents a second; Evan, on the FM-1, 4x went too far); with SEL held, 1 cent.
 * Each step slides over the time since the last detent (15..600 ms): the next one is due about then, so a steady turn
 * is one slide through the grid's notes, as an analog knob's; a first detent after a pause slides over 300 ms. */
/* THE TUNING (v0.3, Setup 2 KNOB 4; Evan, 2026-10-30): every detent moves the pitch to the grid's next step, so the
 * screen shows the note it goes to (#61 glided 60 cents a detent and landed once stopped: on the uneven grids a detent
 * could land back on the same note, and the card showed the cents in between). A pitch SEL has fine-tuned off the
 * grid keeps that offset: +3 cents over a semitone steps to +3 over the next. A pitch off the grid (an old sound, a
 * note) steps to the grid's next step beyond it.
 * The fixed grids are built on A (Tune moves it: -31.8 cents, A = 432 Hz); on a movable one (Just, Shruti: a movable
 * do, Evan 2026-10-30) drone I's knob picks the tonic in semitones and drone II's steps through the Tuning over drone I.
 * Pitch link's offset steps on the grid's intervals. The tables: drone.c */
static int tun_t(int p)                                /* the grid a pitch knob steps on: drone I's on a movable
                                                        * Tuning (Just, Shruti) picks the tonic, in semitones */
{
    int t = proj.par[P_TUNING];
    return DR_TUN_MOVES(t) && p == DR_GP(0, GP_PITCH) ? DR_TUN_12 : t;
}
static int tun_root(int p)                             /* where its grid starts: A; an interval from 0 (the link
                                                        * offset); drone I's pitch, for drone II on a movable Tuning */
{
    return p == P_PLINK_OFF ? 0 : p == DR_GP(1, GP_PITCH) && DR_TUN_MOVES(proj.par[P_TUNING]) ?
           proj.par[DR_GP(0, GP_PITCH)] : 900;
}
static int grid_k(int p, int v) { return dr_grid_k(tun_t(p), v, tun_root(p)); }
static int grid_at(int p, int k) { return dr_grid_at(tun_t(p), k, tun_root(p)); }
static int grid_near(int p, int v) { return grid_at(p, grid_k(p, v)); }
static int tun_i(int p) { return p == P_PLINK_OFF ? 2 : p == DR_GP(1, GP_PITCH); }   /* its proj.tun_off */
static void pitch_turn(int g, int p, int role, int32_t s)
{
    uint32_t now = plat_ms(), dt = now - ui.enc_t[role], a = (uint32_t)(s < 0 ? -s : s), slide;
    int cents, ti = tun_i(p);
    ui.enc_t[role] = now;
    if (a > 1)
        dt /= a;                                       /* several detents since the last look */
    /* the glide: the time since the last detent (when the next is due, so a steady turn never stops),
     * 15 .. 600 ms; after a pause (a first detent), 300 ms */
    slide = accel_off ? 15u : dt < 15u ? 15u : dt > 600u ? 300u : dt;
    if (ui.btn >> B_SEL & 1u) {                        /* fine: SEL held; where it ends is the offset kept */
        int v = proj.par[p] + (int)s;
        ui.btn_used |= 1u << B_SEL;
        cents = (int)s;
        v -= grid_near(p, v);
        proj.tun_off[ti] = (int8_t)(v < -127 ? -127 : v > 127 ? 127 : v);
        tun_keep_t = -1;                               /* a new offset of the hand's: none kept to come back */
    } else {
        float sp = 1000.0f / (float)(dt ? dt : 1u), x = fm_clampf((sp - 10.0f) * (1.0f / 60.0f), 0.0f, 1.0f);
        const dr_param_t *in = dr_param_info(p);
        int n = (int)((float)a * (accel_off ? 1.0f : 1.0f + x * x) + 0.5f), off = proj.tun_off[ti],
            v = proj.par[p] - off, k = grid_k(p, v), to;
        if (s > 0)                                     /* the first step: the grid's next beyond where it is */
            k += (grid_at(p, k) <= v) + n - 1;
        else
            k -= (grid_at(p, k) >= v) + n - 1;
        while ((to = grid_at(p, k) + off) > in->hi && s > 0)   /* the range's ends: the last step inside */
            k--;
        while (to < in->lo && s < 0)
            to = grid_at(p, ++k) + off;
        if (s > 0 ? to <= proj.par[p] : to >= proj.par[p])
            return;                                    /* at the end */
        cents = to - proj.par[p];
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
    {   /* cutoff and resonance run 0..1000, Shift's move in cents: a detent is 10, with SEL held 1 (fine, as a
         * pitch's cent) */
        int gp = p < P_XMOD ? p % GP_N : -1;
        if (gp == GP_CUTOFF || gp == GP_RESO || p == P_SHIFT_MOVE || p == P_TUNE) {   /* Tune: 1 Hz, with SEL 0.1 */
            if (ui.btn >> B_SEL & 1u) {
                ui.btn_used |= 1u << B_SEL;
                steps = (int)s;
            } else {
                steps *= 10;
            }
        }
    }
    dr_pslide(p, (int)slide);
    if (p == P_SHIFT_MOVE && !(ui.btn >> B_SEL & 1u)) {   /* SHIFT along the Tuning's notes (Evan, 2026-11-07):
                                                          * a detent the next note over drone I, 2 at most when
                                                          * turned quickly (as the pitch knobs); SEL, a cent */
        const int lim = 2 * (s < 0 ? -(int)s : (int)s), n = steps / 10;
        param_set(p, DR_SHIFT_MID + dr_shift_step(proj.par[P_TUNING], proj.par[DR_GP(0, GP_PITCH)],
                                                  proj.par[p] - DR_SHIFT_MID, n > lim ? lim : n < -lim ? -lim : n));
    } else
        param_set(p, proj.par[p] + steps);
    loop_note(p, proj.par[p], slide);
}

/* an LFO depth turned that can't be heard: the Movement at 0 %, or the wave(s) it follows off. A hint then,
 * only then (Evan, 2026-10-17: nothing on the screen said why the depths did nothing) */
static void depth_hint(int p)
{
    int tap = -1, w1 = proj.par[P_LFO_W1] != 0, w2 = proj.par[P_LFO_W2] != 0, g;
    for (g = 0; g < DR_NGROUPS; g++) {
        if (p == DR_GP(g, GP_OLFO_DEPTH))
            tap = proj.par[DR_GP(g, GP_OLFO)];
        else if (p == DR_GP(g, GP_FLFO_DEPTH))
            tap = proj.par[DR_GP(g, GP_FLFO)];
    }
    if (p == P_DLY_LFO_DEPTH)
        tap = proj.par[P_DLY_LFO];
    if (tap < 0 || !proj.par[p])
        return;
    if (proj.par[P_MOVE] == 0)
        say_c("MOVEMENT 0%", "ALGORITHM", K_LFO);
    else if ((tap == DR_LFO_I && !w1) || (tap == DR_LFO_II && !w2) || (tap == DR_LFO_BOTH && !w1 && !w2))
        say_c(tap == DR_LFO_II ? "LFO II OFF" : "LFO I OFF", tap == DR_LFO_II ? "tap [OCT+]" : "tap [OCT-]", K_LFO);
}

/* A KNOB OVER A LIST OF NAMED CHOICES GOES ROUND (Evan, 2026-10-25: the rule for every such knob): past the last
 * comes the first, before the first the last, one choice a detent. Not a list ordered by size (Partner's interval,
 * like a pitch, has ends). A new knob of choices goes here */
static int knob_list(int p)
{
    return p == P_DLY_TYPE || p == P_REV_TYPE || p == P_DLY_ROUTE || p == P_WSCALE || p == P_SUBW_0 || p == P_SUBW_1 ||
           p == P_TUNING || p == P_LFO_RMOD_SRC ||   /* Mods' Source: I, II, I+II, I again (Evan, 2026-11-03) */
           p == P_CLK_SYNC;                    /* Sync: On, Off, On */
}

static void knob_turn(int i, int32_t e)
{
    int p = knob_param(i);
    knob_t k = VIEW_KNOB[ui.view][i];
    if (p < 0)
        return;
    if (sel_job(ui.view, i) >= 0) {                  /* which half of the knob's card lights */
        if (p == sel_job(ui.view, i))
            ui.sel_lit |= (uint8_t)(1u << i);
        else
            ui.sel_lit &= (uint8_t)~(1u << i);
        if (p == sel_job(ui.view, i)) {
            ui.btn_used |= 1u << B_SEL;
            ui.sel_turned = 1;
        }               /* SEL let go: not a tap */
    }
    if (proj.par[P_II_OUT] && k.g == 1)        /* drone II out: say how it comes back, on any page (Evan, 2026-10-18) */
        say_c("II OFF", "turn on [SEL] + [PLAY]", K_DRONE[1]);   /* (Evan, 2026-11-02: say what to do) */
    if (k.p == GP_PITCH && k.g < G)
        pitch_turn(k.g, p, EN_K1 + i, e);
    else if (dr_param_info(p)->hi - dr_param_info(p)->lo > 24)
        cont_turn(p, EN_K1 + i, e);
    else if (knob_list(p)) {                           /* a list of choices goes round both ways, as the LFO waves
                                                        * do (Evan, 2026-10-25) */
        const dr_param_t *in = dr_param_info(p);
        int n = in->hi - in->lo + 1, v = (proj.par[p] - in->lo + (e > 0 ? 1 : -1)) % n;
        param_set(p, in->lo + (v < 0 ? v + n : v));
        loop_note(p, proj.par[p], 0);
        if (p == P_TUNING)
            tun_keep_t = -1;                           /* the hand chose it: nothing kept comes back */
    } else {
        param_set(p, proj.par[p] + accel(EN_K1 + i, e, dr_param_info(p)->hi - dr_param_info(p)->lo));
        loop_note(p, proj.par[p], 0);
    }
    if (ui.view == V_LFO || ui.view == V_LFO2)
        depth_hint(p);
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
    case S_SUBOCT: return p[GP_SUBOCT] >= 5 ? 1 : p[GP_SUBOCT] >= 3 ? 2 : p[GP_SUBOCT] == 2;   /* -2, -3, -4 oct lit;
                                                                                               * a subharmonic winks */
    case S_PLOCK: return proj.par[P_PLOCK] != 0;
    case S_HP: return p[GP_MODE] == DR_HP ? 1 : p[GP_MODE] == DR_BP ? 2 : 0;   /* HP lit, BP winking */
    case S_OTAP: return tap_lit(p[GP_OLFO]);
    case S_ODEST: return p[GP_OLFO_DEST] == DR_TO_XWAVE;
    case S_FDEST: return p[GP_FLFO_DEST] == DR_TO_RESO;
    case S_FTAP: return tap_lit(p[GP_FLFO]);
    case S_PLINK: return proj.par[P_PLINK] != 0;
    case S_SYNC: return proj.par[P_SYNC] != 0;
    case S_XMOD: return proj.par[P_XMOD] >= DR_XMOD_FM ? 2 : proj.par[P_XMOD] != 0;   /* cutoff lit; pitch, Snarl winking */
    case S_DTAP: return tap_lit(proj.par[P_DLY_LFO]);
    default: return 0;
    }
}
/* how a black key lights (the lights review, Evan 2026-11-07: dark always means off): 0 dark, 1 dim (the first of
 * two or three live choices: -1 oct, LP, Pitch, Cutoff, LFO I), 2 lit (on, or the second choice), 3 winking (a third).
 * A plain switch (Partner, Pitch lock, the links, hard sync) is dark off, lit on */
static int sw_look(sw_t w)
{
    const int16_t *p = &proj.par[DR_GP(w.g, 0)];
    int v;
    switch (w.s) {
    case S_SUBOCT: return p[GP_SUBOCT] == 3 || p[GP_SUBOCT] == 4 ? 3 : p[GP_SUBOCT] <= 1 ? 1 : 2;
    case S_HP: return p[GP_MODE] == DR_HP ? 2 : p[GP_MODE] == DR_BP ? 3 : 1;
    case S_ODEST: case S_FDEST: return sw_get(w) ? 2 : 1;
    case S_OTAP: case S_FTAP: case S_DTAP:
        v = (w.s == S_DTAP ? proj.par[P_DLY_LFO] : p[w.s == S_OTAP ? GP_OLFO : GP_FLFO]) & 3;
        return v == DR_LFO_BOTH ? 3 : v == DR_LFO_II ? 2 : v ? 1 : 0;
    case S_LFORESET: return proj.par[P_LFO_LOCK] ? 2 : 0;
    default: v = sw_get(w); return v == 2 ? 3 : v ? 2 : 0;
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

static void sw_flip(sw_t w)
{
    static const char *const DR[2] = {"I ", "II "};
    static const char *const WAVES[4] = {"NO LFO", "LFO I", "LFO II", "LFO I+II"};
    char a[24];
    const char *what = 0, *st = 0, *next = 0;
    int on = !sw_get(w), base = DR_GP(w.g, 0), tp = -1, nx;
    /* a route's key goes round: Wave I, Wave II, both (I+II), Wave I */
    if (w.s == S_OTAP || w.s == S_FTAP || w.s == S_DTAP)
        tp = w.s == S_OTAP ? base + GP_OLFO : w.s == S_FTAP ? base + GP_FLFO : P_DLY_LFO;
    nx = tp < 0 ? 0 : proj.par[tp] == DR_LFO_I ? DR_LFO_II : proj.par[tp] == DR_LFO_II ? DR_LFO_BOTH : DR_LFO_I;
    char *q = scopy(a, DR[w.g]);
    switch (w.s) {
    case S_PARTNER: param_set(base + GP_PARTNER, on); what = "PARTNER"; st = on ? "ON" : "OFF"; break;
    case S_SUBOCT: {                           /* round: -1 oct, -2 oct, 1/3, 1/5, -3 oct, -4 oct (v0.3: clicks) */
        static const char *const SUB[7] = {"", "-1 OCT", "-2 OCT", "1/3", "1/5", "-3 OCT", "-4 OCT"};
        int v = proj.par[base + GP_SUBOCT] % 6 + 1;
        param_set(base + GP_SUBOCT, v);
        what = "SUB";
        st = SUB[v];
        if (proj.par[base + GP_MIX] <= 2)      /* not heard: OSC MIX (Sub · Drive, EDIT twice) is at the oscillators */
            next = "OSC MIX 0 · [EDIT] x2";
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
    /* the LFO pages: the wave, then what it moves, as the picture reads ("LFO II -> I PITCH"; Evan, 2026-11-03) */
    case S_OTAP: param_set(tp, nx); what = proj.par[base + GP_OLFO_DEST] == DR_TO_XWAVE ? "X-WAVE" : "PITCH";
        st = WAVES[nx]; break;
    case S_ODEST: param_set(base + GP_OLFO_DEST, on); what = on ? "X-WAVE" : "PITCH"; st = WAVES[proj.par[base + GP_OLFO]];
        break;
    case S_FDEST: param_set(base + GP_FLFO_DEST, on); what = on ? "RESONANCE" : "CUTOFF"; st = WAVES[proj.par[base + GP_FLFO]];
        break;
    case S_FTAP: param_set(tp, nx); what = proj.par[base + GP_FLFO_DEST] == DR_TO_RESO ? "RESONANCE" : "CUTOFF";
        st = WAVES[nx]; break;
    case S_PLINK: link_set(on); q = a; what = "PITCH LINK"; st = on ? "ON" : "OFF"; break;
    case S_SYNC: param_set(P_SYNC, on); q = a; what = "HARD SYNC"; st = on ? "ON" : "OFF"; break;
    case S_XMOD: {                             /* round: off, cutoff, pitch (v0.3: cross-FM), Snarl (v0.3) */
        int m = (proj.par[P_XMOD] + 1) % (DR_XMOD_OUT + 1);
        param_set(P_XMOD, m);
        q = a;
        what = "CROSS-MOD";
        st = m == DR_XMOD_OUT ? "SNARL: OUT > I II" : m == DR_XMOD_FM ? "PITCH I <> II" : m ? "I > II CUTOFF" : "OFF";
        if (m == 1)
            tip(TIP_XMOD);
        break;
    }
    case S_LFORESET: dr_lfo_reset(); q = a; what = "LFO"; st = "RESET"; break;
    case S_DTAP: param_set(tp, nx); q = a; what = "DELAY"; st = WAVES[nx]; break;
    default: return;
    }
    if ((w.s == S_PARTNER && ui.view == V_PARTNER) || (w.s == S_SUBOCT && ui.view == V_SOURCE) ||
        (w.s == S_HP && ui.view == V_PLAY) || (w.s == S_XMOD && ui.view == V_LFO2) ||
        (ui.view == V_LFO && (w.s == S_OTAP || w.s == S_ODEST || w.s == S_FTAP || w.s == S_FDEST)) ||
        (w.s == S_DTAP && ui.view == V_LFO2))
        return;                                /* the page shows it and its key: no banner (Evan, 2026-11-02; the LFO
                                                * pages' keys, 2026-11-04: the routes on LFO sends, the delay's LFO
                                                * and cross-mod on Mods; the rest keep theirs) */
    if (w.s == S_OTAP || w.s == S_ODEST || w.s == S_FDEST || w.s == S_FTAP || w.s == S_DTAP) {
        char b[24];
        scopy(scopy(scopy(b, "\\ "), q == a ? "" : DR[w.g]), what);   /* "\\": a right arrow (gen_font.py) */
        say_c(st, b, RGB(183, 145, 255));      /* in the LFO's lilac */
        if (next)
            scopy(ui.msg_next, next);
        return;
    }
    scopy(q, what);
    say_c(a, st, w.s == S_LFORESET || w.s == S_DTAP ? RGB(183, 145, 255) : w.g ? RGB(69, 227, 211) : RGB(255, 176, 74));
    if (next)
        scopy(ui.msg_next, next);
}

static void hands_on(int p);

/* a black key: what it changes is a hand's, as a turned knob's is, so a scene on its way leaves it alone (Evan,
 * 2026-10-25: during a switch the Sub key showed its next step but held until the scene landed) */
static void sw_toggle(sw_t w)
{
    static int16_t was[DR_NPARAMS];
    int k;
    memcpy(was, proj.par, sizeof was);
    sw_flip(w);
    tip(TIP_NAMES);
    for (k = 0; k < DR_NPARAMS; k++)
        if (proj.par[k] != was[k])
            hands_on(k);
}

/* ------------------------------------------------------------ buttons --- */
static int on_pages(int i) { return ui.view >= PAGE_BTN[i].first && ui.view < PAGE_BTN[i].first + PAGE_BTN[i].n; }
static void set_view(int v)
{
    uint32_t i;
    ui.view = (uint8_t)v;
    for (i = 0; i < 4; i++)
        ui.touch_t[i] = 0;
    if (v == V_PLAY)
        tip(TIP_KN_DRONES);
    else if (v == V_FX) {
        tip(TIP_KN_SPACE);
        tip(TIP_FX);
    }
    else if (v == V_FX2)
        tip(TIP_INTO);
    else if (v == V_LIFE)
        tip(TIP_KN_LIFE);
    else if (v == V_LFO2)
        tip(TIP_KN_MODS);
    else if (v == V_SETUP2)
        tip(TIP_KN_SETUP2);
    for (i = 0; i < NPAGE_BTN; i++)                    /* remembered: the button opens it next time */
        if (on_pages((int)i))
            ui.page_last[i] = (uint8_t)(v - PAGE_BTN[i].first);
}

static int page_btn(int b)                             /* the index in PAGE_BTN, or -1 */
{
    uint32_t i;
    for (i = 0; i < NPAGE_BTN; i++)
        if (PAGE_BTN[i].btn == b)
            return (int)i;
    return -1;
}

/* a page button pressed: the page of its last shown at once (the review: Reverb, Life, Moves couldn't be peeked;
 * HOME always Drones and FX always Space, the pages played: v0.3), unless already on its pages (then the next on a
 * tap) */
static void page_down(int b)
{
    int i = page_btn(b);
    ui.hold_btn = (int8_t)b;
    ui.hold_t = plat_ms();
    ui.hold_turned = 0;
    ui.hold_from = ui.view;
    ui.hold_cycle = (uint8_t)on_pages(i);
    if (!ui.hold_cycle)
        set_view(PAGE_BTN[i].first + (b == B_HOME || b == B_FX ? 0 : ui.page_last[i]));   /* HOME and FX: the page played */
}
/* let go: after a hold (or a knob turned while held), back where it was; after a tap, stay (or step on) */
static void page_up(int b)
{
    int i = page_btn(b), peek;
    if (ui.hold_btn != b)
        return;
    peek = ui.hold_turned || plat_ms() - ui.hold_t >= HOLD_MS;
    if (b == B_ARP && !peek && ui.hold_cycle) {        /* ARP, as the other page buttons (Evan, 2026-10-24): a tap
                                                        * opens the Swell page (landing never switches it), a tap
                                                        * on it is the Swell on / off, a hold peeks */
        param_set(P_PULSE, !proj.par[P_PULSE]);
        say("SWELL", proj.par[P_PULSE] ? "ON" : "OFF");
    }
    if (peek) {
        if (!ui.hold_cycle)
            set_view(ui.hold_from);
    } else if (ui.hold_cycle) {
        set_view(PAGE_BTN[i].first + (ui.view - PAGE_BTN[i].first + 1) % PAGE_BTN[i].n);
    }
    ui.hold_btn = -1;
}

static float morph_ms(void) { return 1000.0f * dr_time_s(P_MORPH, proj.par[P_MORPH]); }

/* OCT- / OCT+: the LFO's two waves (v0.2): a tap is the next shape, round and round (Off, the seven, random,
 * AND, OR);
 * SEL + OCT the one before. The engine glides a new shape in over the Switch time. Shown in the top bar */
static void hands_on(int p) { ui.hands[p / 64] |= (uint64_t)1 << (p % 64); }
static int hands_has(int p) { return (int)(ui.hands[p / 64] >> (p % 64) & 1u); }
static void hands_clear(void) { memset(ui.hands, 0, sizeof ui.hands); }

static void wave_step(int w, int dir)
{
    char t[12];
    const int p = w ? P_LFO_W2 : P_LFO_W1, n = DR_LFO_NSHAPES;
    int v = (proj.par[p] + dir + n) % n;
    hands_on(p);                               /* a scene on its way leaves it alone */
    param_set(p, v);
    ui.scene_at = -1;
    loop_note(p, v, 0);
    if (ui.view == V_LFO)                      /* LFO sends draws both waves, named: no need to say it (Evan) */
        return;
    dr_param_text(p, v, t);
    scopy(ui.glab[0], w ? "LFO II" : "LFO I");
    scopy(ui.glab[1], t);
    ui.glab_until = plat_ms() + 1500u;
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
/* Beyond the parameters, the loop records what the hands do that isn't a value (Evan, 2026-10-28: a loop of gestures
 * was "nothing moved"): each gesture's level as it went (LP_GEST + g, 0..1000; its latch is a value, P_GL_*), a scene
 * tapped (LP_SCENE: bank << 4 | key), a scene blended by pressure (LP_BLEND: its key; then LP_BLEND_X, how far in,
 * 0..1000; LP_BLEND_END let go there, the blend a sound of its own from then on) */
enum { LP_GEST = DR_NPARAMS, LP_SCENE = LP_GEST + DR_NGEST, LP_BLEND, LP_BLEND_X, LP_BLEND_END, LP_N };
_Static_assert(LP_N <= 256, "the events' parameter is a byte");
static uint64_t loop_used[(LP_N + 63) / 64];           /* the parameters the loop moves */
/* playing: a gesture's level and the blend's depth glide between events, as a knob's slide */
typedef struct {
    float from, to;
    uint32_t t0, ms;
} loop_glide_t;
static loop_glide_t loop_gl[DR_NGEST + 1];             /* the gestures, then the blend */
static int16_t loop_bl_from[DR_NPARAMS], loop_bl_to[DR_NPARAMS];   /* playing a blend: the sound it began on, the scene */
static uint8_t loop_bl_on;
static void scene_play(int bank, int n);               /* (scenes, below) */
static void sound_blend(const int16_t *from, const int16_t *to, float x);
static int scene_room(int p);

/* a knob moved a value: noted while recording. Detents of the same knob within LOOP_JOIN of an event (a quick
 * turn: a detent every ~15 ms) join it, its slide reaching on to this one, so the 4096 events last (a busy
 * two-handed recording filled them in 1-2 minutes; the loop then held still) */
#define LOOP_JOIN 20u                  /* 100 ms, in the events' 5 ms units */
#define LOOP_JOIN_SLOW 40u             /* ... a gesture's level, a blend's depth: 200 ms (they move over seconds; the review:
                                        * four moving with the knobs filled the events in minutes) */
static void loop_note(int p, int v, uint32_t slide)
{
    loop_ev_t *e;
    uint16_t t = (uint16_t)((plat_ms() - ui.loop_t0) / 5u);
    int i;
    if (ui.loop != L_REC || ui.loop_n >= LOOP_MAX)
        return;
    if (p == LP_SCENE || p == LP_BLEND)
        ui.loop_scenes = 1;
    for (i = p == LP_SCENE || p == LP_BLEND || p == LP_BLEND_END ? -1 : ui.loop_n - 1; i >= 0 && i >= ui.loop_n - 8; i--) {
        e = &loop_ev[i];
        if (t - e->t >= (p >= LP_GEST ? LOOP_JOIN_SLOW : LOOP_JOIN))
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

static float loop_glide_now(const loop_glide_t *g)
{
    uint32_t dt = plat_ms() - g->t0;
    return dt >= g->ms ? g->to : g->from + (g->to - g->from) * (float)dt / (float)g->ms;
}
static float loop_gest(int g) { return ui.loop == L_PLAY && loop_moves(LP_GEST + g) ? loop_glide_now(&loop_gl[g]) : 0.0f; }
static void loop_glide(loop_glide_t *g, float to, uint32_t ms)
{
    g->from = loop_glide_now(g);
    g->to = to;
    g->t0 = plat_ms();
    g->ms = ms;
}

static void loop_apply(int p, int v, uint32_t slide)
{
    if (p >= LP_GEST && p < LP_SCENE) {
        loop_glide(&loop_gl[p - LP_GEST], (float)v * 0.001f, slide);
        return;
    }
    if (p == LP_SCENE) {
        loop_bl_on = 0;
        scene_play(v >> 4, v & 15);
        return;
    }
    if (p == LP_BLEND) {                                /* a blend begins: from the sound as it is now */
        const int16_t *sc = scene_get(v >> 4, v & 15);
        if (!sc)
            return;
        memcpy(loop_bl_from, proj.par, sizeof loop_bl_from);
        memcpy(loop_bl_to, sc, sizeof loop_bl_to);
        ui.morph = -1;
        loop_gl[DR_NGEST].to = loop_gl[DR_NGEST].from = 0.0f;
        loop_bl_on = 1;
        return;
    }
    if (p == LP_BLEND_X) {
        loop_glide(&loop_gl[DR_NGEST], (float)v * 0.001f, slide);
        return;
    }
    if (p == LP_BLEND_END) {                            /* let go: there, then the loop lets the sound be (a recorded
                                                         * knob on the scene's values isn't overwritten after) */
        if (loop_bl_on)
            sound_blend(loop_bl_from, loop_bl_to, loop_gl[DR_NGEST].to);
        loop_bl_on = 0;
        return;
    }
    if (p == DR_GP(0, GP_PITCH) || p == DR_GP(1, GP_PITCH))
        dr_slide(p / GP_N, (int)slide);
    else if (p == P_PLINK_OFF)
        dr_slide(1, (int)slide);
    else if (slide)
        dr_pslide(p, (int)slide);
    param_set(p, v);
}

static void loop_rewind(void)                  /* the start of a pass: what the loop moves, back to how it */
{                                              /* began, gliding over the Switch time (no seam) */
    int p;
    uint32_t back = (uint32_t)morph_ms();
    if (back < 15u)
        back = 15u;
    if (ui.loop_scenes) {                              /* scenes in it: the whole sound back, as it began */
        loop_bl_on = 0;
        ui.morph = -1;
        for (p = 0; p < DR_NPARAMS; p++)
            if (!scene_room(p) && proj.par[p] != ui.loop_from[p])
                loop_apply(p, ui.loop_from[p], back);
    } else {
        for (p = 0; p < DR_NPARAMS; p++)
            if (loop_moves(p))
                loop_apply(p, ui.loop_from[p], back);
    }
    for (p = 0; p < DR_NGEST; p++)
        if (loop_moves(LP_GEST + p))
            loop_glide(&loop_gl[p], ui.loop_g0[p], back);
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
        for (p = 0; p < DR_NGEST; p++) {
            ui.loop_g0[p] = ui.g_amt[p];
            loop_gl[p].from = loop_gl[p].to = ui.g_amt[p];   /* not the last loop's (the review) */
            loop_gl[p].ms = 0;
        }
        loop_gl[DR_NGEST].from = loop_gl[DR_NGEST].to = 0.0f;
        loop_bl_on = 0;
        for (p = 0; p < (int)(sizeof loop_used / sizeof loop_used[0]); p++)
            loop_used[p] = 0;
        ui.loop_scenes = 0;
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
        for (p = 0; p < DR_NGEST; p++)         /* stopped: everything stays where it is, the gestures too (each */
            ui.g_amt[p] = fm_maxf(ui.g_amt[p], loop_gest(p));   /* then goes its own way, from there) */
        loop_bl_on = 0;
        ui.loop = L_IDLE;
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
    if (loop_bl_on && ui.press < 0)                     /* a blend playing (a hand on a scene key takes over) */
        sound_blend(loop_bl_from, loop_bl_to, loop_glide_now(&loop_gl[DR_NGEST]));
}

/* ------------------------------------------------------------- scenes --- */
static int scene_step(int p)                           /* a value that steps (a switch, a list): it flips; the
                                                        * Tuning's eleven too (never through the ones between) */
{
    const dr_param_t *in = dr_param_info(p);
    return in->hi - in->lo <= 9 || p == P_TUNING;
}

/* settings of the room, not the sound: a scene doesn't carry them (the speaker and the stereo spread, which
 * depend on speaker or headphones; the tuning to the band; the pitch lock; how long things take). The Tuning is
 * a sound's (v0.3, ledger 3, Evan 2026-11-08): a scene or Preset brings its own, a Preset that names none 12-tone,
 * as it was made, with Pitch lock on too (it keeps the drones' pitch, not their grid) */
static int scene_room(int p)
{
    return p == P_FADE || p == P_TUNE || p == P_SPEAKER || p == P_WIDTH || p == P_PLOCK || p == P_MORPH ||
           p == P_SCENE_T || p == P_PRESS_SENS || p == P_SCENE_BARS || p == P_CLK_SYNC;
}

/* a scene's name: its bank's letter and its number ("B5"; the Presets "P2", their second bank "P9" ..) */
static void scene_name(char *t, int bank, int n)
{
    t[0] = BANK_L[(unsigned)bank < NBANKS ? bank : 0];
    itoa_u((uint32_t)n + 1u + (bank == BANK_PRESETS2 ? NPLAY : 0u), t + 1);
}

#define SAVE_AUTO (-3)                                 /* save_now / save_ask: the autosave (no banner) */
/* into the flash now: n = -1 the sound (SAVE), SAVE_AUTO the autosave, else scene n (just stored) */
static void save_now(int n)
{
    char t[12] = "SCENE ";
    int ok;
    if (plat_store_held()) {                           /* a restore from USB is coming: it restarts after */
        say_c("NOT SAVED", "RESTORING FROM USB", K_GOLD);
        return;
    }
    ok = project_save() == 0;
    if (ok)
        ui.dirty = 0;
    if (n == SAVE_AUTO) {                              /* the autosave: no banner */
    } else if (n < 0) {
        say(ok ? "SAVED" : "NOT SAVED", ok ? 0 : "FLASH ERROR");
    } else {
        scene_name(t + 6, proj.bank, n);
        say(t, ok ? "STORED" : "NOT STORED");
    }
}

/* a save: while anything sounds (the drones, or the effects' tails ringing on after PLAY), the output dips out
 * first (the erase stops the audio for a few ms, measured on the FM-1: under one 5.8 ms block; cut in a tail it
 * popped, Evan 2026-11-07), then comes back; Evan, 2026-10-14: no more waiting for PLAY to stop */
#define TAIL_HEARD 1e-4f                               /* the effects' peak over -80 dB: a cut there is heard */
static void save_ask(int n)
{
    if (!ui.playing && dr_fade <= 0.0f && dr_tail < TAIL_HEARD) {
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
    if (BANK_IS_PRESETS(proj.bank)) {                  /* the Presets stay as they are (Evan, 2026-10-19) */
        say_c("PRESETS ARE FIXED", "[SEL]+[G4]-[G5]: BANK", K_GOLD);
        return;
    }
    scene_put(proj.bank, n);
    ui.scene_at = (int8_t)n;
    ui.dirty = 1;
    save_ask(n);
}

static float clk_bar_frac(void);
/* the Scene time, s. On a MIDI clock (v0.3): its bars (P_SCENE_BARS), ending on a bar line: the rest of this bar and
 * the bars after it (less than half a bar left of this one: a bar more) */
static float scene_s(void)
{
    if (dr_ext_spb > 0.0f) {
        float bars = (float)DR_BARS_N[(unsigned)proj.par[P_SCENE_BARS] < DR_NBARS ? proj.par[P_SCENE_BARS] : 2] -
                     clk_bar_frac();
        if (bars < 0.5f)
            bars += 1.0f;
        return bars * 4.0f * dr_ext_spb * (1.0f / DR_SR);
    }
    return dr_time_s(P_SCENE_T, proj.par[P_SCENE_T]);
}

#define VISIT_BACK NSCENES                     /* the morph's target: ui.visit_back (a visit's way back, an undo) */
/* UNDO (v0.3, SEL + SAVE): back to the sound before the last scene tap or blend, over the Scene time; again, forward.
 * Its first time replacing a sound of the player's own (no scene's), a tip says how */
static void undo_keep(void)
{
    memcpy(ui.undo, proj.par, sizeof ui.undo);
    ui.undo_ok = 1;
    ui.undo_at = ui.scene_at;
    if (ui.scene_at < 0)
        tip(TIP_UNDO);
}
static void scene_undo(void)
{
    int k;
    if (!ui.undo_ok) {
        say_c("NOTHING TO UNDO", 0, K_GOLD);
        return;
    }
    for (k = 0; k < DR_NPARAMS; k++) {
        int16_t v = ui.undo[k];
        ui.undo[k] = proj.par[k];                      /* undo again: forward to where it was */
        ui.morph_from[k] = proj.par[k];
        ui.visit_back[k] = v;
    }
    {
        int8_t at = ui.undo_at;
        ui.undo_at = ui.scene_at;
        ui.scene_at = at;
    }
    hands_clear();
    ui.press = -1;
    ui.visit = -1;
    ui.morph = VISIT_BACK;
    ui.morph_t0 = plat_ms();
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f);
    say_c("UNDO", "AGAIN: REDO", K_GOLD);
}

static void scene_recall(int n, int keep)              /* a white key: morph there over the Scene time; keep: the
                                                        * sound before it for undo (a press kept it already) */
{
    char t[12] = "SCENE ";
    const int16_t *sc = scene_get(proj.bank, n);
    int k;
    scene_name(t + 6, proj.bank, n);
    if (!sc) {
        say(t, "EMPTY: [SAVE]+KEY");
        return;
    }
    if (keep)
        undo_keep();
    for (k = 0; k < DR_NPARAMS; k++) {
        ui.morph_from[k] = proj.par[k];
        ui.morph_to[k] = sc[k];
    }
    hands_clear();                             /* a new switch: it moves everything again */
    ui.morph = (int8_t)n;
    ui.scene_at = (int8_t)n;
    ui.morph_t0 = plat_ms();
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f);
    /* (no banner: the header says where it's going, "→ P3 EERIE"; Evan, 2026-11-07) */
    loop_note(LP_SCENE, proj.bank << 4 | n, 0);
}

/* the motion loop playing a scene it recorded: there over the Scene time, as the tap did (no banner, no undo; the
 * bank it was in then) */
static void scene_play(int bank, int n)
{
    const int16_t *sc = scene_get(bank, n);
    int k;
    if (!sc)
        return;
    for (k = 0; k < DR_NPARAMS; k++) {
        ui.morph_from[k] = proj.par[k];
        ui.morph_to[k] = sc[k];
    }
    hands_clear();
    ui.morph = (int8_t)n;
    ui.scene_at = -1;
    ui.morph_t0 = plat_ms();
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f);
}

/* a visit let go: morph back to the sound before it, over the Scene time */
static void scene_back(void)
{
    int k;
    for (k = 0; k < DR_NPARAMS; k++)
        ui.morph_from[k] = proj.par[k];
    ui.morph = VISIT_BACK;
    ui.scene_at = ui.visit_from;
    ui.morph_t0 = plat_ms();
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f);   /* (the header says "→ A5") */
}

/* the sound moved x of the way from one set of values to another: smooth values glide, steps flip half way;
 * the room's settings stay, and with Pitch lock on the pitches */

static void sound_blend(const int16_t *from, const int16_t *to, float x)
{
    int k;
    for (k = 0; k < DR_NPARAMS; k++) {
        int v;
        if (scene_room(k) || hands_has(k))             /* the room's settings aren't a scene's; nor what a
                                                        * hand has moved since the switch began */
            continue;
        if (dr_ext_spb > 0.0f && (k == P_PULSE || k == P_PULSE_DIV))
            continue;                                  /* on a MIDI clock the Swell's on / off and its division stay as
                                                        * they are (Evan, 2026-10-26: a scene switched it off mid-song) */
        if (proj.par[P_PLOCK] && (k == DR_GP(0, GP_PITCH) || k == DR_GP(1, GP_PITCH) || k == P_PLINK_OFF ||
                                  k == P_GL_0 + DR_G_SHIFT))
            continue;                                  /* pitch lock: the drones keep their pitch (2026-10-17); Wander
                                                        * comes with the scene (Evan, 2026-11-08: a scene saved with
                                                        * Wander left it running through every scene after it) */
        if (scene_step(k))
            v = x >= 0.5f ? to[k] : from[k];
        else
            v = from[k] + (int)((float)(to[k] - from[k]) * x + (to[k] > from[k] ? 0.5f : -0.5f));
        if (v == proj.par[k])
            continue;
        if (k == DR_GP(0, GP_PITCH) || k == DR_GP(1, GP_PITCH))
            dr_slide(k / GP_N, 20);                    /* a frame's step of pitch, slid */
        else if (k == P_PLINK_OFF)
            dr_slide(1, 20);
        if (k == P_TUNING) {                           /* SEL's fine tuning: kept for its Tuning, back with it */
            const int back = v == tun_keep_t, was = proj.par[P_TUNING];
            int8_t off[3];
            memcpy(off, proj.tun_off, sizeof off);
            param_set(k, v);                           /* (lets it go) */
            if (back)
                memcpy(proj.tun_off, tun_keep, sizeof tun_keep);
            if (off[0] | off[1] | off[2]) {
                tun_keep_t = was;
                memcpy(tun_keep, off, sizeof tun_keep);
            } else if (back)
                tun_keep_t = -1;
            continue;
        }
        param_set(k, v);
    }
}

/* the morph, each frame: over its time, from where it started */
static void scene_morph(void)
{
    float x;
    if (ui.morph < 0)
        return;
    x = ui.morph_ms ? (float)(plat_ms() - ui.morph_t0) / (float)ui.morph_ms : 1.0f;
    if (x > 1.0f)
        x = 1.0f;
    sound_blend(ui.morph_from, ui.morph == VISIT_BACK ? ui.visit_back : ui.morph_to, x);
    if (x >= 1.0f)
        ui.morph = -1;
}

/* THE PRESSURE CROSSFADE (v0.2, option D): a scene key held is a fader into its scene. Under PR_DEAD nothing
 * moves; PR_FULL is all the way; held at PR_COMMIT for PR_COMMIT_MS the scene is taken (the key flashes);
 * a tap (let go under PR_TAP_MS, however hard) goes there to stay. The pressure is where the sound heads, not
 * where it is: it glides there no faster than the whole way in a Scene time, so a hard press glides in as a tap
 * does and a slow one follows the finger (Evan, 2026-10-19: a quick press to 100 was a jump). Let go: the blend
 * stays, where it was before the finger lifted, a sound of its own (v0.3; it went back unless SEL held it; SEL + SAVE
 * undoes it now). */
#define PR_DEAD 0.08f
#define PR_FULL 0.90f
#define PR_COMMIT 0.90f
#define PR_COMMIT_MS 300u
#define PR_TAP_MS 500u                 /* a scene key let go sooner: a tap, there to stay, as a gesture key (Evan,
                                        * 2026-10-18: 250 was too quick, a relaxed tap became a visit and went BACK) */
#define PR_WAIT_MS 250u                /* ... held this long, the pressure starts to move the sound (a crossfade) */
static void press_start(int n, const int16_t *sc)
{
    int k;
    float x0 = 0.0f;
    if (!(ui.morph == VISIT_BACK && ui.visit == -2 - n)) {  /* not already gliding back from this key: */
        for (k = 0; k < DR_NPARAMS; k++)                   /* where the blend starts from */
            ui.visit_back[k] = proj.par[k];
        hands_clear();
    } else if (ui.morph_ms) {                              /* on the way back: from where it is now */
        float p = fm_clampf((float)(plat_ms() - ui.morph_t0) / (float)ui.morph_ms, 0.0f, 1.0f);
        x0 = ui.press_x * (1.0f - p);
    }
    ui.visit_from = ui.scene_at;
    for (k = 0; k < DR_NPARAMS; k++)
        ui.press_to[k] = sc[k];
    ui.morph = -1;
    ui.press = (int8_t)n;
    ui.press_done = 0;
    ui.press_ft = plat_ms();
    ui.press_t0 = plat_ms();
    ui.press_hi_t = 0;
    ui.press_x = x0;
    loop_note(LP_BLEND, proj.bank << 4 | n, 0);
}

/* a key's pressure at the player's sensitivity (Setup 2, KNOB 3): a curve, light .. firm, 50 as tuned; a full press
 * is always all the way (QA 2026-10-20: as a gain, at 0 a full press went only 38 % in) */
static float press_gain(void) { return fm_exp2f(((float)proj.par[P_PRESS_SENS] - 50.0f) * 0.02f); }
static float press_curve(float x) { return x > 0.0f ? fm_exp2f(fm_log2f(x) / press_gain()) : 0.0f; }   /* 0..1 */
static int press_scaled(int p)
{
    return p < 0 ? p : (int)fm_clampf(127.0f * press_curve((float)p * (1.0f / 127.0f)) + 0.5f, 0.0f, 127.0f);
}

static char *press_label(char *t, int n)               /* "B5  40%": the scene, how far into it */
{
    char *q = t;
    t[0] = 0;
    scene_name(t, proj.bank, n);
    while (*q)
        q++;
    *q++ = ' ';
    *q++ = ' ';
    itoa_u((uint32_t)(ui.press_x * 100.0f + 0.5f), q);
    while (*q)
        q++;
    scopy(q, "%");
    return t;
}
static void press_run(void)                            /* each frame, while a scene key is held */
{
    char t[16];
    int p;
    float x;
    uint32_t now = plat_ms();
    if (ui.press < 0)
        return;
    p = press_scaled(plat_pressure(WHITE_K[ui.press]));
    x = fm_clampf(((float)p / 127.0f - PR_DEAD) / (PR_FULL - PR_DEAD), 0.0f, 1.0f);
    if ((float)p / 127.0f >= PR_COMMIT) {
        if (!ui.press_hi_t)
            ui.press_hi_t = now;
        if (!ui.press_done && now - ui.press_hi_t >= PR_COMMIT_MS && now - ui.press_t0 >= PR_WAIT_MS) {
            ui.press_done = 1;
            ui.flash_t = now;
        }
    } else {
        ui.press_hi_t = 0;
    }
    if (ui.press_done)
        x = 1.0f;
    if (now - ui.press_t0 < PR_WAIT_MS && !ui.press_done) {
        ui.press_ft = now;
        return;                                        /* a tap so far: nothing moves yet */
    }
    {                                                  /* toward the pressure, at most the whole way a Scene time */
        float step = (float)(now - ui.press_ft) / (scene_s() * 1000.0f);
        ui.press_ft = now;
        ui.press_x += fm_clampf(x - ui.press_x, -step, step);
    }
    sound_blend(ui.visit_back, ui.press_to, ui.press_x);
    loop_note(LP_BLEND_X, (int)(ui.press_x * 1000.0f + 0.5f), 20);
    t[0] = 0;
    scene_name(t, proj.bank, ui.press);
    if (ui.press_done)
        say_c(t, "TAKEN", K_GOLD);
    else
        say_c(press_label(t, ui.press), 0, K_GOLD);   /* "B5  40%", as a gesture's (Evan, 2026-10-30) */
}

/* a scene key's press changed the sound (tapped, taken or kept): SEL + SAVE goes back to the sound before it. Not
 * one let go back: nothing changed */
static void press_undo(void)
{
    memcpy(ui.undo, ui.visit_back, sizeof ui.undo);
    ui.undo_ok = 1;
    ui.undo_at = ui.visit_from;
    if (ui.visit_from < 0)                             /* the first time over a sound of the player's own: how */
        tip(TIP_UNDO);
}
/* SEL tapped while a scene key is held (as a gesture's, Evan 2026-10-30): the blend stays where it is, a sound of
 * its own; "B5  40%  KEPT", and the first time, how to undo it */
static void press_keep(int n)
{
    char t[16];
    ui.press = -1;
    ui.scene_at = -1;
    press_undo();
    say_c(press_label(t, n), "KEPT", K_GOLD);
    loop_note(LP_BLEND_X, (int)(ui.press_x * 1000.0f + 0.5f), 0);   /* the loop: where it was kept, and let go */
    loop_note(LP_BLEND_END, 0, 0);
}

static void press_end(void)                            /* the key let go */
{
    int n = ui.press, k;
    uint32_t now = plat_ms();
    char t[16] = "SCENE ";
    ui.press = -1;
    if (n < 0)
        return;
    if (!ui.press_done && now - ui.press_t0 < PR_TAP_MS) {
        /* a tap (however far a crossfade had begun): there, over the Scene time */
        press_undo();
        scene_recall(n, 0);
        tip(TIP_BLEND);
        return;
    }
    scene_name(t + 6, proj.bank, n);
    for (k = 0; k < DR_NPARAMS; k++)
        ui.morph_from[k] = proj.par[k];
    ui.morph_t0 = now;                                 /* the rest of the way, at the same speed (a Scene time
                                                        * the whole way) */
    ui.morph_ms = (uint32_t)(scene_s() * 1000.0f * (ui.press_done ? 1.0f - ui.press_x : ui.press_x));
    if (ui.press_done) {                               /* taken: the rest of the way, and it's the scene now */
        for (k = 0; k < DR_NPARAMS; k++)
            ui.morph_to[k] = ui.press_to[k];
        ui.morph = (int8_t)n;
        ui.scene_at = (int8_t)n;
        press_undo();
        say_c(t, "TAKEN", K_GOLD);
        loop_note(LP_SCENE, proj.bank << 4 | n, 0);   /* the loop: the rest of the way */
    } else {                                           /* let go, no SEL: back to where it began, at the same speed
                                                        * (Evan, 2026-10-30: SEL is what keeps it) */
        ui.morph = VISIT_BACK;
        ui.visit = (int8_t)(-2 - n);                   /* (pressed again on the way: picked up from there) */
        ui.scene_at = ui.visit_from;                   /* (the header says "→ A5", "→ back") */
        loop_note(LP_BLEND_X, 0, ui.morph_ms);         /* the loop: back (its slide 0.5 s at most), and let go */
        loop_note(LP_BLEND_END, 0, 0);
    }
}

static void button(int b)                              /* the other buttons, on release */
{
    switch (b) {
    case B_SAVE:
        if (ui.btn >> B_SEL & 1u) {                    /* SEL + SAVE: undo (v0.3) */
            ui.btn_used |= 1u << B_SEL;
            scene_undo();
        } else {
            save_ask(-1);
        }
        break;
    case B_REC:
        loop_button();
        break;
    default: break;                                    /* SEL: a modifier (fine pitch) */
    }
}

/* MIDI in (USB and the TRS jack): a note on channel 1 tunes drone I to it, on channel 2 drone II */
/* MIDI CLOCK IN (v0.3, step 3; a test build first, OM_CLOCK_TEST: the tempo and jitter on screen). 24 pulses a beat,
 * each stamped as it came in (us; the inputs are read between renders, so a pulse can wait one out). Over the last
 * two beats (48 pulses) a straight line through their times: its slope the tempo, how far each pulse sits off it the
 * jitter (its RMS and its largest). No pulse for 500 ms: lost, and it starts over */
#define CLK_N 48
#define CLK_LOST_MS 500u
static struct {
    uint32_t t[CLK_N];                 /* the last pulses' times, us */
    uint32_t n;                        /* pulses since it (re)started */
    uint32_t count;                    /* pulses since a Start (or since it came), for the beat; 256 beats round */
    uint32_t last_ms, shown_ms, hold_ms;   /* the last pulse; the readout last shown; a message held till */
    uint32_t starts, conts, stops;
    float bpm, jit, jmax;              /* the tempo; the jitter, RMS and largest, ms */
    int shown;                         /* the tempo as shown, whole BPM: moves only past 0.6 of a BPM (a wobbly master's
                                        * readout flickered: a user's report, 2026-10-20) */
    uint8_t following;                 /* the Swell follows it (said once when it starts, and when it's lost) */
} clk;
static void clk_fit(void)
{
    uint32_t k, t0 = clk.t[(clk.n - CLK_N) % CLK_N];
    float sy = 0.0f, sxy = 0.0f, slope, b, e2 = 0.0f, em = 0.0f;
    const float mx = 0.5f * (float)(CLK_N - 1), sxx = (float)CLK_N * ((float)CLK_N * (float)CLK_N - 1.0f) / 12.0f;
    for (k = 0; k < CLK_N; k++) {      /* x: the pulse's number, y: its time after the first (us) */
        float y = (float)(clk.t[(clk.n - CLK_N + k) % CLK_N] - t0);
        sy += y;
        sxy += ((float)k - mx) * y;
    }
    slope = sxy / sxx;
    b = sy / (float)CLK_N - slope * mx;
    for (k = 0; k < CLK_N; k++) {
        float e = (float)(clk.t[(clk.n - CLK_N + k) % CLK_N] - t0) - (b + slope * (float)k);
        e2 += e * e;
        em = fm_maxf(em, fm_fabsf(e));
    }
    clk.bpm = slope > 0.0f ? 60.0e6f / (24.0f * slope) : 0.0f;
    clk.jit = fm_sqrtf(e2 / (float)CLK_N) * 0.001f;
    clk.jmax = em * 0.001f;
}
static void clk_pulse(uint32_t us)
{
    if (clk.n && plat_ms() - clk.last_ms > CLK_LOST_MS)
        clk.n = 0;
    clk.t[clk.n % CLK_N] = us;
    clk.n++;
    clk.count = (clk.count + 1u) % (24u * 256u);
    clk.last_ms = plat_ms();
    if (clk.n >= CLK_N) {
        clk_fit();
        if (!clk.shown || clk.bpm > (float)clk.shown + 0.6f || clk.bpm < (float)clk.shown - 0.6f)
            clk.shown = (int)(clk.bpm + 0.5f);
    }
}
/* each frame: the engine told the tempo and the beat, counted in pulses (the first after a Start beat 0), and how far
 * into the next one by now. Only while the clock runs: stopped or lost, the engine carries on at the last tempo
 * (Evan, 2026-10-26), until a clock comes again */
static void clk_feed(void)
{
    uint32_t now = plat_ms();
    float iv, f;
    char a[24];
    if (!proj.par[P_CLK_SYNC]) {                /* Sync off (Setup 1): the clock ignored, its tempo forgotten, the
                                                 * Swell, LFO, delay and Scene time on their knobs (Evan, 2026-11-01) */
        if (clk.n || clk.following || dr_ext_spb > 0.0f) {
            clk.n = 0;
            clk.following = 0;
            clk.shown = 0;
            dr_ext_spb = 0.0f;
            dr_ext_seq++;
        }
        return;
    }
    if (clk.following && now - clk.last_ms > CLK_LOST_MS) {    /* lost: said once, the Swell carries on */
        clk.following = 0;
#ifndef OM_CLOCK_TEST
        itoa_u((uint32_t)clk.shown, scopy(a, "KEEPS "));   /* (the box fits 23) */
        say("CLOCK STOPPED", a);
#endif
    }
    if (clk.n < CLK_N || clk.bpm < 20.0f || now - clk.last_ms > CLK_LOST_MS)
        return;
    if (!clk.following) {                                       /* found: said once */
        clk.following = 1;
#ifndef OM_CLOCK_TEST
        {
            char *q = a;
            itoa_u((uint32_t)clk.shown, q);
            while (*q)
                q++;
            scopy(q, " BPM");
        }
        say_c(a, "SWELL ON CLOCK", K_GOLD);
        tip(TIP_CLOCK);
#endif
    }
    iv = 60.0e6f / (clk.bpm * 24.0f);                  /* a pulse, us */
    f = fm_clampf((float)(plat_us() - clk.t[(clk.n - 1u) % CLK_N]) / iv, 0.0f, 0.999f);
    dr_ext_spb = DR_SR * 60.0f / clk.bpm;
    dr_ext_beat = ((float)((clk.count + 24u * 256u - 1u) % (24u * 256u)) + f) * (1.0f / 24.0f);
    dr_ext_seq++;
}
int ui_clock_bpm(void) { return clk.shown; }
static float clk_bar_frac(void)        /* how far into its bar (4 beats) the clock is, 0..1 */
{
    float iv, f, b;
    if (clk.n < CLK_N || clk.bpm < 20.0f)
        return 0.0f;
    iv = 60.0e6f / (clk.bpm * 24.0f);
    f = fm_clampf((float)(plat_us() - clk.t[(clk.n - 1u) % CLK_N]) / iv, 0.0f, 0.999f);
    b = ((float)((clk.count + 24u * 256u - 1u) % 96u) + f) * (1.0f / 96.0f);
    return b;
}

void ui_clock_stats(uint32_t w[8])
{
    w[0] = (uint32_t)(clk.bpm * 100.0f + 0.5f);
    w[1] = (uint32_t)(clk.jit * 1000.0f + 0.5f);
    w[2] = (uint32_t)(clk.jmax * 1000.0f + 0.5f);
    w[3] = clk.n;
    w[4] = clk.starts;
    w[5] = clk.conts;
    w[6] = clk.stops;
    w[7] = clk.n ? plat_ms() - clk.last_ms : 0xFFFFFFFFu;
}
#ifdef OM_CLOCK_TEST
static char *dec1(float v, char *b);
static void clk_show(void)                             /* the test build: the clock's tempo and jitter, twice a second */
{
    uint32_t now = plat_ms();
    char a[24], b[24], *q;
    if (!clk.n || now - clk.shown_ms < 500u || (int32_t)(clk.hold_ms - now) > 0)
        return;
    clk.shown_ms = now;
    if (now - clk.last_ms > CLK_LOST_MS) {
        clk.n = 0;
        say("CLOCK LOST", 0);
        return;
    }
    if (clk.n < CLK_N) {
        say("CLOCK", "LISTENING");
        return;
    }
    q = dec1(clk.bpm, a);              /* "120.1 BPM" "JIT 0.8/1.7": the jitter's RMS / largest, ms (the box fits 23) */
    scopy(q, " BPM");
    q = scopy(b, "JIT ");
    q = dec1(clk.jit, q);
    *q++ = '/';
    dec1(clk.jmax, q);
    say(a, b);
}
#endif

static void midi_in(void)
{
    uint32_t pkt, us;
    while (plat_midi_in(&pkt, &us)) {
        uint32_t st = (pkt >> 8) & 0xFFu, n = (pkt >> 16) & 0x7Fu, v = pkt >> 24;
        if (st >= 0xF8u && !proj.par[P_CLK_SYNC])   /* Sync off: no clock, no Start */
            continue;
        if (st == 0xF8u) {
            clk_pulse(us);
            continue;
        }
        if (st == 0xFAu || st == 0xFBu || st == 0xFCu) {   /* Start, Continue, Stop: counted (and named, testing) */
            if (st == 0xFAu) {                 /* Start: the song's top, the Swell's too (Evan, 2026-10-26) */
                clk.starts++;
                clk.count = 0;
                dr_ext_beat = 0.0f;
                dr_ext_seq++;
                dr_ext_start = 1;
            } else if (st == 0xFBu) {
                clk.conts++;
            } else {
                clk.stops++;
            }
#ifdef OM_CLOCK_TEST
            say("CLOCK", st == 0xFAu ? "START" : st == 0xFBu ? "CONTINUE" : "STOP");
            clk.hold_ms = plat_ms() + 1000u;
#endif
            continue;
        }
        if ((st == 0x90u || st == 0x91u) && v) {
            int g = (int)(st & 1u), c = (int)n * 100;
            if (g == 1 && proj.par[P_PLINK])
                param_set(P_PLINK_OFF, c - proj.par[DR_GP(0, GP_PITCH)]);
            else
                param_set(DR_GP(g, GP_PITCH), c);
        }
    }
}

/* THE GESTURES (v0.2): Weight, Shift, Clean, Wash on the top four white keys (always these: Evan, 2026-10-18, you
 * learn where they are), which sense no pressure, so
 * time stands in: held, a gesture comes in (over its own time x Gesture time), its parts in order; let go, it goes
 * over as long as it was held (x Gesture time, turned live: Evan, 2026-10-18). A tap (under GEST_TAP_MS: a press
 * needn't be quick) latches it on, all the way;
 * tapping a latched one lets it go (over its own time); SEL held as you let go keeps it where it is. No depth or
 * time of its own to set (Evan, 2026-10-18: how you play it sets them); Shift's move is on Moves. Latched
 * levels are values (P_GL_*): scenes keep them. v0.3: four more on keys 9-12 (G4-C5: Tick, Chord, Snarl, Stutter,
 * each the partner of the top key four to its right), which sense pressure: held, one goes toward how hard the key is
 * pressed (over its time x Gesture time), easing off or let go it comes back; a tap latches it, as the top keys'.
 * SEL tapped while a gesture is held latches it at the level it has reached (Evan, 2026-10-27), on every gesture key. */
#define GEST_TAP_MS PR_TAP_MS          /* a gesture key let go sooner: a tap, as a scene key */
#define G_PRESS DR_G_TICK              /* gestures from here on are on the pressure keys */
static int gl_par(int g) { return g < G_PRESS ? P_GL_0 + g : P_GL_4 + (g - G_PRESS); }   /* its latch */
static uint32_t gkey(int g) { return g < G_PRESS ? WHITE_K[NSCENES + g] : WHITE_K[NPLAY + g - G_PRESS]; }
static void gesture_down(int g)
{
    if (proj.par[gl_par(g)]) {                     /* latched: let go of it, whatever the press */
        param_set(gl_par(g), 0);
        loop_note(gl_par(g), 0, 0);
        hands_on(gl_par(g));
        ui.g_rel |= (uint8_t)(1u << g);        /* (no banner: the header shows it going, Evan 2026-11-07) */
    }
    ui.g_held |= (uint8_t)(1u << g);
    ui.g_t0[g] = plat_ms();
    tip(TIP_GEST);
    ui.g_out_s[g] = 0.0f;
}
static void gesture_up(int g)
{
    uint32_t held = plat_ms() - ui.g_t0[g];
    if (!(ui.g_held >> g & 1u))
        return;                                        /* not a gesture's press (SEL + the key: a bank) */
    ui.g_held &= (uint8_t)~(1u << g);
    if (ui.g_rel >> g & 1u) {
        ui.g_rel &= (uint8_t)~(1u << g);
        return;
    }
    if (held >= GEST_TAP_MS && !(ui.btn >> B_SEL & 1u) && g < G_PRESS) {   /* let go after a hold: out over as long
                                                                         * as it was held (a pressure key: its own time) */
        ui.g_out[g] = ui.g_amt[g];
        ui.g_out_s[g] = fm_minf((float)held * 0.001f, 60.0f);
    }
    if (held < GEST_TAP_MS) {                          /* a tap: latched, at full */
        param_set(gl_par(g), 100);
        loop_note(gl_par(g), 100, 0);
        hands_on(gl_par(g));                   /* the header says "latched" */
    } else if (ui.btn >> B_SEL & 1u) {                 /* SEL held: latched where it is */
        int v = (int)(ui.g_amt[g] * 100.0f + 0.999f);   /* rounded up: a hair less would read as letting go
                                                          * (QA 2026-10-20: Shift's chord swapped, Weight jumped) */
        ui.btn_used |= 1u << B_SEL;
        param_set(gl_par(g), v < 1 ? 1 : v);
        loop_note(gl_par(g), v < 1 ? 1 : v, 0);
        hands_on(gl_par(g));
    }
}
static char *dec1(float v, char *b);                   /* below */
/* GESTURE TIME: the gestures' times x 0.25 .. x 12, 50 as set. Slower, evenly in seconds (each detent the same time
 * more, to 12x); faster, by ratio and only to 4x (Evan, 2026-10-28: most of the old x 0.1 .. x 10 was fast, the slow
 * end a few big jumps; a slow one is for a tap, which latches and comes in by itself) */
static float mtime_k(void)
{
    float m = (float)proj.mtime;
    return m > 50.0f ? 1.0f + (m - 50.0f) * 0.22f : fm_exp2f((m - 50.0f) * 0.04f);
}
static void mtime_turn(int32_t e)                      /* PRESETS: Gesture time, shown as Weight's time in (the
                                                        * others scale with it): right is faster (Evan, 2026-10-24) */
{
    int v = (int)proj.mtime - (int)e;
    float t;
    char *q;
    proj.mtime = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
    mark_dirty();
    t = fm_exp2f((float)DR_GEST_TIME[DR_G_WEIGHT] * 0.0490689f) * mtime_k();
    scopy(ui.glab[0], "GESTURE TIME");
    q = ui.glab[1];
    if (t < 9.95f)
        q = dec1(t, q);
    else
        itoa_u((uint32_t)(t + 0.5f), q);
    while (*q)
        q++;
    scopy(q, " s");
    ui.glab_until = plat_ms() + 1500u;
    ui.act_t = plat_ms();
}
static void gesture_run(void)                          /* each frame: each gesture toward where it's going */
{
    uint32_t now = plat_ms(), dt = now - ui.g_last;
    int g;
    ui.g_last = now;
    if (dt > 100u)
        dt = 100u;
    for (g = 0; g < DR_NGEST; g++) {
        float to = (float)proj.par[gl_par(g)] * 0.01f;
        if ((ui.g_held >> g & 1u) && !(ui.g_rel >> g & 1u)) {
            to = 1.0f;
            if (g >= G_PRESS) {                        /* a pressure key: as far as it's pressed (none: the emulator) */
                int pr = plat_pressure(gkey(g));
                if (pr >= 0)
                    to = fm_clampf(((float)press_scaled(pr) / 127.0f - PR_DEAD) / (PR_FULL - PR_DEAD), 0.0f, 1.0f);
            }
        }
        float *a = &ui.g_amt[g], k = mtime_k();
        float step = *a > to && ui.g_out_s[g] > 0.0f
                       ? (float)dt * 0.001f * ui.g_out[g] / (ui.g_out_s[g] * k)   /* let go: as long as it was held */
                       : (float)dt * 0.001f / (fm_exp2f((float)DR_GEST_TIME[g] * 0.0490689f) * k);
        int16_t q;
        *a = *a < to ? fm_minf(*a + step, to) : fm_maxf(*a - step, to);
        if (*a <= to)
            ui.g_out_s[g] = 0.0f;                      /* out (or on its way in again): its own time from here */
        q = (int16_t)(*a * 1000.0f + 0.5f);
        if (q != ui.g_sent[g] && ui.loop == L_REC)
            loop_note(LP_GEST + g, q, dt);             /* the loop: the level as played */
        q = (int16_t)(fm_maxf(*a, loop_gest(g)) * 1000.0f + 0.5f);   /* playing: the loop's, or the hand's if more */
        if (q != ui.g_sent[g]) {
            ui.g_sent[g] = q;
            dr_gesture(g, q);
        }
    }
}

static void global_turn(int role, int p, const char *name, int32_t e)
{
    char t[12];
    const dr_param_t *in = dr_param_info(p);
    ui.scene_at = -1;                          /* a hand on the sound */
    hands_on(p);                               /* a scene on its way leaves it alone (the morph goes on) */
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

/* THE SAFE RESTORE's question (v0.3, Evan 2026-10-30; main_fm1.c): a backup from USB staged, the FM-1 asks, the
 * banner held, the question and its keys in turn: SEL yes, HOME no, a minute without either no. While it asks, the panel does nothing else (the sound
 * plays on); a yes restarts it with the restored sound */
#define RQ_MS 60000u
static uint8_t rq, rq_ans;                             /* asking; the answer: 1 yes, 2 no */
static uint32_t rq_t;
void ui_restore_ask(void)
{
    rq = 1;
    rq_ans = 0;
    rq_t = plat_ms();
}
int ui_restore_answer(void)
{
    int a = rq_ans;
    if (rq && !a && plat_ms() - rq_t >= RQ_MS)
        a = 2;
    if (a)
        rq = rq_ans = 0;
    return a;
}
void ui_restore_said(int ok)                            /* 1 restored (it restarts), 0 failed, -1 no */
{
    say_c(ok > 0 ? "RESTORED" : ok ? "NOT RESTORED" : "RESTORE FAILED", ok > 0 ? "RESTARTING" : ok ? 0 : "NOTHING CHANGED",
          K_GOLD);
    ui.msg_until = plat_ms() + 2500u;
}
static int rq_input(uint32_t btn)                      /* asking: SEL or HOME answers, the rest waits; 1 = asking */
{
    uint32_t ch = btn ^ ui.btn, i;
    if (!rq)
        return 0;
    if (ch & btn & (1u << B_SEL))
        rq_ans = 1;
    else if (ch & btn & (1u << B_HOME))
        rq_ans = 2;
    ui.btn = btn;
    ui.btn_used = btn;                                 /* (their let-go does nothing after) */
    ui.keys = plat_keys();
    for (i = 0; i < NE; i++)
        plat_enc((int)i);                              /* turns while asking: dropped */
    say_c((plat_ms() - rq_t) / 1500u & 1u ? "[SEL] YES - [HOME] NO" : "RESTORE FROM USB?", 0, K_GOLD);   /* (one line:
                                                        * the question and its keys in turn) */
    return 1;
}

static void input(void)
{
    uint32_t btn = plat_buttons(), keys = plat_keys(), ch, i;
    int32_t e;
    if (rq_input(btn))
        return;
    save_run();
    ch = btn ^ ui.btn;
    ui.btn = btn;
    if ((ch & btn) >> B_SEL & 1u) {
        int g;
        ui.sel_t = plat_ms();
        ui.sel_turned = 0;
        for (g = 0; g < DR_NGEST; g++)                 /* SEL tapped while a gesture is held: latched where it is */
            if ((ui.g_held >> g & 1u) && !(ui.g_rel >> g & 1u) && plat_ms() - ui.g_t0[g] >= GEST_TAP_MS) {
                int v = (int)(ui.g_amt[g] * 100.0f + 0.999f);   /* rounded up, as a SEL let-go's */
                param_set(gl_par(g), v < 1 ? 1 : v);
                loop_note(gl_par(g), v < 1 ? 1 : v, 0);
                hands_on(gl_par(g));
                ui.g_rel |= (uint8_t)(1u << g);        /* its let-go does nothing more */
                ui.btn_used |= 1u << B_SEL;            /* the header says "latched" */
            }
        if (ui.press >= 0 && !ui.press_done && plat_ms() - ui.press_t0 >= PR_WAIT_MS) {   /* ... a scene key: the
                                                        * blend kept there; its let-go does nothing more */
            ui.btn_used |= 1u << B_SEL;
            press_keep(ui.press);
        }
    }
    if (!(btn >> B_SEL & 1u))
        ui.km_key = -1;
    {   /* THE HEADPHONE JACK: the Speaker low cut only while the speaker plays (Evan, 2026-10-20); a plug settled
         * for 150 ms (its contacts bounce) */
        int ph = plat_headphones();
        if (ph != ui.jack) {
            ui.jack = (uint8_t)ph;
            ui.jack_t = plat_ms();
        } else if (ph != dr_phones && plat_ms() - ui.jack_t >= 150u) {
            dr_phones = (uint8_t)ph;                   /* (no message: you hear which it is, Evan) */
        }
    }
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
            if (i == B_OCTDN || i == B_OCTUP) {        /* the LFO waves, on the press; SEL + OCT: back one */
                int back = (int)(btn >> B_SEL & 1u);
                ui.btn_used |= m;
                if (back)
                    ui.btn_used |= 1u << B_SEL;
                wave_step(i == B_OCTUP, back ? -1 : 1);
            }
            if (i == B_PLAY && (btn >> B_SEL & 1u)) {  /* SEL + PLAY: drone II alone, out / in */
                ui.btn_used |= 1u << B_SEL;
                param_set(P_II_OUT, !proj.par[P_II_OUT]);   /* a sound's: scenes keep it */
                say_c("DRONE II", proj.par[P_II_OUT] ? "OUT" : "IN", K_DRONE[1]);
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
    ch = keys ^ ui.keys;                               /* the black keys: switches, on the press */
    ui.keys = keys;
    for (i = 0; i < 11u; i++) {
        sw_t w = sw_layer()[i];
        if (ui.btn >> B_SEL & 1u) {                   /* SEL held: a key touched names itself on the map, and
                                                        * stays as it is (Evan, 2026-10-19) */
            if ((ch & keys) >> BLACK_K[i] & 1u) {
                ui.km_key = (int8_t)i;
                ui.btn_used |= 1u << B_SEL;
                ui.rst_done = 1;                       /* LFO reset: no lock from this hold either */
            }
            continue;
        }
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
            hands_on(P_LFO_LOCK);
            param_set(P_LFO_LOCK, on);
            ui.scene_at = -1;
            if (on && !proj.par[P_PULSE])              /* (shorter: "LOCKED (SWELL OFF)" ran off the screen) */
                say_c("LFO LOCKED", "SWELL OFF", K_LFO);
            else
                say_c("LFO TO SWELL", on ? "LOCKED" : "FREE", K_LFO);
            ui.rst_done = 1;
        }
    }
    {   /* SAVE held a moment, no key yet: say what it does (the review: holding it gave no hint) */
        static uint32_t save_dn;
        if (btn >> B_SAVE & 1u) {
            if (!save_dn)
                save_dn = plat_ms() | 1u;
            if (plat_ms() - save_dn >= HOLD_MS && !(ui.btn_used >> B_SAVE & 1u)) {
                if (btn >> B_SEL & 1u)
                    say_c("LET GO: UNDO", 0, K_GOLD);
                else
                    say_c(BANK_IS_PRESETS(proj.bank) ? "PRESETS ARE FIXED" : "KEY: STORE", "LET GO: SAVE", K_GOLD);
            }
        } else {
            save_dn = 0;
        }
    }
    for (i = 0; i < NPLAY; i++) {                      /* the white keys 1-8: scenes (SAVE held: store) */
        if ((ch & keys) >> WHITE_K[i] & 1u) {
            if (btn >> B_SAVE & 1u) {
                ui.btn_used |= 1u << B_SAVE;           /* SAVE let go: not a plain save */
                scene_store((int)i);
            } else if (plat_pressure(WHITE_K[i]) >= 0 && scene_has(proj.bank, (int)i)) {
                press_start((int)i, scene_get(proj.bank, (int)i));   /* the pressure crossfade (v0.2) */
            } else {                                   /* no pressure here (the emulator): 0.1's timed visit */
                int k;
                for (k = 0; k < DR_NPARAMS; k++)       /* where a visit comes back to */
                    ui.visit_back[k] = proj.par[k];
                ui.visit_from = ui.scene_at;
                scene_recall((int)i, 1);
                ui.visit = ui.morph == (int8_t)i ? (int8_t)i : -1;
                ui.visit_t = plat_ms();
            }
        } else if ((ch & ~keys) >> WHITE_K[i] & 1u && ui.press == (int8_t)i) {
            press_end();
        } else if ((ch & ~keys) >> WHITE_K[i] & 1u && ui.visit == (int8_t)i) {
            ui.visit = -1;                             /* let go: a tap stays; a hold was a visit: */
            if (plat_ms() - ui.visit_t >= HOLD_MS)     /* back to the sound before it, as it came */
                scene_back();
        }
    }
    for (i = NPLAY; i < NWHITE; i++) {                 /* the eight gesture keys, G4 .. G5 */
        const int g = i < NSCENES ? G_PRESS + (int)(i - NPLAY) : (int)(i - NSCENES);   /* keys 9-12: the pressure
                                                                                          * gestures (v0.3) */
        if (!((ch & keys) >> WHITE_K[i] & 1u))
            continue;
        if (btn >> B_SEL & 1u) {                       /* SEL + one: a bank, in a row (not while a scene key is
                                                        * held: a visit's way back stays put) */
            const uint8_t bk = KEY_BANK[i - NPLAY];
            uint32_t j, held = 0;
            ui.btn_used |= 1u << B_SEL;
            for (j = 0; j < NPLAY; j++)
                held |= keys >> WHITE_K[j] & 1u;
            if (held)
                continue;
            if (proj.bank != bk) {
                proj.bank = bk;
                ui.scene_at = -1;
                mark_dirty();
            }
            say_c("BANK", BANK_N[bk], K_GOLD);
        } else {
            gesture_down(g);                           /* the gestures (v0.2, v0.3) */
        }
    }
    for (i = NPLAY; i < NWHITE; i++)                 /* ... and let go (one that picked a bank: nothing) */
        if ((ch & ~keys) >> WHITE_K[i] & 1u)
            gesture_up(i < NSCENES ? G_PRESS + (int)(i - NPLAY) : (int)(i - NSCENES));
    if (btn || keys || ch)
        ui.act_t = plat_ms();
    loop_run();
    midi_in();
    clk_feed();
#ifdef OM_CLOCK_TEST
    clk_show();
#endif
    if ((e = plat_enc(EN_SELECT)) != 0)              /* SELECT: the LFO rate */
        global_turn(EN_SELECT, dr_ext_spb > 0.0f ? P_LFO_DIV : P_LFO_RATE, dr_ext_spb > 0.0f ? "LFO CYCLE" : "LFO RATE", e);
    if ((e = plat_enc(EN_ALGO)) != 0)                /* ALGORITHM: the Movement, every LFO depth 0 .. 200 % */
        global_turn(EN_ALGO, P_MOVE, "MOVEMENT", e);
    if ((e = plat_enc(EN_PRESET)) != 0)              /* PRESETS: Gesture time, every gesture's speed */
        mtime_turn(e);
    for (i = 0; i < 4; i++)
        if ((e = plat_enc(EN_K1 + (int)i)) != 0) {
            knob_turn((int)i, e);
            if (ui.btn >> B_SEL & 1u)
                ui.btn_used |= 1u << B_SEL;            /* SEL + a knob: SEL used (no map after it) */
            ui.scene_at = -1;                          /* the sound is no longer the scene's */
            ui.morph = -1;                             /* a hand on a knob: the morph stops where it is */
            ui.visit = -1;                             /* ... and a visit held becomes a stay (no way back) */
            ui.press = -1;                             /* ... a crossfade too, where it is */
            if (ui.hold_btn >= 0)
                ui.hold_turned = 1;                    /* a peek: let go of the button, back to the page */
            ui.touch_t[i] = plat_ms() + 1200u;
            ui.act_t = plat_ms();
        }
}

/* ------------------------------------------------------------- screen --- */
/* 0.3's new look (look.c, the screen spec's direction C, "Signal light"): the header (the page, what's running, the
 * sound); the picture in the top half to two-thirds, drone I on the left over KNOB 1-2, drone II on the right over
 * KNOB 3-4; the page's four values below, each lighting while its knob moves (its own timer: two hands, two lit). A
 * message is a banner across the top of the picture. Nothing stays on screen that isn't selected or moving. */
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

static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

/* a knob's name on its card; the few too long for one, shorter there (the manual has the full ones; the short forms
 * Evan's, 2026-11-01) */
static const char *card_name(int p)
{
    const char *n = p == P_PLINK_OFF ? "Link ofs" : dr_param_info(p)->name;
    if (p == P_SHIFT_MOVE)
        return "Shift";
    if (p == P_SCENE_T)
        return "Scene";                        /* Scene time */
    if (n[0] == 'X' && n[1] == '-' && n[2] == 'm')
        return "X-mod";                        /* X-mod amt */
    switch (p) {                               /* too long at the cards' size (FONT_S, 2026-10-20) */
    case P_CHAR: return "Char.";
    case P_FEEDBACK: return "Feedbk";
    case P_LFO_RMOD: return "Speed";           /* Speed mod */
    case P_DLY_LFO_DEPTH: return "Delay";      /* Delay mod */
    case P_LFO_RMOD_SRC: return "Source";      /* Mod by */
    case P_MORPH: return "Morph";
    case P_DLY_REPEATS: return "Repeat";
    case P_GLIDE: return "Glide";              /* Pitch glide */
    case P_PRESS_SENS: return "Press.";
    case P_DLY_TYPE: return "Type";            /* the delay's (KNOB 4 with SEL), the reverb's (KNOB 3) */
    case P_REV_TYPE: return "Type";
    case P_DLY_ROUTE: return "Into rev";       /* what goes into the reverb */
    case P_SCENE_BARS: return "Scene";         /* the Scene time in bars (on a clock) */
    default: break;
    }
    if (p < P_XMOD && p % GP_N == GP_OLFO_DEPTH)  /* LFO sends: named for what they move */
        return proj.par[p - GP_OLFO_DEPTH + GP_OLFO_DEST] == DR_TO_XWAVE ? "X-Wave" : "Pitch";
    if (p < P_XMOD && p % GP_N == GP_FLFO_DEPTH)
        return proj.par[p - GP_FLFO_DEPTH + GP_FLFO_DEST] == DR_TO_RESO ? "Reso" : "Cutoff";
    if (p < P_XMOD && p % GP_N == GP_MIX)
        return "Osc/Sb";                       /* Osc/Sub */
    return n;
}

static int fade_q(void) { return (int)(dr_fade * 20.0f + 0.5f); }   /* the fade in 20 steps */

static int lit(int i) { return plat_ms() < ui.touch_t[i]; }          /* knob i moving (or just moved) */

static uint16_t blend(uint16_t a, uint16_t b, int q, int n)          /* a -> b, q of n */
{
    int r = ((a >> 11) * (n - q) + (b >> 11) * q) / n, g = (((a >> 5) & 63) * (n - q) + ((b >> 5) & 63) * q) / n,
        bl = ((a & 31) * (n - q) + (b & 31) * q) / n;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

/* the picture as drawn: each new capture blends into it (~0.1 s at ~20 pictures a second), so a detuned partner's
 * ramp slides across instead of jumping from picture to picture (it looked like two waves at once) */
static float sc_disp[DR_NGROUPS][DR_SCOPE_N];
static int gpar(int g, int p) { return proj.par[DR_GP(g, p)]; }


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
static float xwave_atf(float x, float pw, float ph)   /* x: X-WAVE in knob units, 0..100 (a fraction while it moves) */
{
    const float s = x * 3.0f;
    int wi = (int)((s + 2.0f) / 100.0f);       /* (a divide: exact on whole knob values, as the int one was) */
    float wf = (s - 100.0f * (float)wi) * 0.01f, a;
    if (wi >= 3 || wf <= 0.02f)
        return wave_at(wi > 3 ? 3 : wi, pw, ph);
    a = wave_at(wi, pw, ph);
    return a + (wave_at(wi + 1, pw, ph) - a) * wf;
}
/* X-WAVE and the resonance as heard while the drones play (an LFO or Wander moving them: Evan, 2026-11-08, the
 * pictures show what moves; the knob's number stays the knob's), else the knob */
static float live_par(int g, int p, float now)
{
    const float k = (float)gpar(g, p);
    return dr_fade > 0.0f && fm_fabsf(now - k) >= 0.5f ? now : k;
}
static float xw_now(int g) { return live_par(g, GP_WAVE, dr_xw[g]); }
static float reso_now(int g) { return live_par(g, GP_RESO, dr_reso[g]); }
static float pw_of(int g) { return 0.5f - 0.45f * (float)gpar(g, GP_WIDTH) * 0.01f; }


/* a drone's filter, as the engine's (12 dB state-variable): its response in dB at f, on the cutoff knob's
 * own scale (0..100 = 20 Hz .. 18 kHz); cut, the cutoff there (moved, for the cross-mod and the LFO) */
static float filt_db(int g, float cut, float f)
{
    int mode = gpar(g, GP_MODE);
    float r = reso_now(g) * 0.001f, k = fm_maxf(2.0f - 2.1f * r, 0.06f), rin = 1.0f - 0.6f * r * r;
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

/* the reverb's tail, as measured (to -60 dB after a burst, QA batch 3; doubling the -30 dB time had made
 * the long ones short, and the Chorus hall no longer than the Hall) and its pre-delay: an envelope over 5 s,
 * as high as h */
static const float RV_PRE_S[DR_NREV] = {0.004f, 0.018f, 0.042f, 0.024f, 0.030f};   /* drone_fx.c's pre-delays */
static float rev_secs(int v) { return v >= 100 ? 0.0f : 0.3f * fm_exp2f((float)v * 0.070607f); }   /* 0: Inf */

/* where an echo is drawn, off its time: Grain's jitter (its own sound shakes the time), Self's drift later (its
 * loudness stretches it) */
static int32_t echo_shift(int ty, int k)
{
    static const int8_t JIT[8] = {0, 2, -2, 3, -1, -3, 1, 2};
    return ty == 4 ? JIT[k & 7] : ty == 5 ? (int32_t)(k * k) / 3 : 0;
}

/* THE BLACK KEYS' MAP (SEL held 0.4 s, nothing else asking for the screen: a knob turning, a white key held): the
 * eleven keys drawn where they sit on the keyboard (3-2-3-2-1), lit as the panel's are, and only their groups
 * named. A key touched while SEL is held names itself and its setting, large, and doesn't switch (Evan, 2026-10-19:
 * the eleven names and settings at once were too crowded to read). The LFO pages' layer on those pages */
#define KEYMAP_MS 400u
#define KEYMAP_JOB_MS 1500u            /* ... on a page where a knob has a SEL job: a quick SEL + turn never meets the map
                                        * (Evan, 2026-10-28); once a job is turned, not until SEL is let go */
static int keymap_on(void)
{
    uint32_t white = 0, i, wait = page_jobs(ui.view) ? KEYMAP_JOB_MS : KEYMAP_MS;
    for (i = 0; i < NWHITE; i++)
        white |= 1u << WHITE_K[i];
    if (page_jobs(ui.view) && ui.sel_turned && ui.km_key < 0)
        return 0;
    if ((ui.btn_used >> B_SEL & 1u) && ui.km_key < 0)
        return 0;                                      /* SEL already used in this hold (SEL + PLAY, a bank, a knob):
                                                        * no map until it's let go (Evan, 2026-11-08) */
    return (ui.btn >> B_SEL & 1u) && (plat_ms() - ui.sel_t >= wait || ui.km_key >= 0) && !lit(0) && !lit(1) &&
           !lit(2) && !lit(3) && !(ui.keys & white) && ui.press < 0;
}
static void sw_text(sw_t w, const char **name, const char **st)
{
    static const char *const SUB[7] = {"", "-1 oct", "-2 oct", "1/3", "1/5", "-3 oct", "-4 oct"},
                             *const MODE[3] = {"LP", "BP", "HP"};
    static const char *const TAP[4] = {"off", "I", "II", "I+II"};
    const int16_t *p = &proj.par[DR_GP(w.g, 0)];
    int on = sw_get(w);
    *st = on ? "on" : "off";
    switch (w.s) {
    case S_PARTNER: *name = "Partner"; break;
    case S_SUBOCT: *name = "Sub"; *st = SUB[(unsigned)p[GP_SUBOCT] < 7u ? p[GP_SUBOCT] : 1]; break;
    case S_HP: *name = "Filter"; *st = MODE[(unsigned)p[GP_MODE] < 3u ? p[GP_MODE] : 0]; break;
    case S_PLOCK: *name = "Pitch lock"; break;
    case S_LFORESET: *name = "LFO reset"; *st = proj.par[P_LFO_LOCK] ? "locked" : "tap"; break;
    case S_PLINK: *name = "Pitch link"; break;
    case S_SYNC: *name = "Hard sync"; break;
    case S_XMOD: *name = "X-mod"; *st = proj.par[P_XMOD] == DR_XMOD_OUT ? "snarl" : proj.par[P_XMOD] == DR_XMOD_FM ? "pitch" : on ? "cutoff" : "off"; break;
    case S_OTAP: *name = "Osc LFO"; *st = TAP[(unsigned)p[GP_OLFO] & 3u]; break;
    case S_ODEST: *name = "Osc to"; *st = on ? "X-Wave" : "Pitch"; break;
    case S_FDEST: *name = "Filt to"; *st = on ? "Reso" : "Cutoff"; break;
    case S_FTAP: *name = "Filt LFO"; *st = TAP[(unsigned)p[GP_FLFO] & 3u]; break;
    case S_DTAP: *name = "Dly LFO"; *st = TAP[(unsigned)proj.par[P_DLY_LFO] & 3u]; break;
    default: *name = ""; *st = ""; break;
    }
}

/* a wave -1..1 as a light's level: dark at its bottom, lit at its top */
static uint8_t wave_lvl(float w) { return (uint8_t)(fm_clampf(0.5f + 0.5f * w, 0.0f, 1.0f) * 255.0f + 0.5f); }

/* THE LIGHTS (Evan, 2026-11-07, after the lights review: ~/niagara-drift-notes/reviews/2026-11-07-lights.md). One
 * clock for every slow blink; a dim step for a first choice, visible beside dark (2 frames of 32: the gesture glow's
 * 40 is 1); stopped, PLAY breathes and nothing else moves */
#define LED_DIM 64
#define BLINK_MS 1200u
static int blink(uint32_t on_ms) { return plat_ms() % BLINK_MS < on_ms; }
static int lk_glide(uint32_t now, float *line, int *bank);   /* look.c: a scene on its way, as the header shows it */

/* a key or button a message names in a box ("turn on [SEL] + [PLAY]", "unlock [C#4]", "[D5]-[G5] bank") blinks
 * quickly over its own light while the message shows, so the box on the screen points at the panel */
static int key_of_note(const char *n)          /* "F#3" .. "G5": its key (0 = F3), or -1 */
{
    static const int8_t PC[7] = {9, 11, 0, 2, 4, 5, 7};   /* A .. G */
    int pc, oct, k;
    if (n[0] < 'A' || n[0] > 'G')
        return -1;
    pc = PC[n[0] - 'A'] + (n[1] == '#');
    oct = n[1 + (n[1] == '#')] - '0';
    if (oct < 0 || oct > 9 || n[2 + (n[1] == '#')])
        return -1;
    k = oct * 12 + pc - (3 * 12 + 5);
    return k >= 0 && k < NKEYS ? k : -1;
}
static int txt_eq(const char *a, const char *b)
{
    while (*a && *a == *b)
        a++, b++;
    return *a == *b;
}
static void boxed_blink(uint8_t *b, uint8_t *k)
{
    static const struct {
        const char *n;
        uint8_t b;
    } BTN[] = {{"SEL", B_SEL}, {"PLAY", B_PLAY}, {"SAVE", B_SAVE}, {"HOME", B_HOME}, {"ENV", B_ENV}, {"EDIT", B_EDIT},
               {"LFO", B_LFO}, {"SEQ", B_SEQ}, {"GLO", B_GLO}, {"FX", B_FX}, {"ARP", B_ARP}, {"REC", B_REC},
               {"OCT-", B_OCTDN}, {"OCT+", B_OCTUP}, {"OCT", B_OCTDN}};
    const uint8_t lv = plat_ms() % 300u < 150u ? 255 : 0;
    int m, last = -1, g;
    for (g = 0; g < DR_NGROUPS; g++)           /* Partner's Detune turning with its partner off: the partner's key
                                                * blinks with the page's "needs [F#3]" (Evan, 2026-11-08) */
        if (ui.view == V_PARTNER && plat_ms() < ui.touch_t[g ? 3 : 1] && !proj.par[DR_GP(g, GP_PARTNER)])
            k[key_of_note(g ? "F#4" : "F#3")] = lv;
    if (plat_ms() >= ui.msg_until)
        return;
    for (m = 0; m < 2; m++) {
        const char *q = ui.msg[m];
        for (; *q; q++) {
            char n[8];
            int i = 0, kk, range;
            if (*q != '[')
                continue;
            range = q > ui.msg[m] + 1 && q[-1] == '-' && q[-2] == ']';
            for (q++; *q && *q != ']' && i < 7; q++)
                n[i++] = *q;
            n[i] = 0;
            for (i = 0; i < (int)(sizeof BTN / sizeof BTN[0]); i++)
                if (txt_eq(n, BTN[i].n)) {
                    b[BTN[i].b] = lv;
                    if (BTN[i].b == B_OCTDN && !n[3])
                        b[B_OCTUP] = lv;              /* "[OCT]": both */
                }
            if ((kk = key_of_note(n)) >= 0) {
                for (i = range && last >= 0 && last < kk ? last : kk; i <= kk; i++)
                    k[i] = lv;                         /* "[D5]-[G5]": the keys between too */
                last = kk;
            }
            if (!*q)
                break;
        }
    }
}

static void leds(void)
{
    uint8_t b[NB] = {0}, k[NKEYS] = {0};
    const uint32_t now = plat_ms();
    const int still = !ui.playing && dr_fade <= 0.0f;    /* stopped: nothing moves but PLAY */
    uint32_t i;
    for (i = 0; i < NPAGE_BTN; i++)                  /* the button of the page shown */
        if (on_pages((int)i))
            b[PAGE_BTN[i].btn] = 255;
    if (ui.playing)
        b[B_PLAY] = 255;
    else                                               /* stopped: PLAY breathes, the one thing to press (the header
                                                        * says "[PLAY] to start") */
        b[B_PLAY] = (uint8_t)(40.0f + 215.0f * (0.5f - 0.5f * fm_cosf(6.2831853f * (float)(now % 3600u) / 3600.0f)));
    if (ui.dirty)                                      /* SAVE: a dim glow while something isn't saved yet */
        b[B_SAVE] = 90;
    if (proj.par[P_LFO_W1])                            /* OCT- / OCT+: each wave, pulsing with it; stopped, still */
        b[B_OCTDN] = still ? LED_DIM : wave_lvl(dr_lfo[0]);
    if (proj.par[P_LFO_W2])
        b[B_OCTUP] = still ? LED_DIM : wave_lvl(dr_lfo[1]);
    if (proj.par[P_PULSE]) {                           /* ARP: breathing with the swell, a beat a breath; stopped,
                                                        * still */
        float b1 = 1.0f + (float)proj.par[P_PULSE_SWING] * 0.01f, ph = dr_pulse_ph;
        float u = ph < b1 ? ph / b1 : (ph - b1) / (2.0f - b1);
        b[B_ARP] = still ? LED_DIM : wave_lvl(fm_cosf(6.2831853f * u));
    }
    if (ui.loop == L_PLAY)                             /* REC: steady looping; recording, once a second, fast in the
                                                        * last 20 s before it's full (fast all along was an alarm) */
        b[B_REC] = 255;
    else if (ui.loop == L_REC)
        b[B_REC] = now - ui.loop_t0 + 20000u >= LOOP_MS ? ((ui.frame / 8u) & 1u ? 255 : 0) : now % 1000u < 500u ? 255 : 0;
    {                                                  /* SEL: glows while it can keep what a hand holds (a blend, a
                                                        * gesture past its tap) */
        int keep = ui.press >= 0 && !ui.press_done && now - ui.press_t0 >= PR_WAIT_MS;
        for (i = 0; i < DR_NGEST; i++)
            if ((ui.g_held >> i & 1u) && !(ui.g_rel >> i & 1u) && now - ui.g_t0[i] >= GEST_TAP_MS)
                keep = 1;
        if (keep) {
            b[B_SEL] = 128;
            tip(TIP_KEEP);
        }
    }
    for (i = 0; i < 11u; i++) {                      /* the black keys: dark off, dim a first choice, lit, winking */
        int v = sw_look(sw_layer()[i]);
        if (sw_layer()[i].s == S_LFORESET && (ui.keys >> BLACK_K[i] & 1u))
            v = 2;
        k[BLACK_K[i]] = v == 1 ? LED_DIM : v == 2 || (v == 3 && blink(1000u)) ? 255 : 0;
    }
    for (i = 0; i < NPLAY; i++)                      /* the scene keys: the bank's slots that hold a scene; the */
        if (scene_has(proj.bank, (int)i) &&            /* one the sound is, blinking slowly */
            (ui.scene_at != (int8_t)i || blink(720u)))
            k[WHITE_K[i]] = 255;
    {                                                  /* a scene on its way: its key as bright as the header's line,
                                                        * then blinking once there; gliding back, the left one dims */
        float x = 0.0f;
        int bank, n = lk_glide(now, &x, &bank);
        if (n >= 0 && n < NPLAY && ui.press < 0 && bank == proj.bank)
            k[WHITE_K[n]] = (uint8_t)(40.0f + 215.0f * x);
        if (ui.morph == VISIT_BACK && ui.visit <= -2 && -2 - ui.visit < NPLAY && ui.morph_ms)
            k[WHITE_K[-2 - ui.visit]] = (uint8_t)(255.0f - 215.0f * fm_clampf((float)(now - ui.morph_t0) /
                                                                              (float)ui.morph_ms, 0.0f, 1.0f));
    }
    if (ui.press >= 0)                                 /* the key pressed into: as bright as how far in */
        k[WHITE_K[ui.press]] = (uint8_t)(40.0f + 215.0f * ui.press_x);
    if (ui.flash_t && now - ui.flash_t < 480u && ui.scene_at >= 0)   /* taken: three quick flashes */
        k[WHITE_K[ui.scene_at]] = (now - ui.flash_t) % 160u < 80u ? 255 : 0;
    else if (ui.flash_t && ui.press >= 0 && now - ui.flash_t < 480u)
        k[WHITE_K[ui.press]] = (now - ui.flash_t) % 160u < 80u ? 255 : 0;
    for (i = 0; i < DR_NGEST; i++)                   /* the gestures: a dim glow, lit as far as each is in */
        k[gkey((int)i)] = (uint8_t)(40.0f + 215.0f * ui.g_amt[i]);
    if (keymap_on())                                   /* SEL held, the map on screen: the eight gesture keys are
                                                        * the banks, the one playing lit (as the map draws them) */
        for (i = 0; i < 8u; i++)
            k[WHITE_K[NPLAY + i]] = proj.bank == KEY_BANK[i] ? 255 : LED_DIM;
    boxed_blink(b, k);
    plat_leds(b, k);
}

/* AUTOSAVE: a few seconds after the last change, while the drone is stopped and nothing is touched. "The last
 * change", not the last touch: a scene still gliding (or the loop playing) changes values every few frames, and
 * each would be a flash write (Evan's FM-1, 2026-10-19: a long scene switch wrote 660 times in 20 s, the lights
 * flickering with each). And never more often than AUTOSAVE_EVERY, whatever happens, nor again soon after a fail */
#define AUTOSAVE_QUIET 4000u
#define AUTOSAVE_EVERY 30000u
#define AUTOSAVE_TAIL 15000u                   /* the tails still heard this long after the last touch (a long reverb,
                                                * a freeze): saved with the dip, rather than not at all */
static void autosave(void)
{
    uint32_t now = plat_ms();
    if (!ui.dirty || ui.playing || dr_fade > 0.0f || ui.btn || ui.keys || now - ui.act_t < AUTOSAVE_QUIET ||
        now - ui.chg_t < AUTOSAVE_QUIET || ui.morph >= 0 || ui.press >= 0 ||
        (ui.saved_t && now - ui.saved_t < AUTOSAVE_EVERY) || plat_store_held() || ui.save_q != -2 ||
        (dr_tail >= TAIL_HEARD && now - ui.act_t < AUTOSAVE_TAIL))   /* the tails fade out first (a save cut in
                                                                        * them popped, Evan 2026-11-07) */
        return;
    ui.saved_t = now;
    save_ask(SAVE_AUTO);                       /* (still heard: with the dip) */
}

void ui_init(void)
{
    memset(&ui, 0, sizeof ui);
    ui.hold_btn = -1;
    ui.morph = -1;
    ui.visit = -1;
    ui.press = -1;
    ui.scene_at = -1;
    ui.km_key = -1;
    ui.jack = dr_phones = (uint8_t)plat_headphones();
    ui.save_q = -2;
    ui.splash_t0 = plat_ms();
    lcd_fill(0, 0, 240, 240, K_BG);
}

/* ------------------------------------------------------------- the start --- */
/* As the web page: a red field, two thick bands (violet, and one that darkens what it crosses) drifting in from
 * the right and settling across the name, which turns pale pink where they cross it; it holds still for half a
 * second where it lands, then fades to the Drones page slowly. About 3.5 s (the last 1.3 s the fade); any button
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

#include "look.c"                             /* 0.3's new look: every page, the header, the key map */

/* ------------------------------------------------------ screen reader --- */
/* The screen as text for a screen reader on the computer (announce.c, Charles Vestal's fm1-reader; issue #87): a page
 * once it settles, with its knobs; a knob's value as it turns; the banners as they show; PLAY and stop; the sound
 * a scene or a Preset takes it to; a gesture coming in. WHERE (the reader asks, or it has just started listening) is
 * everything on the screen. The words are the screen's own, said in full by the words file (firmware/words.tsv,
 * published with the site). Nothing is built while no reader listens. */
static struct {
    uint32_t ctx;                              /* the page, hashed: a change is said */
    int16_t val[4];                            /* the four knobs' values said last */
    int16_t at;                                /* the sound (lk_at) said last */
    int8_t gest, playing, km, morph;
    uint8_t started, said, map;
    uint32_t hint[3];                          /* each drone's key hints and the page's, hashed: a key press is said */
    char glab[2][16];
} sp;

static int sp_eq(const char *a, const char *b)  /* the same text (the device's libc has no strcmp) */
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}
static uint32_t sp_hint_sig(int g);
#define SP_MAX (2u * ANN_MAX)                 /* a page is built whole, then sent in one or two messages */
static char sp_buf[SP_MAX + 1], sp_rest[ANN_MAX + 1];   /* (sp_rest: a long page's second half, said after) */
static uint32_t sp_n;

static void sp_cat(const char *s)              /* the screen's text as plain ASCII: "[SEL]" a word, "·" a comma */
{
    const uint8_t *u = (const uint8_t *)s;
    while (*u && sp_n < SP_MAX) {
        const char *w = 0;
        if (*u == '[' || *u == ']') {
            u++;
            continue;
        }
        if (u[0] == 0xC2 && u[1] == 0xB7) {         /* · */
            while (sp_n && sp_buf[sp_n - 1] == ' ')
                sp_n--;
            w = ",";
            u += 2;
        } else if (u[0] == 0xE2 && u[1] == 0x86) {  /* → ↑ ↓ */
            w = u[2] == 0x92 ? "to" : u[2] == 0x91 ? "up" : u[2] == 0x93 ? "down" : "";
            u += 3;
        } else if (*u >= 0x80) {
            u++;
            continue;
        }
        if (w) {
            while (*w && sp_n < SP_MAX)
                sp_buf[sp_n++] = *w++;
            continue;
        }
        sp_buf[sp_n++] = (char)*u++;
    }
    sp_buf[sp_n] = 0;
}
static void sp_start(void) { sp_n = 0; sp_buf[0] = 0; }

/* a value as words can take it, a word at a time: a number and its unit apart ("2.1 Hz", "-12 dB"; "5th", "8ves" kept);
 * minutes ("1m30": "1 minutes 30"); a note's flat or sharp in words ("E flat 2": the words file ignores capitals, so it
 * can't tell "Bb" from "BB"); an interval's minor or major ("m3" from "M3", the same) and its way ("+5th": "5th up") */
static void sp_word(const char *s, uint32_t len)
{
    static const char *const ORD[] = {"", "", "second", "third", "fourth", "fifth", "sixth", "seventh", "octave",
                                      "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth"};
    char o[40];
    uint32_t n = 0, i = 0;
    uint32_t dg = 1;                                   /* an interval: a sign, then a word or an ordinal ("+5th") */
    while (dg < len && s[dg] >= '0' && s[dg] <= '9')
        dg++;
    const char sign = len > 1u && (s[0] == '+' || s[0] == '-') && s[1] != '.' &&
                      (dg == 1u || (len - dg == 2u && ((s[dg] == 't' && s[dg + 1] == 'h') || (s[dg] == 's' && s[dg + 1] == 't') ||
                                                      (s[dg] == 'n' && s[dg + 1] == 'd') || (s[dg] == 'r' && s[dg + 1] == 'd'))))
                          ? s[0] : 0;
    if (sign)
        i = 1;
    {   /* a step of an equal Tuning, "+11\\19": "11 steps of 19 up" (the xenharmonic writers' notation) */
        const char sg = s[0] == '+' || s[0] == '-' ? s[0] : 0;
        uint32_t j = i = sg ? 1u : 0u;
        while (j < len && s[j] >= '0' && s[j] <= '9')
            j++;
        if (j > i && j + 1u < len && s[j] == '\\') {
            memcpy(o, s + i, j - i);
            o[j - i] = 0;
            sp_cat(o);
            sp_cat(j - i == 1u && s[i] == '1' ? " step of " : " steps of ");
            memcpy(o, s + j + 1, len - j - 1u);
            o[len - j - 1u] = 0;
            sp_cat(o);
            if (sg)
                sp_cat(sg == '+' ? " up" : " down");
            return;
        }
        i = sign ? 1u : 0u;
    }
    if (i + 1u < len && (s[i] == 'm' || s[i] == 'M') && s[i + 1] >= '1' && s[i + 1] <= '9') {   /* m3, M7, m10 */
        int k = s[i + 1] - '0';
        if (i + 2u < len && s[i + 2] >= '0' && s[i + 2] <= '9')
            k = k * 10 + (s[i + 2] - '0');
        sp_cat(s[i] == 'm' ? "minor " : "major ");
        sp_cat(k > 0 && k < 15 ? ORD[k] : "");
    } else if (len >= 4u && s[1] == 'm' && s[0] >= '0' && s[0] <= '9' && s[2] >= '0' && s[2] <= '9') {   /* 1m30 */
        o[0] = s[0];
        o[1] = 0;
        sp_cat(o);
        sp_cat(s[0] == '1' ? " minute" : " minutes");
        if (s[2] != '0' || s[3] != '0') {
            o[0] = ' ';
            o[1] = s[2] == '0' ? s[3] : s[2];
            o[2] = s[2] == '0' ? 0 : s[3];
            o[3] = 0;
            sp_cat(o);
        }
    } else {
        for (; i < len && n < sizeof o - 9u; i++) {
            const char c = s[i], d = i + 1u < len ? s[i + 1] : 0, e = i + 2u < len ? s[i + 2] : 0;
            o[n++] = c;
            if (c >= 'A' && c <= 'G' && d == 'b' && (e == '-' || (e >= '0' && e <= '9'))) {   /* (the reader says sharps) */
                const char *w = " flat ";
                while (*w)
                    o[n++] = *w++;
                i++;
            } else if (c >= '0' && c <= '9' && ((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z'))) {
                const char *r = s + i + 1;   /* an ordinal or "8ve(s)" stays whole */
                const uint32_t rl = len - i - 1u;
                if (!((rl == 2u && ((r[0] == 't' && r[1] == 'h') || (r[0] == 'n' && r[1] == 'd') || (r[0] == 'r' && r[1] == 'd') ||
                                    (r[0] == 'v' && r[1] == 'e'))) ||
                      (rl == 3u && ((r[0] == 'v' && r[1] == 'e' && r[2] == 's') || (r[0] == 't' && r[1] == 'h' && r[2] == 's')))))
                    o[n++] = ' ';
                if (rl == 1u && r[0] == 'c') {    /* cents: in words here ("3 C#4" is a note's C to the reader) */
                    const char *w = "cents";
                    while (*w)
                        o[n++] = *w++;
                    break;
                }
            }
        }
        o[n] = 0;
        sp_cat(o);
    }
    if (sign)
        sp_cat(sign == '+' ? " up" : " down");
}
static void sp_value(const char *s)
{
    while (*s) {
        uint32_t k = 0;
        while (s[k] && s[k] != ' ')
            k++;
        if (k)
            sp_word(s, k);
        s += k;
        if (*s) {
            sp_cat(" ");
            s++;
        }
    }
}

/* a knob's name in full (the cards abbreviate to fit: the words file says the rest) */
static const char *sp_name(int p, int second)
{
    if (second)
        return p == P_SUBW_0 || p == P_SUBW_1 ? "Sub wave" : p == P_DLY_TYPE ? "Delay type" : "Into reverb";
    if (p < P_XMOD && (p % GP_N == GP_OLFO_DEPTH || p % GP_N == GP_FLFO_DEPTH))
        return card_name(p);                   /* LFO sends: named for what they move */
    switch (p) {
    case P_PLINK_OFF: return "Link offset";
    case P_SHIFT_MOVE: return "Shift";
    case P_SCENE_T: case P_SCENE_BARS: return "Scene time";
    case P_REV_TYPE: return "Reverb type";
    case P_DLY_TIME: case P_DLY_NOTE: return "Delay time";
    case P_DLY_DRIVE: return "Delay drive";
    case P_PULSE_DIV: return "Rate";
    default: return dr_param_info(p)->name;
    }
}

/* knob i on the page shown: "Cutoff 57.5", "Interval +5th just" (its value and the word under it) */
static int sp_short;                           /* a page's sentence on LFO sends: the values without their second
                                                * line (said as a knob turns), so it fits one message with its keys */
static void sp_knob(int i)
{
    const int p = knob_param(i), second = (ui.btn >> B_SEL & 1u) && sel_job(ui.view, i) >= 0;
    char t[16], t2[16];
    if (p < 0)
        return;
    sp_cat(sp_name(p, second));
    sp_cat(" ");
    lk_text(p, proj.par[p], t);
    if (!second && lk_line2(p, proj.par[p], t, t2) && t2[0] && !sp_short) {
        sp_value(t);
        sp_cat(" ");
        sp_value(t2);
    } else {
        sp_value(t);
    }
}

/* a scene's name as said: "Preset 3 Vespers", "scene B 5" (the screen's "P3", "B5"; "A5" is a note too) */
static void sp_scene(int bank, int n)
{
    char t[4];
    sp_cat(BANK_IS_PRESETS(bank) ? "Preset " : "scene ");
    if (!BANK_IS_PRESETS(bank)) {
        t[0] = BANK_L[(unsigned)bank < NBANKS ? bank : 0];
        t[1] = ' ';
        t[2] = 0;
        sp_cat(t);
    }
    itoa_u((uint32_t)n + 1u + (bank == BANK_PRESETS2 ? NPLAY : 0u), t);
    sp_cat(t);
    if (BANK_IS_PRESETS(bank)) {
        sp_cat(" ");
        sp_cat(preset_name(n + (bank == BANK_PRESETS2 ? NPLAY : 0)));
    }
}

/* what a black key does now, said after its key: "A#3 for band-pass", "F#3 for off" (Evan, 2026-11-10: a
 * key's name alone didn't say what it does). The next press's result, as sw_flip makes it */
static const char *const SP_SUB[7] = {"", "1 octave down", "2 octaves down", "one third", "one fifth", "3 octaves down",
                                      "4 octaves down"};
static void sp_does_k(int k, int key)
{
    static const char *const MODE[3] = {"low-pass", "band-pass", "high-pass"};
    static const char *const WAVES[4] = {"no LFO", "LFO I", "LFO II", "LFO I+II"};
    static const char *const XM[4] = {"off", "cutoff", "pitch", "Snarl"};
    const sw_t w = sw_layer()[k];
    const int16_t *p = &proj.par[DR_GP(w.g, 0)];
    const int on = sw_get(w);
    if (key)
        sp_value(LK_BKEY[k]);
    switch (w.s) {
    case S_PARTNER: case S_PLOCK: case S_PLINK: case S_SYNC: sp_cat(on ? " for off" : " for on"); break;
    case S_SUBOCT: sp_cat(" for "); sp_cat(SP_SUB[p[GP_SUBOCT] % 6 + 1]); break;
    case S_HP: sp_cat(" for "); sp_cat(MODE[(p[GP_MODE] + 1) % 3]); break;
    case S_OTAP: case S_FTAP: case S_DTAP: {
        const int v = w.s == S_OTAP ? p[GP_OLFO] : w.s == S_FTAP ? p[GP_FLFO] : proj.par[P_DLY_LFO];
        sp_cat(" for ");
        sp_cat(WAVES[v == DR_LFO_I ? DR_LFO_II : v == DR_LFO_II ? DR_LFO_BOTH : DR_LFO_I]);
        break;
    }
    case S_ODEST: sp_cat(on ? " for pitch" : " for x-wave"); break;
    case S_FDEST: sp_cat(on ? " for cutoff" : " for reso"); break;
    case S_XMOD: sp_cat(" for "); sp_cat(XM[(proj.par[P_XMOD] + 1) % (DR_XMOD_OUT + 1)]); break;
    case S_LFORESET: sp_cat(" restarts the LFO"); break;
    default: break;
    }
}
static void sp_does(int k) { sp_does_k(k, 1); }
/* a black key's setting and its state now: "drone 1 filter, band-pass" (the key map) */
static void sp_setting(int k)
{
    static const char *const MODE[3] = {"low-pass", "band-pass", "high-pass"};
    const sw_t w = sw_layer()[k];
    const char *name, *st;
    sw_text(w, &name, &st);
    if (w.s == S_PARTNER || w.s == S_SUBOCT || w.s == S_HP || w.s == S_OTAP || w.s == S_ODEST || w.s == S_FTAP ||
        w.s == S_FDEST)
        sp_cat(w.g ? "drone 2 " : "drone 1 ");
    sp_cat(name);
    sp_cat(", ");
    if (w.s == S_HP)
        sp_cat(MODE[(unsigned)proj.par[DR_GP(w.g, GP_MODE)] < 3u ? proj.par[DR_GP(w.g, GP_MODE)] : 0]);
    else if (w.s == S_SUBOCT)
        sp_cat(SP_SUB[(unsigned)proj.par[DR_GP(w.g, GP_SUBOCT)] < 7u ? proj.par[DR_GP(w.g, GP_SUBOCT)] : 1]);
    else
        sp_value(st);
}

/* a page's key hints, as its picture draws them beside each drone (g 0, 1) or for the page (g 2): "LP filter, key
 * A#3" (Evan, 2026-11-10: in the page, after the knobs). Returns 0 when the page has none there */
static int sp_hints(int g)
{
    static const char *const MODE[3] = {"low-pass", "band-pass", "high-pass"};
    const int v = ui.view;
    if (g < G && v == V_PLAY) {
        sp_cat(MODE[(unsigned)gpar(g, GP_MODE) < 3u ? gpar(g, GP_MODE) : 0]);
        sp_cat(" filter, ");
        sp_does(g ? 7 : 2);
    } else if (g < G && v == V_SOURCE) {
        sp_cat("sub ");
        sp_cat(SP_SUB[(unsigned)gpar(g, GP_SUBOCT) < 7u ? gpar(g, GP_SUBOCT) : 1]);
        sp_cat(", ");
        sp_does(g ? 6 : 1);
    } else if (g < G && v == V_PARTNER) {
        sp_cat(gpar(g, GP_PARTNER) ? "partner on, " : "partner off, ");
        sp_does(g ? 5 : 0);
    } else if (g < G && v == V_LFO) {
        int d;
        for (d = 0; d < 2; d++) {              /* "pitch from LFO I, F#3 changes it to LFO II, G#3 switches it to x-wave" */
            const int route = gpar(g, d ? GP_FLFO : GP_OLFO);
            if (d)
                sp_cat("; ");
            sp_cat(d ? (gpar(g, GP_FLFO_DEST) == DR_TO_RESO ? "reso" : "cutoff")
                     : (gpar(g, GP_OLFO_DEST) == DR_TO_XWAVE ? "x-wave" : "pitch"));
            sp_cat(route == DR_LFO_BOTH ? " from LFO I+II, " : route == DR_LFO_II ? " from LFO II, " : route ? " from LFO I, "
                                                                                                        : ", no LFO, ");
            sp_does(g * 5 + (d ? 2 : 0));
            sp_cat(", ");
            sp_does(g * 5 + (d ? 3 : 1));
        }
    } else if (g == G && v == V_LFO2) {
        static const char *const DOES[4] = {"off", "drone 1 moves drone 2's cutoff", "the drones bend each other",
                                            "Snarl"};
        const int xm = proj.par[P_XMOD];
        sp_cat("Cross-mod ");
        sp_cat(DOES[(unsigned)xm < 4u ? xm : 0]);
        sp_cat(", ");
        sp_does(10);
    } else if (g == G && v == V_MOVES) {
        sp_cat("E5 plays Shift");
    } else {
        return 0;
    }
    return 1;
}

/* the page: its name, then its knobs, drone by drone; all: PLAY's state, the sound, what's running */
static void sp_screen(int all)
{
    int i;
    sp_start();
    sp_cat(ui.view == V_SOURCE ? "Sub and Drive" : VIEW_NAME[ui.view]);   /* (the screen's "·") */
    sp_cat(" page");
    for (i = 0; i < 4; i++) {
        const knob_t k = VIEW_KNOB[ui.view][i];
        if (knob_param(i) < 0)
            continue;
        if (k.g < G && (i == 0 || VIEW_KNOB[ui.view][i - 1].g != k.g))
            sp_cat(k.g ? ". Drone 2: " : ". Drone 1: ");
        else
            sp_cat(i ? ", " : ": ");
        sp_short = ui.view == V_LFO;           /* (LFO sends: the values alone; their depth as a knob turns) */
        sp_knob(i);
        sp_short = 0;
        if (all && k.g < G && (i == 3 || VIEW_KNOB[ui.view][i + 1].g != k.g)) {   /* WHERE: the drone's key hints after
                                                * its knobs (a page opening says its knobs only: Evan, 2026-11-10) */
            const uint32_t n0 = sp_n;
            sp_cat(", ");
            if (!sp_hints(k.g)) {
                sp_n = n0;
                sp_buf[sp_n] = 0;
            }
        }
    }
    sp_cat(".");
    if (all) {
        const uint32_t n0 = sp_n;
        sp_cat(" ");
        if (sp_hints(G))
            sp_cat(".");
        else {
            sp_n = n0;
            sp_buf[sp_n] = 0;
        }
    }
    if (!all)
        return;
    sp_cat(ui.playing ? " Playing" : " Stopped");
    lk_sound_at();
    if (lk_at >= 0) {
        sp_cat(", ");
        sp_scene(lk_at >> 4, lk_at & 15);
        if (lk_chg)
            sp_cat(", changed");
    } else if (ui.morph >= 0 && ui.morph < NPLAY) {
        sp_cat(", on its way to ");
        sp_scene(proj.bank, ui.morph);
    }
    sp_cat(".");
    if (proj.par[P_PULSE])
        sp_cat(" Swell on.");
    i = lk_gesture();
    if (i >= 0) {
        sp_cat(" ");
        sp_cat(GESTURE[i]);
        sp_cat(proj.par[gl_par(i)] ? " latched." : " on.");
    }
    if (proj.par[P_II_OUT])
        sp_cat(" Drone 2 out.");
    if (ui.loop != L_IDLE)
        sp_cat(ui.loop == L_REC ? " Recording a loop." : " Loop playing.");
}

/* a banner's word as said: the drones' numerals as numbers ("II" 2), its arrows in words, a scene's key apart */
static void sp_msg_word(const char *s, uint32_t k, int first)
{
    char w[24];
    if (k >= sizeof w)
        k = sizeof w - 1u;
    memcpy(w, s, k);
    w[k] = 0;
    if (sp_eq(w, "I") || sp_eq(w, "II"))
        sp_cat(first ? (k == 1u ? "Drone 1" : "Drone 2") : k == 1u ? "1" : "2");
    else if (sp_eq(w, "I+II"))
        sp_cat("1 and 2");
    else if (sp_eq(w, ">") || sp_eq(w, "\\"))
        sp_cat("to");
    else if (sp_eq(w, "<>"))
        sp_cat("and");
    else if (k == 2u && w[0] >= 'A' && w[0] <= 'F' && w[1] >= '1' && w[1] <= '8') {   /* a scene's key: "A1" */
        char t[4] = {w[0], ' ', w[1], 0};
        sp_cat(first ? "scene " : "");
        sp_cat(t);
    } else
        sp_word(w, k);
}
/* a banner: its two lines as one sentence. One that repeats its first line (a pressure blend's "A1 41%") is a readout:
 * said once it settles */
static void sp_msg(const char *a, const char *b)
{
    static char last[24];
    static uint32_t last_t;
    const uint32_t now = plat_ms();
    const char *l;
    int i, first = 1, run;
    if (!ann_on() || !a || !*a)
        return;
    run = sp_eq(last, a) && now - last_t < 1500u && b && *b;
    for (i = 0; i < 23 && a[i]; i++)
        last[i] = a[i];
    last[i] = 0;
    last_t = now;
    sp_start();
    for (i = 0; i < 2; i++) {
        l = i ? b : a;
        if (!l || !*l)
            continue;
        if (i)
            sp_cat(" ");
        while (*l) {
            uint32_t k = 0;
            while (l[k] && l[k] != ' ')
                k++;
            if (k)
                sp_msg_word(l, k, first);
            first = 0;
            l += k;
            while (*l == ' ')
                l++;
            if (*l)
                sp_cat(" ");
        }
    }
    if (run)
        ann_later(sp_buf, ANN_NOW, 200u, now);
    else
        ann_say(sp_buf, 0, ANN_NOW);
    sp.said = 1;
}

/* the text built, said: in one message, or a long page in two (at ". Drone 2", or the last sentence that fits), the
 * second said once the first has gone (now: at once; later: once the page settles, as ann_later) */
static void sp_split(void)                     /* sp_buf to its first message, sp_rest the second (or none) */
{
    uint32_t cut = sp_n, i;
    sp_rest[0] = 0;
    if (sp_n > ANN_MAX) {
        for (i = 0, cut = 0; i + 9u < sp_n && i < ANN_MAX; i++)
            if (sp_buf[i] == '.' && sp_buf[i + 1] == ' ') {
                cut = i + 1u;
                if (sp_buf[i + 2] == 'D' && sp_buf[i + 8] == '2')   /* ". Drone 2" */
                    break;
            }
        if (!cut)
            cut = ANN_MAX;
        for (i = cut; sp_buf[i] == ' '; i++)
            ;
        memcpy(sp_rest, sp_buf + i, sp_n - i > ANN_MAX ? ANN_MAX : sp_n - i);
        sp_rest[sp_n - i > ANN_MAX ? ANN_MAX : sp_n - i] = 0;
        sp_buf[cut] = 0;
        sp_n = cut;
    }
}
static void sp_send(int later)
{
    const uint32_t now = plat_ms();
    sp_split();
    if (later)
        ann_later(sp_buf, ANN_NOW, 200u, now);
    else {
        ann_say(sp_buf, 0, ANN_NOW);
        if (sp_rest[0]) {
            ann_say(sp_rest, 0, 0);
            sp_rest[0] = 0;
        }
    }
}
static uint32_t sp_hint_sig(int g)            /* the hint's text, hashed (built aside: sp_buf is kept) */
{
    char keep[ANN_MAX + 1];
    const uint32_t kn = sp_n;
    uint32_t h = 2166136261u, i;
    memcpy(keep, sp_buf, kn + 1u);
    sp_start();
    if (sp_hints(g))
        for (i = 0; i < sp_n; i++)
            h = hash(h, (uint8_t)sp_buf[i]);
    memcpy(sp_buf, keep, kn + 1u);
    sp_n = kn;
    return h;
}
static uint32_t sp_ctx_sig(void)
{
    return hash(hash(2166136261u, ui.view),
                (uint32_t)(page_jobs(ui.view) && (ui.btn >> B_SEL & 1u)) | (uint32_t)proj.par[P_PLINK] << 1);
}
static void sp_snap(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        const int p = knob_param(i);
        sp.val[i] = (int16_t)(p < 0 ? 0 : proj.par[p]);
    }
    sp.playing = (int8_t)ui.playing;
    sp.at = ui.morph >= 0 && ui.morph < NPLAY ? (int16_t)(proj.bank << 4 | ui.morph) : lk_at;   /* (its end: said) */
    sp.morph = ui.morph;
    sp.gest = (int8_t)lk_gesture();
    memcpy(sp.glab, ui.glab, sizeof sp.glab);
    for (i = 0; i < 3; i++)
        sp.hint[i] = sp_hint_sig(i);
}

static void speak(void)
{
    const uint32_t now = plat_ms();
    uint32_t c;
    int where = ann_where(), i, changed = -1, g;
    if (!ann_on()) {
        sp.started = 0;
        sp.said = 0;
        return;
    }
    lk_sound_at();
    if (sp_rest[0] && !ann.later_set) {              /* a long page's second half, once its first has been said */
        ann_say(sp_rest, 0, 0);
        sp_rest[0] = 0;
    }
    if (!sp.started || where) {                       /* everything on the screen */
        sp.started = 1;
        sp.ctx = sp_ctx_sig();
        sp_snap();
        sp_screen(1);
        sp_send(0);
        sp.said = 0;
        return;
    }
    if (sp.said) {                                    /* a banner said what a key did (and what it changed) */
        sp.said = 0;
        sp_snap();
        return;
    }
    if (keymap_on() != sp.map || (sp.map && ui.km_key != sp.km)) {   /* SEL held: the key map, a key touched */
        sp.map = (uint8_t)keymap_on();
        sp.km = ui.km_key;
        if (sp.map) {
            sp_start();
            if (ui.km_key < 0)
                sp_cat("Keys, SEL held: touch a black key to hear what it does");
            else {                                    /* "A#3: drone 1 filter, band-pass. A#3 steps it to high-pass" */
                sp_value(LK_BKEY[ui.km_key]);
                sp_cat(": ");
                sp_setting(ui.km_key);
                sp_cat(". Press");
                sp_does_k(ui.km_key, 0);     /* " for high-pass" (its key not again) */
            }
            ann_later(sp_buf, ANN_NOW, 120u, now);
        }
        return;
    }
    if (ui.playing != (uint8_t)sp.playing) {
        sp.playing = (int8_t)ui.playing;
        ann_say(ui.playing ? "Play" : "Stop", 0, ANN_NOW);
    }
    c = sp_ctx_sig();
    if (c != sp.ctx) {                                /* a new page (or SEL's second jobs): said when it settles */
        sp.ctx = c;
        sp_snap();
        sp_screen(0);
        sp_send(1);
        return;
    }
    for (i = 0; i < 3; i++) {                         /* a key changed what a page's hint shows (no banner said it) */
        const uint32_t h = sp_hint_sig(i);
        if (h != sp.hint[i]) {
            sp.hint[i] = h;
            sp_start();
            if (i < G)
                sp_cat(i ? "Drone 2 " : "Drone 1 ");
            sp_hints(i);
            ann_say(sp_buf, 0, ANN_NOW);
            return;
        }
    }
    for (i = 0; i < 4; i++) {                         /* a knob turning */
        const int p = knob_param(i), v = p < 0 ? 0 : proj.par[p];
        if (v != sp.val[i]) {
            sp.val[i] = (int16_t)v;
            if (changed < 0 && lit(i))                /* a hand on it (not a scene gliding it) */
                changed = i;
        }
    }
    if (changed >= 0) {
        sp_rest[0] = 0;                               /* (a page's second half: a hand moved on) */
        sp_start();
        if (VIEW_KNOB[ui.view][changed].g < G)
            sp_cat(VIEW_KNOB[ui.view][changed].g ? "Drone 2 " : "Drone 1 ");
        sp_knob(changed);
        ann_later(sp_buf, ANN_NOW, 120u, now);
        return;
    }
    if (ui.glab[0][0] && now < ui.glab_until && memcmp(sp.glab, ui.glab, sizeof sp.glab)) {   /* the top bar's readout */
        memcpy(sp.glab, ui.glab, sizeof sp.glab);
        sp_start();
        sp_cat(ui.glab[0]);
        sp_cat(" ");
        sp_cat(ui.glab[1]);
        ann_later(sp_buf, ANN_NOW, 120u, now);
        return;
    }
    if (ui.morph != sp.morph) {                       /* on its way to a scene or a Preset (the header's "→ P3") */
        sp.morph = ui.morph;
        if (ui.morph >= 0 && ui.morph < NPLAY) {
            sp_start();
            sp_cat("to ");
            sp_scene(proj.bank, ui.morph);
            ann_say(sp_buf, 0, ANN_NOW);
            sp.at = (int16_t)(proj.bank << 4 | ui.morph);   /* (arriving says nothing more) */
            return;
        }
    }
    if (lk_at != sp.at && ui.morph < 0) {             /* a scene or a Preset taken: the sound's name */
        sp.at = lk_at;
        if (lk_at >= 0) {
            sp_start();
            sp_scene(lk_at >> 4, lk_at & 15);
            ann_say(sp_buf, 0, 0);
        }
    }
    g = lk_gesture();
    if (g != sp.gest) {                               /* a gesture coming in (its % is the header's to show) */
        sp.gest = (int8_t)g;
        if (g >= 0) {
            sp_start();
            sp_cat(GESTURE[g]);
            sp_cat(proj.par[gl_par(g)] ? " latched" : "");
            ann_say(sp_buf, 0, ANN_NOW);
        }
    }
}

#ifdef OM_HOST
/* every page, knob value and screen as a screen reader gets them (the "speakdump" command): KIND <tab> the text sent
 * (fm1-reader's tools/audit.mjs puts it in words with web/words/ and checks it). Each knob from its lowest value to its
 * highest; with SEL held, the second jobs; WHERE stopped and playing, on a Preset and a scene of bank A */
static void sp_print(const char *kind, const char *t) { printf("%s\t%s\n", kind, t); }
static void speak_dump(void)
{
    const uint8_t keep_view = ui.view, keep_play = ui.playing;
    const uint32_t keep_btn = ui.btn;
    const int16_t keep_at = lk_at;
    project_t keep = proj;
    int v, s, i, x, lo, hi, step;
    for (v = 0; v < NVIEWS; v++)
        for (s = 0; s < 2; s++) {
            ui.view = (uint8_t)v;
            ui.btn = s ? keep_btn | 1u << B_SEL : keep_btn & ~(1u << B_SEL);
            if (s && !page_jobs(v))
                continue;
            sp_screen(0);
            sp_split();
            sp_print("SCREEN", sp_buf);
            if (sp_rest[0])
                sp_print("SCREEN", sp_rest);
            for (i = 0; i < 4; i++) {
                const int p = knob_param(i);
                if (p < 0 || (s && sel_job(v, i) < 0))
                    continue;
                lo = dr_param_info(p)->lo;
                hi = dr_param_info(p)->hi;
                step = hi - lo > 2000 ? 7 : 1;          /* the pitches: a note in every few cents is enough */
                for (x = lo; x <= hi; x += step) {
                    proj.par[p] = (int16_t)x;
                    sp_start();
                    sp_knob(i);
                    sp_print("KNOB", sp_buf);
                }
                proj = keep;
            }
        }
    ui.btn = keep_btn;
    for (x = 0; x < 4; x++) {                         /* WHERE: stopped and playing, a Preset and a scene of A */
        ui.view = V_PLAY;
        ui.playing = (uint8_t)(x & 1);
        lk_at = (int16_t)(x < 2 ? BANK_PRESETS << 4 | 2 : 0 << 4 | 4);
        proj.par[P_PULSE] = (int16_t)(x & 1);
        sp_screen(1);
        sp_split();
        sp_print("WHERE", sp_buf);
        if (sp_rest[0])
            sp_print("WHERE", sp_rest);
    }
    for (x = 0; x < 2 * NPLAY; x++) {                 /* each Preset's name, and bank B's scenes, as a tap says them */
        sp_start();
        sp_scene(x < NPLAY ? BANK_PRESETS : BANK_PRESETS2, x % NPLAY);
        sp_print("SOUND", sp_buf);
        sp_start();
        sp_scene(1, x % NPLAY);
        sp_print("SOUND", sp_buf);
    }
    for (x = 0; x < DR_NGTYPE; x++) {
        sp_print("GESTURE", GESTURE[x]);
    }
    proj = keep;
    lk_at = keep_at;
    ui.view = keep_view;
    ui.playing = keep_play;
}
#endif

void ui_frame(void)
{
    input();
    press_run();
    gesture_run();
    tips_run();
    scene_morph();
    autosave();
    if (!ui.splash_done) {
        uint32_t t = plat_ms() - ui.splash_t0;
        if (ui.btn || ui.keys || t >= SPLASH_MS) {                    /* over, or a hand on the FM-1 */
            ui.splash_done = 1;
            ui.sig[0] = ui.sig[1] = ui.sig[2] = 0;                   /* every band drawn afresh */
            lcd_fill(0, 0, 240, 240, LK_BG);
            if (proj_note_moved || proj_note_stored) {   /* updated to 0.3: keys 9-12 are gestures now */
                char t[24], *q = t;
                itoa_u((uint32_t)proj_note_moved, t);
                while (*q)
                    q++;
                scopy(q, proj_note_moved == 1 ? " SCENE TO KEYS 1-8" : " SCENES TO KEYS 1-8");
                say_c(t, 0, K_GOLD);
                if (proj_note_stored) {
                    itoa_u((uint32_t)proj_note_stored, ui.msg_next);
                    for (q = ui.msg_next; *q; q++)
                        ;
                    scopy(q, " KEPT, UNPLAYED");
                } else if (proj_note_presets)
                    scopy(ui.msg_next, "0.2 PRESETS [SEL]+[E5]");
                ui.msg_until = plat_ms() + 3500u;
            } else if (proj_note_presets) {      /* from 0.2: its dropped Presets are in bank F (SEL + E5) */
                say_c("0.2 PRESETS [SEL]+[E5]", 0, K_GOLD);
                ui.msg_until = plat_ms() + 3500u;
            }
        } else {
            draw_splash(t);
            leds();
            ui.frame++;
            return;
        }
    }
    lk_frame();
    leds();
    speak();
    ann_tick(plat_ms());
    ui.frame++;
}

void ui_input_only(void) { input(); }
