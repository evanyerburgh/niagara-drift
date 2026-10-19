#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The README's pictures: runs tests/scenarios/pages.omni in the host simulator (each page with values that
# show something) and puts each button's page, or its two pages side by side, in docs/img/page-<name>.png;
# and draws the FM-1 itself (the emulator's geometry, the Play page on its screen) with what every knob,
# button and key does in Niagara Drift written on it: docs/img/panel.png.
#   sh host/build_host.sh && python3 tools/make_screens.py
import os, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUTTONS = [("play", ["play", "play2"]), ("tone", ["tone", "tone2"]), ("osc", ["osc", "source"]), ("seq", ["partner", "partner2"]),
           ("lfo", ["lfo", "lfo2"]), ("swell", ["pulse"]), ("effects", ["fx", "fx2"]), ("setup", ["setup", "setup2"]), ("keys", ["keymap"])]
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


# the panel, in the emulator's units (web/emu/index.html: a 1000 x 640 drawing)
BODY, TRAY, KNOB, BTN, LABEL = (58, 61, 66), (38, 40, 44), (31, 33, 36), (46, 49, 53), (232, 232, 234)
KEY_W, KEY_B = (217, 218, 220), (201, 202, 205)
AMBER, TEAL, LILAC, PALE, INK = (255, 176, 74), (69, 227, 211), (201, 155, 255), (255, 214, 240), (20, 0, 15)
KNOBS = [(105, 100, "MASTER", "Volume", PALE), (215, 100, "SELECT", "LFO rate", LILAC),
         (105, 205, "PRESETS", "Gesture time", PALE), (215, 205, "ALGORITHM", "Movement", LILAC)] + \
        [(610 + i * 104, 100, "KNOB%d" % (i + 1), "Drone I" if i < 2 else "Drone II", AMBER if i < 2 else TEAL)
         for i in range(4)]
ROW1 = [("FX", "Delay · Rev"), ("SEL", "Fine"), ("ENV", "Tone"), ("LFO", "LFO"), ("EDIT", "Osc · Src"), ("GLO", "Setup")]
ROW2 = [("HOME", "Play"), ("SAVE", "Save"), ("ARP", "Swell"), ("SEQ", "Partner"), ("PLAY", "On · off"),
        ("REC", "Loop")]
BLACK_AFTER = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14]
BLACK = [("Partner", AMBER), ("Sub", AMBER), ("Filter", AMBER), ("Pitch lock", None), ("LFO reset", None),
         ("Partner", TEAL), ("Sub", TEAL), ("Filter", TEAL), ("Pitch link", None), ("Hard sync", None),
         ("X-mod", None)]


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
    def pill(cx, cy, text, fg, bg, size=15):   # 15 units: ~15 px where GitHub shows the picture (~980 px wide)
        fo = f(size)
        w = d.textlength(text, font=fo) / S + 12
        rect(cx - w / 2, cy - size * .7, w, size * 1.4, size * .7, bg)
        d.text((P(cx), P(cy)), text, font=fo, fill=fg, anchor="mm")
    rect(4, 4, 992, 632, 44, BODY)
    for x, y, name, job, col in KNOBS:                        # knobs: the printed name above, the job below
        d.ellipse([P(x - 29), P(y - 29), P(x + 29), P(y + 29)], fill=TRAY)
        d.ellipse([P(x - 24), P(y - 24), P(x + 24), P(y + 24)], fill=KNOB, outline=(68, 71, 76), width=P(2))
        d.line([P(x), P(y - 19), P(x), P(y - 9)], fill=LABEL, width=P(3))
        d.text((P(x), P(y - 38)), name, font=f(16, "SemiBold"), fill=(205, 207, 212), anchor="mm")
        pill(x, y + 40, job, INK, col)
    rect(58, 262, 208, 56, 16, TRAY)                           # OCT- / OCT+: the LFO waves
    for x, n in ((72, "OCT-"), (170, "OCT+")):
        rect(x, 268, 82, 38, 9, BTN, (74, 77, 82), 1.5)
        d.text((P(x + 41), P(287)), n, font=f(15), fill=LABEL, anchor="mm")
    pill(162, 326, "LFO wave I · wave II", INK, LILAC, 14)
    rect(300, 44, 236, 236, 26, (20, 21, 24))                   # the screen
    scr = Image.open(play_png).convert("RGB").resize((P(200), P(200)), Image.LANCZOS)
    img.paste(scr, (P(318), P(62)))
    rect(566, 160, 384, 138, 22, TRAY)                         # the buttons: printed name, its job under it
    for row, (y, names) in enumerate(((174, ROW1), (236, ROW2))):
        for i, (n, job) in enumerate(names):
            x = 582 + i * 63
            rect(x, y, 54, 48, 9, BTN, (74, 77, 82), 1.5)
            d.text((P(x + 27), P(y + 15)), n, font=f(15 if len(n) < 5 else 13.5), fill=LABEL, anchor="mm")
            d.text((P(x + 27), P(y + 34)), job, font=f(14 if len(job) < 8 else 12.5), fill=PALE, anchor="mm")
    x0, y0, w, h = 36, 340, 928, 270                           # the keys
    rect(x0, y0, w, h, 18, TRAY)
    pitch = (w - 24) / 16
    kw = pitch * .78
    for i in range(16):
        x = x0 + 12 + i * pitch + (pitch - kw) / 2
        rect(x, y0 + h * .43, kw, h * .52, kw / 2, KEY_W)
        label = str(i + 1) if i < 12 else ("Weight", "Shift", "Clean", "Wash")[i - 12]   # 12 scenes, 4 gestures
        d.text((P(x + kw / 2), P(y0 + h * .43 + h * .52 - 22)), label, font=f(16 if i < 12 else 12), fill=(60, 62, 68),
               anchor="mm")
    for k, a in enumerate(BLACK_AFTER):
        x = x0 + 12 + (a + 1) * pitch - kw / 2
        job, col = BLACK[k]
        rect(x, y0 + h * .06, kw, h * .33, kw / 2, KEY_B)
        if col:
            rect(x + kw / 2 - 9, y0 + h * .06 + 10, 18, 6, 3, col)
        words = job.split(" ")
        for j, wd in enumerate(words):
            d.text((P(x + kw / 2), P(y0 + h * .06 + h * .33 / 2 + (j - (len(words) - 1) / 2) * 16 + 5)), wd,
                   font=f(15 if len(wd) < 7 else 13.5), fill=INK, anchor="mm")
    pill(x0 + 12 + 6 * pitch, y0 + h * .43 + h * .24, "12 scenes · Tap: go there · Hold SAVE, then tap: store", INK,
         PALE, 17)
    pill(x0 + 12 + 14 * pitch, y0 + h * .43 + h * .24, "Gestures · SEL + key: bank", INK, LILAC, 14)
    return img


if __name__ == "__main__":
    main()
