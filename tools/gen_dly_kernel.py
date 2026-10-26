#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Write the delay's read kernel (firmware/src/dsp/dly_kernel.h): 8 taps of a windowed sinc (Kaiser, beta 6, cut
at 0.46 of the half rate), DLY_PH + 1 phases, each summing to 1. Fixed numbers in the firmware, not RAM worked out
at start-up (2026-10-26). Run it again only if the kernel changes: python3 tools/gen_dly_kernel.py"""
import math
import os

TAPS, PH, BETA, CUT = 8, 128, 6.0, 0.92


def i0(z):  # the modified Bessel function of the first kind, order 0 (the Kaiser window's)
    s = t = 1.0
    for m in range(1, 30):
        t *= (z / 2.0 / m) ** 2
        s += t
    return s


rows = []
for ph in range(PH + 1):
    f = ph / PH
    k = []
    for t in range(TAPS):
        x = t - TAPS // 2 + 1 - f
        w = x / (0.5 * TAPS)
        sx = CUT if abs(x) < 1e-9 else math.sin(math.pi * CUT * x) / (math.pi * x)
        k.append(sx * (i0(BETA * math.sqrt(1.0 - w * w)) / i0(BETA) if abs(w) <= 1.0 else 0.0))
    s = sum(k)
    rows.append([v / s for v in k])

out = os.path.join(os.path.dirname(__file__), "..", "firmware", "src", "dsp", "dly_kernel.h")
with open(out, "w") as fh:
    fh.write("/* SPDX-License-Identifier: GPL-3.0-only */\n")
    fh.write("/* The delay's read kernel, made by tools/gen_dly_kernel.py (don't edit by hand): %d taps of a windowed sinc\n"
             " * (Kaiser, beta %g, cut at %g of the half rate's Nyquist), %d phases and the next sample's first */\n" %
             (TAPS, BETA, CUT, PH))
    fh.write("#define DLY_TAPS %d\n#define DLY_PH %d\n" % (TAPS, PH))
    fh.write("static const float dly_k[DLY_PH + 1][DLY_TAPS] = {\n")
    for r in rows:
        fh.write("    {" + ", ".join("%.9gf" % v for v in r) + "},\n")
    fh.write("};\n")
print("wrote", os.path.normpath(out))
