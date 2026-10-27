# Niagara Drift

Drone-synth firmware for the M-VAVE FM-1 (GPL-3.0-only). The README is the player's manual; BUILDING.md
says how to build. This file is for anyone working on the code with Claude, and for the PR review in
GitHub Actions. Personal notes go in `CLAUDE.local.md` (ignored).

## Commands

```
sh tests/run_tests.sh        # every host test, the scenarios, and the emulator if emcc is there
sh host/build_host.sh        # the host simulator: build/host/omni_host SCRIPT.omni OUTDIR (WAVs, PNGs)
./build.sh                   # the firmware (Docker, the JieLi toolchain and SDK): build/omni.fwsc
sh web/emu/build.sh          # the browser emulator into build/emu
tools/budget.py              # after ./build.sh: the PR's budget line against main
```

On macOS, export `SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"` first. On Linux the tests need
`CC="cc -D_DEFAULT_SOURCE -Wl,--no-as-needed -lm"`, and drone-cost can't count instructions there (CI
counts them with valgrind: `tools/ci_cost.py`).

## The FM-1's budget

The chip is small, and the sound comes first. A change can spend these only when it's worth hearing:

- **RAM:** .data + .bss under 96 KB, the pool with at least 8 KB spare; the build fails past either.
  Both are close (80 KB, 11 KB spare in October 2026): a new buffer or table in RAM needs a reason.
- **CPU:** the engine's worst case (`tests/host/drone_cost.c`) under its limit; on the device the
  longest audio block (`tools/fm1_cpu.py`) about 69 % in the worst realistic case at 0.2: keep the
  headroom. A block past 100 % is late, and a late block is a click.
- **Flash:** the image fits its slot; tables that aren't read every sample stay `const` in flash.

Every firmware PR body carries `tools/budget.py`'s line, so a change's cost is seen beside what it does.

## What the build enforces (don't work around it)

- No double: the FPU is single precision, and a soft-double routine in the image fails the build.
- `.ram_text` (code that runs with the flash off) makes no calls.
- Registers are touched only in `firmware/hal/`.
- `-ffp-contract=off` everywhere, so the host, the emulator and the device give the same samples.

## Conventions

- New files start with `/* SPDX-License-Identifier: GPL-3.0-only */`; keep the headers of reused files;
  anything borrowed gets a row in LICENSING.md, its licence checked first. No JieLi SDK files in git.
- New parameters go at the end of the enums and bump `PROJ_FORMAT`; renumber the `.omni` scenarios when
  parameters or pages move (the top of `tests/scenarios/drone.omni` lists them).
- `drone.c` is one compilation unit (it includes `drone_fx.c`): declare statics before first use.
- A sound change comes with a host render (WAV) and numbers measured on it; a screen change with a
  screenshot. Measure on and off as two renders of the same passage (the host is deterministic).
- Audio code: `.claude/rules/dsp.md`.

## Gotchas

- `tools/make_screens.py` (after `sh host/build_host.sh`) remakes the README's pictures from
  `tests/scenarios/pages.omni`, which starts with `notips` so no one-time tip shows; afterwards only the screens a
  change touches should differ (`git status docs/img`).
- A host build next to stale copies of `drone.h` picks those up: build from the tree.
- `.omni` files are the host simulator's scenarios (the name is FoMni's: "OMNI"); `omni_host`, `build/omni.fwsc` too.
