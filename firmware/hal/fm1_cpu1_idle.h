/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal: felucca-bsp's fm1_flash_access.h (its CPU1 idle word), reduced to that word for
 *   Niagara Drift; with Kerem Kilic's Melodee (Ellic Studio), under MPL-2.0 by agreement
 *   (https://github.com/hugelton/felucca-bsp/issues/5) */
/* The second core's idle word (fm1_cpu1.h, fm1_cpu1_stop.h): 1 while CPU1 sits in its RAM loop with no job, so the
 * flash may be turned off. felucca-bsp's fm1_flash_access.h holds it beside a gate on every flash entry; here the
 * flash driver (fm1_flash.h) runs only from the main loop with interrupts off, and CPU1 gets jobs only inside the
 * audio interrupt, which waits for each before it returns: CPU1 is always idle when the flash is touched. Niagara
 * Drift's flash calls (main.c, fl_cpu1_busy) check this word and refuse a busy CPU1. Static: one unit reads it
 * (main.c, which includes the app); the loader has its own copy and never starts CPU1. */
#pragma once
#include <stdint.h>

static volatile uint32_t fm1_flash_cpu1_idle;
#if FM1_CPU1                                    /* (fm1_cpu1.h checks what this was built with) */
#define FM1_FLASH_GATE_CPU1 1
#else
#define FM1_FLASH_GATE_CPU1 0
#endif
