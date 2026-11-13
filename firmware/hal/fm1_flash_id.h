/* SPDX-License-Identifier: GPL-3.0-only */
/* Which flash parts Niagara Drift saves to and installs on: the two 1 MiB parts FM-1s carry, the Puya P25Q80 (0x856014,
 * the first FM-1s) and the UC25HQ80 (0xB36014, FM-1s sold since autumn 2026; issue #66). The same check as Felucca's
 * (firmware/src/project.c) and fm1-base's (device/fm1_flash_ok.h): the hubs and package checkers find a firmware's
 * chips by these numbers in its code (0.2.1's rule, any standard part of 1-16 MiB, had neither, and
 * thefirmwarehub.com said "it does not check the chip"). Any other answer: RAM only, nothing written. Pure, so
 * tests/host/flash_id_test.c checks it on the host. */
#pragma once
#include <stdint.h>

static inline int fl_id_ok(uint32_t id)
{
    return id == 0x856014u || id == 0xB36014u;   /* known 1 MiB parts, else RAM-only */
}
