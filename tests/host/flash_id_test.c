/* SPDX-License-Identifier: GPL-3.0-only */
/* The flash parts Niagara Drift saves to and installs on (fl_id_ok, firmware/hal/fm1_flash_id.h): the first FM-1s'
 * Puya P25Q80 (JEDEC 85 60 14) and the newer FM-1s' UC25HQ80 (B3 60 14), as Felucca, fm1-base (#35, Laurens) and
 * fm1-x0x take them; anything else, RAM only and nothing written (the cases of fm1-x0x's flash_ok_test.c). */
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
    check("nothing answering (000000, FFFFFF): refused", !fl_id_ok(0u) && !fl_id_ok(0xFFFFFFu));
    check("another part (Winbond EF4016, GigaDevice C84019): refused", !fl_id_ok(0xEF4016u) && !fl_id_ok(0xC84019u));
    check("a near miss (856015, B36015): refused", !fl_id_ok(0x856015u) && !fl_id_ok(0xB36015u));
    printf("flash_id: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
