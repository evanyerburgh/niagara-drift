/* SPDX-License-Identifier: GPL-3.0-only */
/* Which flash parts Niagara Drift saves to and installs on: a JEDEC id that is a standard SPI NOR of 1 to 16 MiB. The
 * first FM-1s carry 0x856014 (Puya P25Q80, 1 MiB); FM-1s sold since autumn 2026 a UC25HQ80, 0xB36014 (1 MiB; issue
 * #66, fm1-base's device/fm1_flash_ok.h). Every such part takes the commands fm1_flash.h uses (06 05 20 02 0B), and
 * the update loader writes and checks the whole app with them on whatever part is there. Nothing answering (00 / FF)
 * or another size: RAM only, nothing written. Pure, so tests/host/flash_id_test.c checks it on the host. */
#pragma once
#include <stdint.h>

static inline int fl_id_ok(uint32_t id)
{
    uint32_t maker = id >> 16, size = id & 0xFFu;
    return maker != 0u && maker != 0xFFu && size >= 0x14u && size <= 0x18u;
}
