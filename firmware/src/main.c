/* SPDX-License-Identifier: GPL-3.0-only */
/* OMNI firmware for the M-VAVE FM-1: one compilation unit for the platform and the app
 * (Felucca's layout, as X0X: the HAL is header-only, so everything that touches it is here).
 * The instrument (dsp/drone.c) is a separate unit compiled at -O2 by tools/build.py; this one is
 * -Os. Order matters. */
#include <stdint.h>
#ifndef OM_CPU2
#define OM_CPU2 1                 /* the second core (felucca-bsp's fm1_cpu1.h): started at power-on; its tests on request */
#endif
#ifndef OM_RAILS
#define OM_RAILS 1                /* M-VAVE's own supplies at start (fm1_cstart) */
#endif
#define FM1_CPU1 OM_CPU2          /* (before the first HAL header: fm1_irq.h and fm1_sys.h read it) */
#define FM1_CPU1_FALLBACK 1       /* a misread, a fault or a late job: one core until power-off, no restart */
#define FM1_CPU1_TIMEOUT_US 5000u /* a job not back 5 ms after its hand-over: retired inside the block (5.8 ms) */
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#if OM_CPU2
#include "fm1_cpu1.h"
#endif
/* the key pressure (MONK-1's, from Velo): the scan tick hands each analog row sample to app/plat_fm1.c */
static void plat_analog_sample(uint32_t col, uint32_t rowbit, uint32_t v, uint32_t driven);
static int plat_analog_drive(uint32_t col, uint32_t rowbit);
static int plat_analog_led_ok(uint32_t col, uint32_t rowbit);
#define FM1_INPUT_ANALOG(c, r, v, d) plat_analog_sample(c, r, v, d)
#define FM1_INPUT_DRIVE(c, r) plat_analog_drive(c, r)
#define FM1_INPUT_LED_OK(c, r) plat_analog_led_ok(c, r)
#include "fm1_input.h"
#include "fm1_timer.h"
#include "fm1_audio.h"
#include "fm1_adc.h"
#include "fm1_lcd_hw.h"
#include "fm1_flash.h"
#include "fm1_xip.h"
#include "fm1_perf.h"

#include "libc.c"
#include "lcd.c"
#include "gfx.c"

static volatile uint32_t fm1_ms;  /* milliseconds since boot (TIMER4-based, TIMER5 ISR) */
static volatile uint32_t fm1_us, fm1_us_t;   /* microseconds since boot, kept the same way, and the tick they were at
                                              * (the timer wraps every 179 s: its ticks / 24 can't be used across it) */
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static inline int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

#include "app/plat.h"

#define FELUCCA_OTA 1             /* M-UPGRADE update entry: keeps the web installer working */
#ifndef OM_CDC
#define OM_CDC 0                  /* USB serial console off: one plain MIDI interface */
#endif
#define FELUCCA_CDC OM_CDC
#ifndef OM_UAC
#define OM_UAC 1                  /* USB audio input: the master output, recordable over the cable */
#endif
#define FELUCCA_UAC OM_UAC
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_800"     /* package identity (tools/build.py) */
#endif
#define MQ_NOW_US() (fm1_us + (fm1_ticks() - fm1_us_t) / FM1_TICKS_PER_US)   /* a MIDI packet's time, us (the clock's) */
#include "usb.c"
#ifndef OM_TRS
#define OM_TRS 1                  /* MIDI IN on the TRS jack */
#endif
#if OM_TRS
#include "midi_uart.c"
#endif

static uint8_t flash_ok;          /* JEDEC id matched at boot */
#if OM_CPU2
/* The second core's jobs run from the flash, so the flash is touched only while CPU1 is idle in its RAM loop (or
 * held). Every job is waited for before the audio interrupt returns (main_fm1.c's backstop), so the main loop always
 * finds it idle; this checks it, and refuses (counting it, in fm1_cpu.py --cores) rather than cut the flash under a
 * running job. Called with interrupts off, or just before a driver call that turns them off. */
static uint32_t fl_refused;
static int fl_cpu1_busy(void)
{
    if (!fm1_cpu1_flash_ok()) {
        fl_refused++;
        return 1;
    }
    return 0;
}
#else
#define fl_cpu1_busy() 0
#endif
static void audio_silence(void);
static int st_read(uint32_t off, void *dst, uint32_t n)   /* 256-byte IRQ-off windows: audio keeps up */
{
    uint8_t *d = dst;
    while (n) {
        uint32_t k = n > 256u ? 256u : n, f = irq_save();
        int rc = fl_cpu1_busy() ? -21 : FL_FAR(fl_read_ram)(off, d, k);
        irq_restore(f);
        if (rc)
            return rc;
        off += k;
        d += k;
        n -= k;
    }
    return 0;
}
static int st_erase(uint32_t off)
{
    uint32_t took;
    if (!FL_STORE_OK(off, 0x1000u))
        return -8;
    if (fl_cpu1_busy())
        return -21;
    audio_silence();
    return fl_erase4k(off, &took);
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    if (!FL_STORE_OK(off, n))
        return -8;
    if (fl_cpu1_busy())
        return -21;
    return fl_write(off, src, n);
}
#include "storage.c"

struct bootguard_s { uint32_t magic, failed, pending; };
extern struct bootguard_s bootguard;
#include "ota.c"
static uint32_t ota_now_ms(void) { return fm1_ms; }
static void ota_idle(void) { fm1_wdt_feed(); }
static int ota_in_area(uint32_t off, uint32_t n) { return FL_IN(off, n, OTA_AREA, OTA_AREA + OTA_AREA_LEN); }
static int ota_erase(uint32_t off)
{
    uint32_t took;
    if (!ota_in_area(off, 0x1000u) || (off & 0xFFFu))
        return -8;
    if (fl_cpu1_busy())
        return -21;
    audio_silence();
    return fl_erase4k(off, &took);
}
static int ota_prog(uint32_t off, const void *p, uint32_t n)
{
    if (!ota_in_area(off, n))
        return -8;
    if (fl_cpu1_busy())
        return -21;
    return fl_write(off, p, n);
}
static int ota_fread(uint32_t off, void *p, uint32_t n) { return st_read(off, p, n); }
/* THE SAFE RESTORE's flash core (fm1-base's, MPL-2.0: restore_core.c): the objects a backup holds, in the order of
 * its journal record (the panel's calibration never: it belongs to the unit); the record's sector, the update's */
#define APP_BACKUP_OBJS {OBJ_PROJ, OBJ_BANKB, OBJ_BANKC, OBJ_BANKP, OBJ_BANKD, OBJ_BANKE, OBJ_BANKF}
#include "restore_core.c"
static int rs_rec_erase(void) { return ota_erase(RS_REC); }
static int rs_rec_prog(uint32_t off, const void *src, uint32_t n) { return ota_prog(RS_REC + off, src, n); }
static int rs_rec_read(uint32_t off, void *dst, uint32_t n) { return ota_fread(RS_REC + off, dst, n); }
static void ota_show(uint32_t step, int32_t code)
{
    static const char *const STEP[] = {"", "PACKAGE", "CHECK HEAD", "LOADER", "CONFIRM", "RESTART"};
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 92, 240, &FONT_S, "UPDATE", C_WHITE, 1);
    if (step < 9u) {
        draw_text_box(0, 124, 240, &FONT_S, STEP[step < 6u ? step : 0], C_HI, 1);
        return;
    }
    draw_text_box(0, 124, 240, &FONT_S, code == 1 ? "DRY RUN OK" : "FAILED", C_HI, 1);
    fm1_delay_ms(1500);
}
static void ota_commit(const uint8_t *parm)
{
    bootguard.pending = 0;                              /* intentional reset */
    bootguard.failed = 0;                               /* a new install starts with a clean slate */
    usb_detach();
    fm1_delay_ms(30);
    fm1_enter_update(parm);
}

#include "app/app.h"
#define ANN_NAME "Niagara Drift " OM_VERSION
#include "ann_words.h"            /* the link to Niagara Drift's words file (tools/gen_words.py, from firmware/words.tsv) */
#include "announce.c"             /* the screen as text for a screen reader (Charles Vestal's fm1-reader; fm1-base, MPL-2.0) */
static int ann_wire_send(const uint8_t *p, uint32_t n)   /* never waits: no room (nobody reading), not sent */
{
    if (!usb.config || usb.suspended || SXQ - (so_w - so_r) < (n + 2u) / 3u)
        return -1;
    return ota_wire_send(p, n);
}
#include "app/sense.c"
#include "app/panel.c"
#include "app/plat_fm1.c"
#include "app/project.c"
#include "app/ui.c"
#include "app/main_fm1.c"
