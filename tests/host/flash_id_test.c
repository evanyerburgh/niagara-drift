/* SPDX-License-Identifier: GPL-3.0-only */
/* The flash parts Niagara Drift saves to and installs on (fl_id_ok, firmware/hal/fm1_flash.h): any standard SPI NOR of
 * 1 to 16 MiB. The first FM-1s carry a Puya P25Q80 (JEDEC 85 60 14); FM-1s sold since autumn 2026 a UC25HQ80
 * (B3 60 14), the same plain commands (fm1-base's device/fm1_flash_ok.h, Laurens, #35; thefirmwarehub.com lists the
 * firmwares that take it). Nothing answering (00 / FF) or a part under 1 MiB: RAM only, nothing written. */
#include <stdio.h>
#include "fm1_flash_id.h"
static int fails;
static void check(const char *what, int ok)
{
    printf("%-60s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
int main(void)
{
    check("the Puya P25Q80 (856014), the first FM-1s", fl_id_ok(0x856014u));
    check("the UC25HQ80 (B36014), the newer FM-1s", fl_id_ok(0xB36014u));
    check("another standard 2-16 MiB part (EF4015, C84018)", fl_id_ok(0xEF4015u) && fl_id_ok(0xC84018u));
    check("nothing answering (000000, FFFFFF): refused", !fl_id_ok(0u) && !fl_id_ok(0xFFFFFFu));
    check("under 1 MiB or over 16 MiB (856013, EF4019): refused", !fl_id_ok(0x856013u) && !fl_id_ok(0xEF4019u));
    printf("flash_id: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
