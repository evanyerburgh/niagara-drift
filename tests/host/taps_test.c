/* SPDX-License-Identifier: GPL-3.0-only */
/* No press lost between two looks at the panel (fm1_taps, firmware/hal/fm1_taps.h): under a heavy sound a screen
 * frame can outlast a quick tap, so the UI's look is corrected by the edges the scan tick saw since the last one. */
#include <stdio.h>
#include "fm1_taps.h"
static int fails;
static void check(const char *what, int ok)
{
    printf("%-72s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
int main(void)
{
    uint32_t seen = 0, a, b;
    check("nothing touched: nothing held", fm1_taps(0, 0, 0, &seen) == 0);
    check("a key held across looks: held, held", fm1_taps(1, 1, 0, &seen) == 1 && fm1_taps(1, 0, 0, &seen) == 1);
    check("let go: up", fm1_taps(0, 0, 1, &seen) == 0);
    a = fm1_taps(0, 1, 1, &seen);                   /* down and up again between two looks */
    b = fm1_taps(0, 0, 0, &seen);
    check("a tap between two looks: held for one look, then up", a == 1 && b == 0);
    seen = 0;
    fm1_taps(1, 1, 0, &seen);
    a = fm1_taps(1, 1, 1, &seen);                   /* held, let go and pressed again between two looks */
    b = fm1_taps(1, 0, 0, &seen);
    check("held, a second tap between two looks: up for one look, then held", a == 0 && b == 1);
    seen = 0;
    a = fm1_taps(2, 1 | 2, 1, &seen);               /* key 0 tapped while key 1 went down and stayed */
    b = fm1_taps(2, 0, 0, &seen);
    check("two keys at once, each its own", a == 3 && b == 2);
    seen = 0;
    a = fm1_taps(1, 1, 1, &seen);                   /* tapped twice, the second still held: one press seen */
    b = fm1_taps(1, 0, 0, &seen);
    check("up, two presses between looks, the second held: held, held", a == 1 && b == 1);
    printf("taps: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
