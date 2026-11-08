/* SPDX-License-Identifier: GPL-3.0-only */
/* An interval in a Tuning's own terms (ledger 3a, dr_tun_name): steps over the division on the equal ones, the ratio
 * on the others, nothing on 12-tone, at unison, off the Tuning's notes or too long to fit. Built with the engine */
#include "drone.c"
#include <stdio.h>
#include <string.h>

static int fails;

static void check(int t, int p1, int a, int b, const char *want)
{
    char got[16] = "";
    const int ok = dr_tun_name(t, p1, a, b, got);
    if ((want == 0 && ok) || (want && (!ok || strcmp(got, want)))) {
        printf("FAIL %s %d..%d: got %s, want %s\n", DR_TUNING_NAME[t], a, b, ok ? got : "(none)", want ? want : "(none)");
        fails++;
    }
}

int main(void)
{
    const int D3 = 5000, A3 = 5700;
    check(DR_TUN_12, D3, D3, A3, 0);                         /* 12-tone: the familiar names */
    check(DR_TUN_19, 0, A3 - 695, A3, "11\\19");              /* 19-tone's fifth, down from A */
    check(DR_TUN_19, 0, A3, A3 - 695, "11\\19");              /* (the size alone, either way) */
    check(DR_TUN_53, 0, A3 - 702, A3, "31\\53");
    check(DR_TUN_24, 0, A3 - 350, A3, "7\\24");               /* a neutral third */
    check(DR_TUN_19, 0, D3, A3, "11\\19");                   /* D3 5c off 19-tone's D: by its nearest note */
    check(DR_TUN_19, 0, A3 - 682, A3, 0);                     /* the gap 13c from 11\\19: the familiar name */
    check(DR_TUN_19, 0, A3 - 704, A3, "11\\19");             /* 9c: still its name */
    check(DR_TUN_JUST, D3, D3, D3 + 702, "3/2");              /* Just on drone I */
    check(DR_TUN_JUST, D3, D3, D3 + 1902, "3/1");             /* an octave and a fifth */
    check(DR_TUN_JUST, D3, D3, D3, 0);                        /* unison */
    check(DR_TUN_HARM, 0, 4500 + 386, 4500 + 551, "11/10");   /* harmonics 5 and 11 of A (A2 4500) */
    check(DR_TUN_HARRISON, 0, 4500 + 498, A3, "3/2");         /* D (4/3 over A) to A */
    check(DR_TUN_SHRUTI, D3, D3, D3 + 90, "256/243");
    check(DR_TUN_PARTCH, 0, 4500, 4500 + 1178, "160/81");
    check(DR_TUN_LMY, 0, 4500 + 525, 4500 + 969, 0);          /* 256/189 to 7/4: 1323/1024 is too long */
    check(DR_TUN_LMY, 0, 4500 + 267, 4500 + 525, "512/441");
    printf("dr_tun_name: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
