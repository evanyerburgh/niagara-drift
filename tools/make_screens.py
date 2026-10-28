#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The README's pictures: runs tests/scenarios/pages.omni in the host simulator (each page with values that
# show something) and puts each button's page, or its two pages side by side, in docs/img/page-<name>.png;
# and draws the FM-1 itself (the emulator's geometry, the Drones page on its screen) with what every knob,
# button and key does in Niagara Drift written on it: docs/img/panel.png.
#   sh host/build_host.sh && python3 tools/make_screens.py
import os, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUTTONS = [("play", ["play", "play2"]), ("tone", ["tone", "tone2"]), ("osc", ["osc", "source"]), ("seq", ["partner", "partner2"]),
           ("lfo", ["lfo", "lfo2"]), ("swell", ["pulse"]), ("effects", ["fx", "fx2", "types"]), ("setup", ["setup", "setup2"]), ("keys", ["keymap"])]
SCALE, GAP = 2, 16
BG = (26, 4, 24)


def main():
    host = os.path.join(ROOT, "build", "host", "omni_host")
    if not os.path.exists(host):
        sys.exit("no build/host/omni_host: run host/build_host.sh")
    out = tempfile.mkdtemp()
    subprocess.run([host, os.path.join(ROOT, "tests", "scenarios", "pages.omni"), out], check=True,
                   stdout=subprocess.DEVNULL)
    w = 240 * SCALE
    for name, shots in BUTTONS:
        img = Image.new("RGB", (len(shots) * w + (len(shots) - 1) * GAP, w), BG)
        for i, shot in enumerate(shots):
            scr = Image.open(os.path.join(out, shot + ".png")).convert("RGB").resize((w, w), Image.NEAREST)
            img.paste(scr, (i * (w + GAP), 0))
        dst = os.path.join(ROOT, "docs", "img", "page-%s.png" % name)
        img.save(dst, optimize=True)
        print("screens:", os.path.relpath(dst, ROOT), img.size)
    pm = panel(os.path.join(out, "play.png"))
    dst = os.path.join(ROOT, "docs", "img", "panel.png")
    pm.save(dst, optimize=True)
    print("panel:", os.path.relpath(dst, ROOT), pm.size)


# the panel, in the emulator's units (web/emu/index.html: a 1000 x 640 drawing), drawn as the emulator draws it: a black
# FM-1 with light keys; the labels in three roles: the printed name small and grey (legend), what it does in Niagara
# Drift under it in colour (job), and one size in ink on the keys
BODY, TRAY, KNOB, BTN, LABEL = (29, 29, 32), (14, 14, 16), (10, 10, 11), (42, 42, 46), (232, 232, 234)
KEY_W, KEY_B = (217, 218, 220), (201, 202, 205)
AMBER, TEAL, LILAC, PALE, INK = (255, 176, 74), (69, 227, 211), (220, 188, 255), (255, 214, 240), (20, 0, 15)   # the site's
LEGEND, MUTED_KEY = (166, 166, 173), (85, 85, 91)
KNOBS = [(105, 100, 24, "MASTER", "Volume", PALE), (215, 100, 24, "SELECT", "LFO rate", LILAC),
         (105, 210, 24, "PRESETS", "Gesture time", PALE), (215, 210, 24, "ALGORITHM", "Movement", LILAC)] + \
        [(610 + i * 104, 100, 22, "KNOB%d" % (i + 1), None, None) for i in range(4)]
ROW1 = [("FX", "Delay"), ("SEL", "Fine"), ("ENV", "Tone"), ("LFO", "Depths"), ("EDIT", "Osc"), ("GLO", "Setup")]
ROW2 = [("HOME", "Drones"), ("SAVE", "Store"), ("ARP", "Swell"), ("SEQ", "Partner"), ("PLAY", "On/off"), ("REC", "Loop")]
BLACK_AFTER = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14]
BLACK = [(["Partner"], AMBER), (["Sub"], AMBER), (["Filter"], AMBER), (["Pitch", "lock"], None), (["LFO", "reset"], None),
         (["Partner"], TEAL), (["Sub"], TEAL), (["Filter"], TEAL), (["Pitch", "link"], None), (["Hard", "sync"], None),
         (["X-mod"], None)]


def panel(play_png):
    S = 2.6                                                   # pixels a unit
    def P(v): return int(round(v * S))
    img = Image.new("RGBA", (P(1000), P(640)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    def f(size, w="Bold"):
        return ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-%s.ttf" % w), P(size))
    def rect(x, y, w, h, r, fill, outline=None, width=0):
        d.rounded_rectangle([P(x), P(y), P(x + w), P(y + h)], radius=P(r), fill=fill, outline=outline,
                            width=P(width) if width else 0)
    def text(x, y, s, size, fill, w="Bold", track=0):        # centred on x, its baseline at y (as the SVG's)
        fo = f(size, w)
        if not track:
            d.text((P(x), P(y)), s, font=fo, fill=fill, anchor="ms")
            return d.textlength(s, font=fo) / S
        sp = size * track                                     # letter-spaced, a letter at a time
        ws = [d.textlength(c, font=fo) / S for c in s]
        x0 = x - (sum(ws) + sp * (len(s) - 1)) / 2
        for c, cw in zip(s, ws):
            d.text((P(x0), P(y)), c, font=fo, fill=fill, anchor="ls")
            x0 += cw + sp
    def legend(x, y, s): text(x, y, s, 11, LEGEND, "SemiBold", .08)
    def job(x, y, s, col): text(x, y, s, 14, col, "SemiBold")
    def bracket(x1, x2, y, s, col, size, tick):               # a caption between two rules, ticks at the ends
        w = d.textlength(s, font=f(size, "SemiBold")) / S / 2
        m = (x1 + x2) / 2
        text(m, y + size * .35, s, size, col, "SemiBold")
        for a, b in ((x1, m - w - 8), (m + w + 8, x2)):
            d.line([P(a), P(y), P(b), P(y)], fill=col, width=P(1.2))
        for x in (x1, x2):
            d.line([P(x), P(y), P(x), P(y + tick)], fill=col, width=P(1.2))
    rect(4, 4, 992, 632, 44, BODY)
    for x, y, r, name, what, col in KNOBS:                   # knobs: the legend above, the job below
        d.ellipse([P(x - r - 5), P(y - r - 5), P(x + r + 5), P(y + r + 5)], fill=TRAY)
        d.ellipse([P(x - r), P(y - r), P(x + r), P(y + r)], fill=KNOB, outline=(58, 58, 64), width=P(2))
        d.line([P(x), P(y - r + 5), P(x), P(y - r + 15)], fill=LABEL, width=P(3))
        legend(x, y - r - 10, name)
        if what:
            job(x, y + r + 20, what, col)
    bracket(580, 744, 143, "Drone I", AMBER, 14, -4)
    bracket(788, 952, 143, "Drone II", TEAL, 14, -4)
    rect(58, 262, 210, 58, 14, TRAY)                           # OCT- / OCT+: the LFO waves
    for x, n, w in ((72, "OCT−", "Wave I"), (170, "OCT+", "Wave II")):
        rect(x, 272, 82, 38, 9, BTN, (70, 70, 76), 1.5)
        legend(x + 41, 287, n)
        job(x + 41, 303, w, LILAC)
    rect(300, 44, 236, 236, 26, (10, 10, 11))                  # the screen
    scr = Image.open(play_png).convert("RGB").resize((P(200), P(200)), Image.LANCZOS)
    img.paste(scr, (P(318), P(62)))
    rect(568, 160, 400, 140, 18, TRAY)                         # the buttons: the legend, the page it opens
    for y, names in ((174, ROW1), (236, ROW2)):
        for i, (n, what) in enumerate(names):
            x = 582 + i * 63
            rect(x, y, 54, 48, 9, BTN, (70, 70, 76), 1.5)
            legend(x + 27, y + 17, n)
            job(x + 27, y + 36, what, PALE)
    x0, y0, w, h = 36, 340, 928, 270                           # the keys
    rect(x0, y0, w, h, 18, TRAY)
    pitch = (w - 24) / 16
    kw = pitch * .78
    wy, wh, by, bh = y0 + h * .46, h * .49, y0 + h * .05, h * .31
    def left(i): return x0 + 12 + i * pitch + (pitch - kw) / 2
    for i in range(16):                                        # 12 scenes; 4 gestures, each with SEL a bank
        rect(left(i), wy, kw, wh, kw / 2, KEY_W)
        kx = left(i) + kw / 2
        text(kx, wy + wh - 14, str(i + 1) if i < 8 else ("Tick", "Chord", "Snarl", "Stutter", "Weight", "Shift", "Clean", "Wash")[i - 8], 14 if i < 8 else 12.5, INK)
        if i >= 12:
            text(kx, wy + 22, ("A", "B", "C", "Presets")[i - 12], 10.5, MUTED_KEY, "SemiBold")
    for k, a in enumerate(BLACK_AFTER):
        cx = x0 + 12 + (a + 1) * pitch
        words, col = BLACK[k]
        rect(cx - kw / 2, by, kw, bh, kw / 2, KEY_B)
        if col:
            rect(cx - 9, by + 9, 18, 5, 2.5, col)
        for j, wd in enumerate(words):
            text(cx, by + 52 + (j - (len(words) - 1) / 2) * 14 + 4.5, wd, 12.5, INK)
    bracket(left(0), left(7) + kw, 451, "Presets & scenes · tap: go · SAVE + tap: store", LEGEND, 11.5, 4)
    bracket(left(8), left(11) + kw, 451, "Gestures · press", LEGEND, 11.5, 4)
    bracket(left(12), left(15) + kw, 451, "Gestures · with SEL: bank", LEGEND, 11.5, 4)
    return img


if __name__ == "__main__":
    main()
