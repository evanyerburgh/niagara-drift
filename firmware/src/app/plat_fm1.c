/* SPDX-License-Identifier: GPL-3.0-only */
/* OMNI platform on the FM-1: plat.h over Felucca's HAL, panel map, USB rings and storage.
 * Part of the unity build (main.c), after the HAL, usb.c, storage.c and panel.c. */

uint32_t plat_ms(void) { return fm1_ms; }

uint32_t plat_buttons(void)
{
    uint32_t raw = fm1_in.buttons, out = 0, i;
    for (i = 0; i < NB; i++)
        if (raw & (1u << panel.btn[i]))
            out |= 1u << i;
    return out;
}

uint32_t plat_keys(void) { return fm1_in.notes & ((1u << NKEYS) - 1u); }

int32_t plat_enc(int role)
{
    if (role < 0 || role >= NE)
        return 0;
    return fm1_enc_take(panel.enc[role]) * panel.dir[role];
}

static volatile uint32_t master_q12 = 2048;
uint32_t plat_master(void) { return master_q12; }

/* LEDs: the picture is built off-line and copied one byte per column (Felucca: clearing
 * and relighting would let the 10 kHz scan catch the dark gap and flicker) */
static uint8_t led_pos[FM1_NKEY];
static void led_pos_init(void)
{
    uint32_t id, p, r;
    for (id = 0; id < FM1_NKEY; id++) {
        led_pos[id] = 0xFF;
        for (p = 0; p < FM1_NCOL; p++)
            for (r = 1; r < 5u; r++)
                if (FM1_KEYMAP[r][p] == (int8_t)id)
                    led_pos[id] = (uint8_t)((p << 3) | r);
    }
}
void plat_leds(uint32_t buttons, uint32_t keys)
{
    uint8_t nl[FM1_NCOL] = {0};
    uint32_t i, c;
    for (i = 0; i < NB + NKEYS; i++) {
        uint32_t id = i < NB ? panel.btn[i] : 14u + (i - NB);
        int on = i < NB ? (buttons >> i) & 1u : (keys >> (i - NB)) & 1u;
        uint8_t q = led_pos[id];
        if (on && q != 0xFF)
            nl[q >> 3] |= (uint8_t)(1u << (q & 7u));
    }
    for (c = 0; c < FM1_NCOL; c++)
        fm1_led[c] = nl[c];
}

/* KEY PRESSURE (aftertouch), MONK-1's (Charles Vestal), from Velo's sensing (sense.c). The scan tick hands
 * each sample of the two analog rows (PA7, PA8: the 20 lower keys, F3 .. C5) to sense.c; the seven top keys
 * (C#5 .. G5, row PA6) have no converter and no pressure. Nothing plays from it yet: the digital scan still
 * says which keys are down. */
static int plat_analog_led_ok(uint32_t col, uint32_t rowbit)   /* a held key: its column's LEDs dark while it's read */
{
    int id = FM1_KEYMAP[rowbit][col];
    uint32_t k = id >= 14 ? (uint32_t)id - 14u : 0xFFu;
    return k >= NKEYS || sn_key[k].st == SN_OPEN;
}
static void plat_analog_sample(uint32_t col, uint32_t rowbit, uint32_t v, uint32_t driven)
{
    int id = FM1_KEYMAP[rowbit][col];
    if (id >= 14)
        sense_sample((uint32_t)id - 14u, v, fm1_ticks(), (int)driven);
}
static int plat_analog_drive(uint32_t col, uint32_t rowbit)
{
    int id = FM1_KEYMAP[rowbit][col];
    return id >= 14 && sense_want_drive((uint32_t)id - 14u);
}
static uint32_t plat_analog_mask(void)
{
    uint32_t m = 0, c, r;
    for (c = 0; c < FM1_NCOL; c++)
        for (r = 3; r <= 4u; r++)
            if (FM1_KEYMAP[r][c] >= 14)
                m |= 1u << (FM1_KEYMAP[r][c] - 14);
    return m;
}
/* Velo's settings as MONK-1 ships them: the driven reading at DRIVE 2, the deeper of two samples (WAIT 2),
 * no calibration; converter divider 7, delay 2; no top-row probe (it gives a velocity, never a pressure) */
static void plat_pressure_init(void)
{
    sense_init(plat_analog_mask());
    sn_cfg.touch = 900;
    sn_cfg.press = 400;
    sn_cfg.release = 600;
    sn_cfg.wait = 2;
    sn_cfg.v_lo = 0;
    sn_cfg.v_hi = 72;
    sn_cfg.vmax = 127;
    sn_cfg.at_range = 32;
    sn_cfg.drive = 2;
    sn_cfg.d_curve = 70;
    sn_cfg.d_ceil = 896;
    sn_cfg.d_at_range = 64;
    sn_cfg.learn = 0;
    sense_cal_reset();
    fm1_an.immediate = 0;
    fm1_an.drive = 2;
    fm1_an.probe = 0;
    fm1_an.con = (uint16_t)FM1_ADC_CON(7, 2);
}
/* The pressure on an absolute scale (v0.2: MONK-1's, counted from where the key rests, drifted after a held
 * press and read a fast hard press as 0, on Evan's FM-1): the pill's resistance (the driven reading less the
 * diode, SN_DBASE) on a log scale, conductive rubber's resistance falling as a power of the force: 0 at
 * PR_LIGHT (a light touch that seats), 100 % at PR_FIRM (a firm press). Smoothed over ~30 ms. Every 5 ms
 * from the main loop. For measuring, over USB: the pressure as polyphonic aftertouch (the key as a note,
 * F3 = 53), and the raw driven reading / 8 as CC (the key's number, 0..26) on channel 16; both 0 when let go. */
/* driven readings, measured on Evan's FM-1 (2026-10-16, tools/fm1_pressure.py, F3 B3 E4 G4): a light touch
 * 920 .. 1016, the firmest press 480 .. 528. 0 % under a resting finger, 100 % within every key's firm press */
#define PR_LIGHT 900.0f
#define PR_FIRM 540.0f
/* Evan, on the FM-1: a light press read too high, and holding a firm press the number fell: the reading creeps
 * back up ~30 counts under a steady hard press (472 .. 504, measured). So a curve (x ^ PR_CURVE: slow at first,
 * steep near the top) and a hysteresis: harder is followed at once, easing only once the reading has risen
 * PR_HYST above the firmest point since (the creep and the jitter stay put; a real ease-off still moves it) */
#define PR_CURVE 1.6f
#define PR_HYST 32u
static float p_s[NKEYS];
static uint16_t p_ref[NKEYS];
static uint8_t p_val[NKEYS], p_raw[NKEYS];
static void plat_pressure_poll(void)
{
    static uint32_t t;
    const float lo = PR_FIRM - (float)SN_DBASE, hi = PR_LIGHT - (float)SN_DBASE;
    uint32_t i, now = fm1_ms;
    if (now - t < 5u)
        return;
    t = now;
    for (i = 0; i < NKEYS; i++) {
        float r, x;
        uint8_t p, raw;
        if (!sense_is_analog(i))
            continue;
        if (sn_key[i].st != SN_DOWN || !sn_key[i].dn) {
            if (p_raw[i]) {                                    /* let go: both to 0 (a held key's raw never is) */
                plat_midi_out(0x0Au | 0xA0u << 8 | (53u + i) << 16);
                plat_midi_out(0x0Bu | 0xBFu << 8 | i << 16);
            }
            p_s[i] = 0.0f;
            p_val[i] = 0;
            p_raw[i] = 0;
            p_ref[i] = 0;
            continue;
        }
        if (!p_ref[i] || sn_key[i].d < p_ref[i])
            p_ref[i] = sn_key[i].d;                            /* harder: at once */
        else if (sn_key[i].d > p_ref[i] + PR_HYST)
            p_ref[i] = (uint16_t)(sn_key[i].d - PR_HYST);      /* eased off, past the creep */
        r = (float)(p_ref[i] > SN_DBASE + 1u ? p_ref[i] - SN_DBASE : 1u);
        x = fm_clampf(fm_log2f(hi / r) / fm_log2f(hi / lo), 0.0f, 1.0f);
        x = fm_exp2f(PR_CURVE * fm_log2f(x > 1e-6f ? x : 1e-6f));   /* x ^ PR_CURVE */
        p_s[i] += (x - p_s[i]) * 0.17f;
        p = (uint8_t)(p_s[i] * 127.0f + 0.5f);
        raw = (uint8_t)(sn_key[i].d >> 3 > 127u ? 127u : sn_key[i].d >> 3);
        if (p != p_val[i]) {
            p_val[i] = p;
            plat_midi_out(0x0Au | 0xA0u << 8 | (53u + i) << 16 | (uint32_t)p << 24);
        }
        if (raw != p_raw[i]) {
            p_raw[i] = raw;
            plat_midi_out(0x0Bu | 0xBFu << 8 | i << 16 | (uint32_t)raw << 24);
        }
    }
}
int plat_pressure(uint32_t k) { return k < NKEYS && sense_is_analog(k) ? p_val[k] : -1; }

/* MIDI: Felucca's rings carry 4-byte USB-MIDI event packets, byte 0 = cable / CIN */
int plat_midi_in(uint32_t *pkt)
{
    if (mi_r == mi_w)
        return 0;
    *pkt = midi_in_q[mi_r % MQ];
    mi_r++;
    return 1;
}
void plat_midi_out(uint32_t pkt) { midi_out_event(pkt); }

int plat_store_load(uint32_t obj, void *dst, uint32_t max) { return flash_ok ? st_load(obj, dst, max) : -1; }
int plat_store_save(uint32_t obj, const void *src, uint32_t len) { return flash_ok ? st_save(obj, src, len) : -9; }

static volatile uint32_t audio_cpu_pct, audio_xruns;
uint32_t plat_cpu_pct(void) { return audio_cpu_pct; }
uint32_t plat_xruns(void) { return audio_xruns; }
