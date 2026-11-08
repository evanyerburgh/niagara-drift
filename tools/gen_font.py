#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Render the bitmap fonts to a C header (assets/fonts/):
  XS / S / B / M / L: Barlow Semi Condensed (TTF, SIL OFL 1.1); see SIZES. Anti-aliased, tabular figures
via the OpenType `tnum` feature.

Glyph format: per glyph an advance width, a bitmap width and an offset; the
bitmap starts FONT_PAD pixels left of the pen position (room for side
bearings); rows top to bottom, 2 pixels per byte (high nibble first),
alpha 0..15.
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

FONTS = Path(__file__).resolve().parents[1] / "assets" / "fonts"
# (name, file, px, scale, pixel, first, last): pixel fonts are rendered without
# anti-aliasing at their design size and enlarged by an integer factor
# X0X: five roles, XS (grid labels, units, hints), S (body), B (headers, names, the selected
# row), M (knob values) and L (the big readout). Each set gives them a face and a pixel size,
# chosen so the line height stays near the layout's 12 / 16 / 24 / 32. Niagara Drift's barlow set is a
# quarter bigger (Evan, 2026-10-14: too small on the FM-1): lines of 16 / 20 / 29. X0X_FONTSET picks one.
# A TTF entry "file.ttf@wght=600" sets a variable font's axis.
FONTSETS = {
    "barlow": [("XS", "BarlowSemiCondensed-SemiBold.ttf", 15, 1, False, 32, 126),
               ("S", "BarlowSemiCondensed-Medium.ttf", 19, 1, False, 32, 126),
               ("B", "BarlowSemiCondensed-Bold.ttf", 19, 1, False, 32, 126),
               # (M, 28, was the old screen's values: out with it, 0.3's PR H)
               ("L", "BarlowSemiCondensed-Bold.ttf", 31, 1, False, 32, 126),   # the start's name, calibration
               # 0.3's new look (the screen spec, sizes judged on the FM-1 by Evan, 2026-11-01): values selected and at
               # rest, their units; the page's name; the drones' names; knob names and their fallbacks; tags
               ("N1", "BarlowSemiCondensed-Bold.ttf", 26, 1, False, 32, 126),
               ("N2", "BarlowSemiCondensed-SemiBold.ttf", 21, 1, False, 32, 126),
               ("U1", "BarlowSemiCondensed-Bold.ttf", 16, 1, False, 32, 126),
               ("U2", "BarlowSemiCondensed-SemiBold.ttf", 13, 1, False, 32, 126),
               ("U3", "BarlowSemiCondensed-SemiBold.ttf", 12, 1, False, 72, 115),   # a long Tuning's name: "Harmonics"
               # alone needs it, so H .. s only (the full set was 5.7 KB of flash)
               ("H22", "BarlowSemiCondensed-Bold.ttf", 22, 1, False, 32, 126),
               ("T22", "BarlowSemiCondensed-Medium.ttf", 22, 1, False, 32, 126),
               ("T20", "BarlowSemiCondensed-Medium.ttf", 20, 1, False, 32, 126),
               ("T18", "BarlowSemiCondensed-Medium.ttf", 18, 1, False, 32, 126),
               ("Q17", "BarlowSemiCondensed-Medium.ttf", 17, 1, False, 32, 126)],
    "inter": [("XS", "Inter[opsz,wght].ttf@wght=600", 11, 1, False, 32, 126),
              ("S", "Inter[opsz,wght].ttf@wght=500", 14, 1, False, 32, 126),
              ("B", "Inter[opsz,wght].ttf@wght=700", 14, 1, False, 32, 126),
              ("M", "Inter[opsz,wght].ttf@wght=650", 21, 1, False, 32, 95),
              ("L", "Inter[opsz,wght].ttf@wght=700", 29, 1, False, 32, 95)],
    "rajdhani": [("XS", "Rajdhani-Bold.ttf", 14, 1, False, 32, 126),
                 ("S", "Rajdhani-SemiBold.ttf", 17, 1, False, 32, 126),
                 ("B", "Rajdhani-Bold.ttf", 17, 1, False, 32, 126),
                 ("M", "Rajdhani-Bold.ttf", 26, 1, False, 32, 95),
                 ("L", "Rajdhani-Bold.ttf", 35, 1, False, 32, 95)],
    "chakra": [("XS", "ChakraPetch-SemiBold.ttf", 11, 1, False, 32, 126),
               ("S", "ChakraPetch-Medium.ttf", 14, 1, False, 32, 126),
               ("B", "ChakraPetch-Bold.ttf", 14, 1, False, 32, 126),
               ("M", "ChakraPetch-SemiBold.ttf", 21, 1, False, 32, 95),
               ("L", "ChakraPetch-Bold.ttf", 29, 1, False, 32, 95)],
}
import os  # noqa: E402
FONTSET = os.environ.get("X0X_FONTSET", "barlow")    # the others need X0X_FONT_DIRS (not in assets/)
FONTDIRS = [Path(p) for p in os.environ.get("X0X_FONT_DIRS", "").split(":") if p]
SIZES = FONTSETS[FONTSET]
PAD = 2


def upscale(px_, w, h, scale):
    """integer nearest-neighbour enlargement of a w x h glyph"""
    if scale == 1:
        return px_
    big = []
    for y in range(h):
        row = []
        for x in range(w):
            row += [px_[y * w + x]] * scale
        big += row * scale
    return big


def font_path(name):
    for d in [FONTS, *FONTDIRS]:
        if (d / name).exists():
            return d / name
    raise SystemExit(f"gen_font: {name} not in {FONTS} or X0X_FONT_DIRS")


ARROW = {"S", "Q17"}                                  # the fonts that show the arrow (look.c: the banner, LK_F_MSG, and
#   the header's "-> P3", LK_F_TAG); the others leave its slot a space (the arrow in all 14 was 3.1 KB of flash)
BSLASH = {"N1", "N2", "U1", "U2", "U3", "XS", "T18"}     # the fonts that draw an interval (the values, Drones 1's
#   line): a real backslash for an equal Tuning's steps, "11\\19" (ledger 3a); the rest leave its slot a space


def render(ttf, px, scale, pixel, first, last, fname=""):
    name, _, var = ttf.partition("@")
    font = ImageFont.truetype(str(font_path(name)), px)
    if var:                                            # a variable font: "wght=600"
        axes = dict(kv.split("=") for kv in var.split(","))
        names = [a["name"] if isinstance(a["name"], str) else a["name"].decode() for a in font.get_variation_axes()]
        font.set_variation_by_axes([float(axes.get(n.lower()[:4], axes.get(n, 0)) or a["default"])
                                    for n, a in zip(names, font.get_variation_axes())])
    feats = None if pixel else ["tnum"]
    asc, desc = font.getmetrics()
    top = max(0, font.getbbox("A8|(", features=feats)[1] - 1)
    h = asc + desc - top
    glyphs = []
    for c in range(first, last + 1):
        ch = {0x60: "\u00b7", 0x5E: "\u00b1", 0x7E: "\u00d7"}.get(c, chr(c))
        arrow = c == 0x5C and fname in ARROW
        if c == 0x5C and not arrow and fname not in BSLASH:
            ch = " "
        # spare slots (gfx.c): the backtick's holds a middle dot ("·"), the caret's "±", the tilde's "×" (nothing on
        #   screen uses those three); the backslash's, an arrow (ARROW), a backslash (BSLASH) or a space
        adv = int(round(font.getlength(ch, features=feats)))
        bw = adv + 2 * PAD
        if arrow:                                      # the backslash's slot: a right arrow, drawn (Barlow has
            adv = int(round(font.getlength("-", features=feats) * 2.2))   # none): the banner's "LFO II -> I PITCH"
            bw = adv + 2 * PAD
        img = Image.new("L", (bw, asc + desc), 0)
        if arrow:
            d, sw = ImageDraw.Draw(img), max(1, round(px / 11))
            my = font.getbbox("-", features=feats)
            y = (my[1] + my[3]) / 2
            x0, x1, hh = PAD + 0.5, PAD + adv - 1.0, max(2.0, px * 0.2)
            d.line([(x0, y), (x1, y)], fill=255, width=sw)
            d.line([(x1 - hh, y - hh), (x1, y), (x1 - hh, y + hh)], fill=255, width=sw, joint="curve")
        else:
            ImageDraw.Draw(img).text((PAD, 0), ch, font=font, fill=255, features=feats)
        img = img.crop((0, top, bw, top + h))
        px_ = [(15 if v >= 128 else 0) if pixel else min(15, (v + 8) // 17) for v in img.tobytes()]
        glyphs.append((adv * scale, bw * scale, upscale(px_, bw, h, scale)))
    return h * scale, glyphs, (asc - top) * scale


def main(out):
    lines = [f"/* generated by tools/gen_font.py: font set {FONTSET} */",
             "#pragma once", "#include <stdint.h>", f"#define FONT_PAD {PAD}  /* x scale for L, see below */", ""]
    for name, ttf, px, scale, pixel, first, last in SIZES:
        h, glyphs, base = render(ttf, px, scale, pixel, first, last, name)
        data, offs = [], []
        for adv, bw, g in glyphs:
            offs.append(len(data))
            for y in range(h):
                row = g[y * bw:(y + 1) * bw] + [0]
                for x in range(0, bw, 2):
                    data.append((row[x] << 4) | row[x + 1])
        assert len(data) < 65536
        lines.append(f"static const uint8_t FONT_{name}_DATA[{len(data)}] = {{")
        for i in range(0, len(data), 24):
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 24]) + ",")
        lines.append("};")
        lines.append(f"static const uint16_t FONT_{name}_OFF[{len(offs)}] = {{" + ", ".join(map(str, offs)) + "};")
        lines.append(f"static const uint8_t FONT_{name}_ADV[{len(glyphs)}] = {{" +
                     ", ".join(str(a) for a, _, _ in glyphs) + "};")
        lines.append(f"static const uint8_t FONT_{name}_BW[{len(glyphs)}] = {{" +
                     ", ".join(str(b) for _, b, _ in glyphs) + "};")
        lines.append(f"static const felucca_font_t FONT_{name} = {{ {h}, {PAD * scale}, {first}, {last}, "
                     f"FONT_{name}_ADV, FONT_{name}_BW, FONT_{name}_OFF, FONT_{name}_DATA }};")
        lines.append(f"#define FONT_{name}_BASE {base}  /* the baseline, from the cell's top */")
        lines.append("")
        print(f"font {name}: {ttf} {px}px h {h}, digit adv {glyphs[ord('0') - first][0]}, {len(data)} B")
    print(f"font set {FONTSET}: {sum(len(l) for l in lines if l.startswith('    0x')) // 6} B (approx)")
    Path(out).write_text("\n".join(lines))


if __name__ == "__main__":
    main(sys.argv[1])
