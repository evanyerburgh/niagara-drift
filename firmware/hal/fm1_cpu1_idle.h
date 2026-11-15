/* SPDX-License-Identifier: GPL-3.0-only */
/* The second core's idle word (fm1_cpu1.h, fm1_cpu1_stop.h): 1 while CPU1 sits in its RAM loop with no job, so the
 * flash may be turned off. felucca-bsp's fm1_flash_access.h holds it beside a gate on every flash entry; here the
 * flash driver (fm1_flash.h) runs only from the main loop with interrupts off, and CPU1 gets jobs only inside the
 * audio interrupt, which waits for each before it returns: CPU1 is always idle when the flash is touched. */
#pragma once
#include <stdint.h>

static volatile uint32_t fm1_flash_cpu1_idle;
#if FM1_CPU1                                    /* (fm1_cpu1.h checks what this was built with) */
#define FM1_FLASH_GATE_CPU1 1
#else
#define FM1_FLASH_GATE_CPU1 0
#endif
