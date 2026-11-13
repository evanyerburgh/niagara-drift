#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The README's pictures: runs tests/scenarios/pages.omni in the host simulator (each page with values that
# show something) and puts each button's page, or its two pages side by side, in docs/img/page-<name>.png, and each
# page alone in docs/img/screen-<shot>.png (the manual's Reference, a picture in each page's section);
# and draws the FM-1 itself (the emulator's geometry, the Drones page on its screen) with what every knob,
# button and key does in Niagara Drift written on it: docs/img/panel.png.
#   sh host/build_host.sh && python3 tools/make_screens.py
import os, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUTTONS = [("play", ["play", "play2"]), ("tone", ["life", "tone"]), ("osc", ["osc", "source"]), ("seq", ["partner", "partner2"]),
           ("lfo", ["lfo", "lfo2"]), ("swell", ["pulse"]), ("effects", ["fx", "fx-sel", "fx2"]), ("setup", ["setup", "setup2"]), ("keys", ["keymap"])]
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
            one = os.path.join(ROOT, "docs", "img", "screen-%s.png" % shot)    # each page alone, for the manual's sections
            scr.save(one, optimize=True)
        dst = os.path.join(ROOT, "docs", "img", "page-%s.png" % name)
        img.save(dst, optimize=True)
        print("screens:", os.path.relpath(dst, ROOT), img.size)
    for n, first, bank in ((1, 1, 14), (2, 9, 15)):           # the Presets' two banks on the keys (the manual's Presets)
        dst = os.path.join(ROOT, "docs", "img", "keys-presets-%d.png" % n)
        preset_keys(first, bank).save(dst, optimize=True)
        print("keys:", os.path.relpath(dst, ROOT))
    dst = os.path.join(ROOT, "docs", "img", "keys-gestures.png")   # the gesture keys (the manual's Gestures)
    gesture_keys().save(dst, optimize=True)
    print("keys:", os.path.relpath(dst, ROOT))
    for n, only in (("press", (8, 11)), ("hold", (12, 15))):      # each kind alone (the manual's Gestures sections)
        dst = os.path.join(ROOT, "docs", "img", "keys-gestures-%s.png" % n)
        gesture_keys(only).save(dst, optimize=True)
        print("keys:", os.path.relpath(dst, ROOT))
    for n, banks in (("scenes", 0), ("banks", 1)):             # the white keys' two jobs (the manual's Scenes and banks)
        dst = os.path.join(ROOT, "docs", "img", "keys-%s.png" % n)
        scene_keys(banks).save(dst, optimize=True)
        print("keys:", os.path.relpath(dst, ROOT))
    GROUPS = {0: [(0, 2, "Drone I", AMBER), (3, 4, "Both", LABEL), (5, 7, "Drone II", TEAL), (8, 10, "Both", LABEL)],
              1: [(0, 3, "Drone I", AMBER), (4, 4, "Both", LABEL), (5, 8, "Drone II", TEAL), (9, 9, "Delay", LILAC),
                  (10, 10, "Both", LABEL)]}
    for n, lfo in (("drones", 0), ("lfo", 1)):                # the black keys' two sets of jobs (the manual's Black keys)
        dst = os.path.join(ROOT, "docs", "img", "black-keys-%s.png" % n)
        black_keys(lfo, groups=GROUPS[lfo]).save(dst, optimize=True)
        print("keys:", os.path.relpath(dst, ROOT))
    for n, lfo, only in (("filter", 0, (2, 7)), ("sub", 0, (1, 6)), ("partner", 0, (0, 5)), ("link", 0, (8,)),
                         ("sends", 1, (0, 1, 2, 3, 5, 6, 7, 8))):   # a page's own keys picked out
        dst = os.path.join(ROOT, "docs", "img", "black-keys-%s.png" % n)
        black_keys(lfo, only).save(dst, optimize=True)
        print("keys:", os.path.relpath(dst, ROOT))
    for n, rows in KNOB_PAGES:                                 # each Reference page's knobs, its jobs under them
        dst = os.path.join(ROOT, "docs", "img", "knobs-%s.png" % n)
        page_knobs(rows).save(dst, optimize=True)
        print("knobs:", os.path.relpath(dst, ROOT))
    pm = panel(os.path.join(out, "play.png"))
    dst = os.path.join(ROOT, "docs", "img", "panel.png")
    pm.save(dst, optimize=True)
    print("panel:", os.path.relpath(dst, ROOT), pm.size)
    for part, box in PARTS.items():                            # its sections, for the manual's Reference
        dst = os.path.join(ROOT, "docs", "img", "panel-%s.png" % part)
        pm.crop(tuple(int(round(v * 2.6)) for v in box)).save(dst, optimize=True)
        print("panel:", os.path.relpath(dst, ROOT))


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
ROW1 = [("FX", "Space"), ("SEL", "Fine"), ("ENV", "Life"), ("LFO", "Sends"), ("EDIT", "Osc"), ("GLO", "Setup")]
ROW2 = [("HOME", "Drones"), ("SAVE", "Store"), ("ARP", "Swell"), ("SEQ", "Partner"), ("PLAY", "On/off"), ("REC", "Loop")]
BLACK_AFTER = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14]
BLACK = [(["Partner"], AMBER), (["Sub"], AMBER), (["Filter"], AMBER), (["Pitch", "lock"], None), (["LFO", "reset"], None),
         (["Partner"], TEAL), (["Sub"], TEAL), (["Filter"], TEAL), (["Pitch", "link"], None), (["Hard", "sync"], None),
         (["X-mod"], None)]


PARTS = {"buttons": (560, 154, 978, 312), "knobs": (560, 46, 978, 154), "left": (44, 52, 290, 324), "keys": (30, 334, 970, 616)}   # in panel units


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
    for x, n, w in ((72, "OCT−", "LFO I"), (170, "OCT+", "LFO II")):
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
        if i >= 8:                                             # SEL + each gesture key picks a bank (since #81)
            text(kx, wy + 22, ("A", "B", "C", "D", "E", "F", "P1–8", "P9–16")[i - 8], 10.5, MUTED_KEY, "SemiBold")
    for k, a in enumerate(BLACK_AFTER):
        cx = x0 + 12 + (a + 1) * pitch
        words, col = BLACK[k]
        rect(cx - kw / 2, by, kw, bh, kw / 2, KEY_B)
        if col:
            rect(cx - 9, by + 9, 18, 5, 2.5, col)
        for j, wd in enumerate(words):
            text(cx, by + 52 + (j - (len(words) - 1) / 2) * 14 + 4.5, wd, 12.5, INK)
    bracket(left(0), left(7) + kw, 451, "Presets & scenes · tap: go · SAVE + tap: store", LEGEND, 11.5, 4)
    bracket(left(8), left(11) + kw, 451, "Gestures · press · SEL: bank", LEGEND, 11.5, 4)
    bracket(left(12), left(15) + kw, 451, "Gestures · hold · SEL: bank", LEGEND, 11.5, 4)
    return img


# each Reference page's four knobs, its two pages' jobs under each (the manual's Knobs sections): (file, [(page, per
# drone, [KNOB 1..4])]); a row per drone in amber and teal, a row for the whole sound pale
KNOB_PAGES = [
    ("drones", [("Drones", 1, ["Level", "Cutoff", "Level", "Cutoff"]), ("Pitch", 1, ["Pitch", "Resonance", "Pitch", "Resonance"])]),
    ("lfo", [("LFO sends", 1, ["Pitch", "Cutoff", "Pitch", "Cutoff"]), ("Mods", 0, ["Speed mod", "Source", "Delay mod", "Cross-mod"])]),
    ("partner", [("Partner", 1, ["Interval", "Detune", "Interval", "Detune"]), ("Pitch moves", 0, ["Shift", "Glide", "II wander", "Scale"])]),
    ("osc", [("Osc", 1, ["X-Wave", "Width", "X-Wave", "Width"]), ("Sub · Drive", 1, ["Osc/sub", "Drive", "Osc/sub", "Drive"])]),
    ("life", [("Life", 0, ["Feedback", "Drift", "Character", "Age"]), ("Tone", 1, ["Noise", "Color", "Noise", "Color"])]),
    ("setup", [("Setup 1", 0, ["Fade", "Morph", "Scene time", "Sync"]), ("Setup 2", 0, ["Tuning", "Tune", "Stereo", "Pressure"])]),
    ("swell", [("Swell", 0, ["Rate", "Depth", "Shape", "Swing"])]),
    ("space", [("Space", 0, ["Repeats", "Delay mix", "Length", "Reverb mix"]), ("Details", 0, ["Drive", "Time", "Damp", "Type"])]),
]


def page_knobs(rows):
    """KNOB 1-4 as on the panel, each page's job for each knob under it, the page's name at the row's left; "Drone I"
    and "Drone II" under them when every row is per drone (else the colors alone: a pale row is the whole sound's)."""
    S = 2.6
    def P(v): return int(round(v * S))
    lw, kx0, step, r, lh = 96, 160, 112, 22, 24
    drones = all(per for _, per, _ in rows)
    w = kx0 + 3 * step + 70
    h = 104 + lh * len(rows) + (30 if drones else 6)
    img = Image.new("RGBA", (P(w), P(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    def f(size, wt="SemiBold"):
        return ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-%s.ttf" % wt), P(size))
    def text(x, y, t, size, fill, wt="SemiBold", anchor="ms"):
        d.text((P(x), P(y)), t, font=f(size, wt), fill=fill, anchor=anchor)
    d.rounded_rectangle([0, 0, P(w), P(h)], radius=P(18), fill=TRAY)
    for i in range(4):
        x, y = kx0 + i * step, 60
        d.ellipse([P(x - r - 5), P(y - r - 5), P(x + r + 5), P(y + r + 5)], fill=BODY)
        d.ellipse([P(x - r), P(y - r), P(x + r), P(y + r)], fill=KNOB, outline=(58, 58, 64), width=P(2))
        d.line([P(x), P(y - r + 5), P(x), P(y - r + 15)], fill=LABEL, width=P(3))
        text(x, y - r - 12, "KNOB %d" % (i + 1), 11, LEGEND)
    for j, (page, per, jobs) in enumerate(rows):
        y = 112 + j * lh
        text(lw - 4 + 8, y, page, 12.5, LEGEND, anchor="rs")
        for i, t in enumerate(jobs):
            col = (AMBER if i < 2 else TEAL) if per else PALE
            text(kx0 + i * step, y, t, 15, col)
    if drones:
        y = h - 16
        for a, b, t, col in ((kx0 - 46, kx0 + step + 46, "Drone I", AMBER), (kx0 + 2 * step - 46, kx0 + 3 * step + 46, "Drone II", TEAL)):
            tw = d.textlength(t, font=f(13)) / S / 2
            m = (a + b) / 2
            text(m, y + 4.5, t, 13, col)
            for p, q in ((a, m - tw - 8), (m + tw + 8, b)):
                d.line([P(p), P(y), P(q), P(y)], fill=col, width=P(1.2))
            for x in (a, b):
                d.line([P(x), P(y), P(x), P(y - 4)], fill=col, width=P(1.2))
    return img


BLACK_LFO = [["Pitch", "LFO"], ["Pitch to", "X-Wave"], ["Cutoff", "LFO"], ["Cutoff", "to reso"], None,   # on the LFO pages
             ["Pitch", "LFO"], ["Pitch to", "X-Wave"], ["Cutoff", "LFO"], ["Cutoff", "to reso"], ["Delay", "LFO"], None]  # (ui.c SW_LFO)
BLACK_NOTES = ["F#3", "G#3", "A#3", "C#4", "D#4", "F#4", "G#4", "A#4", "C#5", "D#5", "F#5"]


def black_keys(lfo, only=None, groups=None):
    """The eleven black keys alone, named for their jobs: normally (the drones), or on the LFO pages, where the keys that
    change are named in the LFO's lilac; each key's note under it. only: the keys a page is about, ringed, the rest
    quiet. groups: (first key, last key, words, color) bracketed above them (the manual's Black keys)."""
    S = 2.6
    def P(v): return int(round(v * S))
    top = 34 if groups else 0
    w, h = 928, 150 + top
    img = Image.new("RGBA", (P(w), P(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    def f(size, wt="Bold"):
        return ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-%s.ttf" % wt), P(size))
    def rect(x, y, rw, rh, r, fill, outline=None, width=0):
        d.rounded_rectangle([P(x), P(y), P(x + rw), P(y + rh)], radius=P(r), fill=fill, outline=outline,
                            width=P(width) if width else 0)
    def text(x, y, s, size, fill, wt="Bold"):
        d.text((P(x), P(y)), s, font=f(size, wt), fill=fill, anchor="ms")
    rect(0, 0, w, h, 18, TRAY)
    pitch = (w - 24) / 16
    kw = pitch * .78
    by, bh = 14 + top, 96
    for k, a in enumerate(BLACK_AFTER):
        cx = 12 + (a + 1) * pitch
        words, col = BLACK[k]
        if lfo and BLACK_LFO[k]:
            words, face, ink = BLACK_LFO[k], LILAC, INK
        else:
            face, ink = KEY_B, INK
        ring = None
        if only is not None:
            if k in only:
                ring = PALE
            else:
                face, ink, col = (70, 70, 76), (120, 120, 126), None
        rect(cx - kw / 2, by, kw, bh, kw / 2, face, ring, 3 if ring else 0)
        if lfo and BLACK_LFO[k] and k < 9:                     # on the LFO pages keys 1-4 are drone I's, 6-9 drone II's
            col = AMBER if k < 4 else TEAL
        if col:
            rect(cx - 9, by + 9, 18, 5, 2.5, col)
        for j, wd in enumerate(words):
            text(cx, by + 52 + (j - (len(words) - 1) / 2) * 14 + 4.5, wd, 12 if lfo and BLACK_LFO[k] else 12.5, ink)
        text(cx, by + bh + 26, BLACK_NOTES[k], 13, PALE if ring else LEGEND, "SemiBold")
    for a, b, words, col in groups or ():
        x1 = 12 + (BLACK_AFTER[a] + 1) * pitch - kw / 2
        x2 = 12 + (BLACK_AFTER[b] + 1) * pitch + kw / 2
        m, y = (x1 + x2) / 2, 30
        text(m, 22, words, 16, col, "SemiBold")
        d.line([P(x1), P(y), P(x2), P(y)], fill=col, width=P(1.2))
        for x in (x1, x2):
            d.line([P(x), P(y), P(x), P(y + 5)], fill=col, width=P(1.2))
    return img


def preset_keys(first, bank):
    """The keyboard alone, as the panel draws it: white keys 1-8 named P<first>.. and the gesture key that picks their
    bank lit, "SEL +" on it; the other keys quiet."""
    S = 2.6
    def P(v): return int(round(v * S))
    x0, y0, w, h = 0, 0, 928, 300
    img = Image.new("RGBA", (P(w), P(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    def f(size, wt="Bold"):
        return ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-%s.ttf" % wt), P(size))
    def rect(x, y, rw, rh, r, fill, outline=None, width=0):
        d.rounded_rectangle([P(x), P(y), P(x + rw), P(y + rh)], radius=P(r), fill=fill, outline=outline,
                            width=P(width) if width else 0)
    def text(x, y, s, size, fill, wt="Bold"):
        d.text((P(x), P(y)), s, font=f(size, wt), fill=fill, anchor="ms")
    rect(x0, y0, w, h, 18, TRAY)
    pitch = (w - 24) / 16
    kw = pitch * .78
    top = 30                                                   # room for the captions over the keys
    wy, wh, by, bh = top + (h - top) * .46, (h - top) * .49, top + (h - top) * .05, (h - top) * .31
    def left(i): return x0 + 12 + i * pitch + (pitch - kw) / 2
    QUIET = (120, 120, 126)
    for i in range(16):
        kx = left(i) + kw / 2
        if i < 8:
            rect(left(i), wy, kw, wh, kw / 2, KEY_W)
            text(kx, wy + wh - 16, "P%d" % (first + i), 15, INK)
        elif i == bank:
            rect(left(i), wy, kw, wh, kw / 2, PALE, LILAC, 3)
            text(kx, wy + wh - 30, "SEL +", 12, INK, "SemiBold")
            text(kx, wy + wh - 14, ("F5", "G5")[bank - 14], 15, INK)
        else:
            rect(left(i), wy, kw, wh, kw / 2, QUIET)
    for a in BLACK_AFTER:
        cx = x0 + 12 + (a + 1) * pitch
        rect(cx - kw / 2, by, kw, bh, kw / 2, (70, 70, 76))
    def cap(x1, x2, s, col):
        m = (x1 + x2) / 2
        text(m, 22, s, 14, col, "SemiBold")
    cap(left(0), left(7) + kw, "Presets P%d–P%d · tap: play" % (first, first + 7), LABEL)
    cap(left(bank) - 40, left(bank) + kw + 40, "bank", LILAC)
    return img


BANKS = ("A", "B", "C", "D", "E", "F", "P1–8", "P9–16")
GESTURE_NOTES = ("G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5")


def scene_keys(banks):
    """The keyboard alone for the manual's Scenes and banks: the scene keys 1-8 lit with what a tap and a press do
    (banks 0), or the eight gesture keys lit with the bank each picks with SEL, the scene keys quiet (banks 1): no
    notes on them, as "A" on G4 read as two names (Evan, 2026-11-13)."""
    S = 2.6
    def P(v): return int(round(v * S))
    w, h = 928, 300
    img = Image.new("RGBA", (P(w), P(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    def f(size, wt="Bold"):
        return ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-%s.ttf" % wt), P(size))
    def rect(x, y, rw, rh, r, fill, outline=None, width=0):
        d.rounded_rectangle([P(x), P(y), P(x + rw), P(y + rh)], radius=P(r), fill=fill, outline=outline,
                            width=P(width) if width else 0)
    def text(x, y, t, size, fill, wt="Bold"):
        d.text((P(x), P(y)), t, font=f(size, wt), fill=fill, anchor="ms")
    rect(0, 0, w, h, 18, TRAY)
    pitch = (w - 24) / 16
    kw = pitch * .78
    top = 40
    wy, wh, by, bh = top + (h - top) * .46, (h - top) * .49, top + (h - top) * .05, (h - top) * .31
    def left(i): return 12 + i * pitch + (pitch - kw) / 2
    QUIET = (120, 120, 126)
    for i in range(16):
        kx = left(i) + kw / 2
        if i < 8 and not banks:
            rect(left(i), wy, kw, wh, kw / 2, KEY_W)
            text(kx, wy + wh - 16, str(i + 1), 20, INK)
        elif i >= 8 and banks:
            rect(left(i), wy, kw, wh, kw / 2, PALE, LILAC, 2)
            b = BANKS[i - 8]                                   # the bank's name only: a key's note (G4) beside
            if b.startswith("P"):                              # a bank's letter (A) read as one more name
                text(kx, wy + wh - 40, "Presets", 11.5, MUTED_KEY, "SemiBold")
                text(kx, wy + wh - 16, b[1:], 15, INK)
            else:
                text(kx, wy + wh - 40, "bank", 12, MUTED_KEY, "SemiBold")
                text(kx, wy + wh - 16, b, 22, INK)
        else:
            rect(left(i), wy, kw, wh, kw / 2, QUIET)
    for a in BLACK_AFTER:
        cx = 12 + (a + 1) * pitch
        rect(cx - kw / 2, by, kw, bh, kw / 2, (70, 70, 76))
    if banks:
        m = (left(8) + left(15) + kw) / 2
        text(m, 28, "hold SEL, tap a key: its bank", 20, LILAC, "SemiBold")
    else:
        m = (left(0) + left(7) + kw) / 2
        text(m, 28, "Scenes · tap: glide · press: crossfade", 20, LABEL, "SemiBold")
    return img


GESTURES = ("Tick", "Chord", "Snarl", "Stutter", "Weight", "Shift", "Clean", "Wash")


def gesture_keys(only=None):
    """The keyboard alone for the manual's Gestures: keys 9-16 lit with their gestures and notes, "press" over 9-12 and
    "hold" over 13-16; the scene keys and the black keys quiet. only: (8, 11) or (12, 15), that kind lit alone (the
    manual's two sections)."""
    S = 2.6
    def P(v): return int(round(v * S))
    w, h = 928, 300
    img = Image.new("RGBA", (P(w), P(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    def f(size, wt="Bold"):
        return ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-%s.ttf" % wt), P(size))
    def rect(x, y, rw, rh, r, fill, outline=None, width=0):
        d.rounded_rectangle([P(x), P(y), P(x + rw), P(y + rh)], radius=P(r), fill=fill, outline=outline,
                            width=P(width) if width else 0)
    def text(x, y, t, size, fill, wt="Bold"):
        d.text((P(x), P(y)), t, font=f(size, wt), fill=fill, anchor="ms")
    rect(0, 0, w, h, 18, TRAY)
    pitch = (w - 24) / 16
    kw = pitch * .78
    top = 40
    wy, wh, by, bh = top + (h - top) * .46, (h - top) * .49, top + (h - top) * .05, (h - top) * .31
    def left(i): return 12 + i * pitch + (pitch - kw) / 2
    for i in range(16):
        kx = left(i) + kw / 2
        if i >= 8 and (only is None or only[0] <= i <= only[1]):
            rect(left(i), wy, kw, wh, kw / 2, KEY_W)
            text(kx, wy + wh - 36, GESTURES[i - 8], 14, INK)
            text(kx, wy + wh - 14, GESTURE_NOTES[i - 8], 14, MUTED_KEY, "SemiBold")
        else:
            rect(left(i), wy, kw, wh, kw / 2, (120, 120, 126))
    for a in BLACK_AFTER:
        cx = 12 + (a + 1) * pitch
        rect(cx - kw / 2, by, kw, bh, kw / 2, (70, 70, 76))
    for a, b, t in ((8, 11, "pressure: press harder"), (12, 15, "hold: hold longer")):
        if only is not None and (a, b) != tuple(only):
            continue
        x1, x2, y = left(a), left(b) + kw, 34
        m = (x1 + x2) / 2
        text(m, 28, t, 18, LABEL, "SemiBold")
        for x in (x1, x2):
            d.line([P(x), P(y), P(x), P(y + 5)], fill=LEGEND, width=P(1.2))
        d.line([P(x1), P(y), P(x2), P(y)], fill=LEGEND, width=P(1.2))
    return img


if __name__ == "__main__":
    main()
