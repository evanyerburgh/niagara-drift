/* SPDX-License-Identifier: GPL-3.0-only */
/* No press lost under load (0.3.1; Thomas Loop's report, 2026-11-16: with the audio near 90 % a page button took 3-4
 * tries). The scan tick debounces every key 10,000 times a second whatever the load, but the UI looked at what was
 * held only once a screen frame, and with the audio taking most of the chip a frame can last longer than a quick
 * tap: down and up again between two looks, never seen. fm1_taps() gives the UI what is held, corrected by the
 * edges the scan saw since the last look: a key that went down since then reads held for this look even if it is up
 * again (the next look reads it up: a tap), and one seen held that was let go reads up for this look even if it is
 * down again (a second tap). Pure, so tests/host/taps_test.c checks it on the host.
 *   now: held at this moment; pressed / released: edges since the last look; *seen: what the last look returned */
#pragma once
#include <stdint.h>

static inline uint32_t fm1_taps(uint32_t now, uint32_t pressed, uint32_t released, uint32_t *seen)
{
    uint32_t out = (now | (pressed & ~*seen)) & ~(released & *seen);
    *seen = out;
    return out;
}
