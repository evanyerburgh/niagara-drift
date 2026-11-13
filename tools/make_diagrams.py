# SPDX-License-Identifier: GPL-3.0-only
"""The manual's drawings of the LFO's routing (docs/manual/media/lfo-*.svg, committed): one send's chain top to bottom
for phones, the whole map and Mods' three pieces, each wide and tall (tools/make_manual.py shows the tall one on a
phone), MIDI in (midi-in), the screen reader (reader) and the emulator's computer keys (emu-keys). In the site's colors, as lfo-chain.svg (drawn by hand). Run from the repo: python3 tools/make_diagrams.py"""
import sys

LFO, D1, D2, INK, MUTED, CARD, PALE = "#dcbcff", "#ffb04a", "#45e3d3", "#fbe9f2", "#d3b3c4", "#24081f", "#ffd6f0"
SANS = "Inter Tight, Helvetica Neue, Arial, sans-serif"
MONO = "IBM Plex Mono, Menlo, monospace"
OUT = "docs/manual/media/"


class Svg:
    def __init__(self, name, w, h, about):
        self.name, self.w, self.h, self.p = name, w, h, []
        self.add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" font-family="{SANS}">')
        self.add("<!-- SPDX-License-Identifier: GPL-3.0-only -->")
        self.add(f"<!-- {about} Drawn in the site's colors (web/manual/manual.css), as lfo-chain.svg. -->")
        self.add("<defs>")
        for mid, col in (("a", LFO), ("d", D1), ("e", D2), ("p", PALE)):
            self.add(f'  <marker id="{name}-{mid}" viewBox="0 0 8 8" refX="7" refY="4" markerWidth="7" markerHeight="7" '
                     f'orient="auto"><path d="M0 0 8 4 0 8z" fill="{col}"/></marker>')
        self.add("</defs>")
        self.add(f'<rect width="{w}" height="{h}" rx="16" fill="{CARD}"/>')

    def add(self, s): self.p.append(s)

    def arrow(self, d, col=LFO, dash=False, end=True):
        mid = {LFO: "a", D1: "d", D2: "e", PALE: "p"}[col]
        self.add(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="2"'
                 + (' stroke-dasharray="5 4"' if dash else "") + (f' marker-end="url(#{self.name}-{mid})"' if end else "") + "/>")

    def text(self, x, y, s, size=13, col=MUTED, anchor="start", weight=None, mono=False):
        w = f' font-weight="{weight}"' if weight else ""
        f = f' font-family="{MONO}"' if mono else ""
        self.add(f'<text x="{x}" y="{y}" font-size="{size}" fill="{col}" text-anchor="{anchor}"{w}{f}>{s}</text>')

    def step(self, x, y, s, anchor="middle"):     # a step's name, small capitals in the mono
        self.add(f'<text x="{x}" y="{y}" font-family="{MONO}" font-size="11" letter-spacing="1.3" fill="{MUTED}" '
                 f'text-anchor="{anchor}">{s}</text>')

    def pill(self, cx, cy, s, col=LFO, w=88):
        self.add(f'<rect x="{cx - w / 2}" y="{cy - 17}" width="{w}" height="34" rx="17" fill="none" stroke="{col}" stroke-width="1.5"/>')
        self.text(cx, cy + 5.5, s, 16, col, "middle", 600)

    def key(self, cx, cy, s, w=56):
        self.add(f'<rect x="{cx - w / 2}" y="{cy - 14}" width="{w}" height="28" rx="6" fill="{CARD}" stroke="{INK}" stroke-width="1.5"/>')
        self.text(cx, cy + 5, s, 14, INK, "middle", mono=True)

    def knob(self, cx, cy):
        self.add(f'<circle cx="{cx}" cy="{cy}" r="25" fill="#14000f"/>')
        self.add(f'<circle cx="{cx}" cy="{cy}" r="20" fill="#0a0a0b" stroke="#3a3a40" stroke-width="2"/>')
        self.add(f'<line x1="{cx}" y1="{cy - 16}" x2="{cx}" y2="{cy - 8}" stroke="#e8e8ea" stroke-width="3"/>')

    def times(self, cx, cy):
        self.add(f'<circle cx="{cx}" cy="{cy}" r="18" fill="none" stroke="{LFO}" stroke-width="1.5"/>')
        self.text(cx, cy + 6, "×", 18, LFO, "middle")

    def box(self, x, y, w, h, col=MUTED, dash=False):
        self.add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="none" stroke="{col}" stroke-opacity=".45" '
                 f'stroke-width="1.2"' + (' stroke-dasharray="4 4"' if dash else "") + "/>")

    def save(self):
        self.add("</svg>")
        open(OUT + self.name + ".svg", "w").write("\n".join(self.p) + "\n")


# ---- 1. one send, top to bottom (phones) ---------------------------------------------------------------------------
s = Svg("lfo-chain-tall", 360, 446, "One LFO send, top to bottom (the phone's lfo-chain.svg).")
s.pill(54, 34, "LFO I"); s.pill(166, 34, "LFO II")
s.arrow("M54 51 C54 70 110 64 110 84", end=False); s.arrow("M166 51 C166 70 110 64 110 84", end=False)
s.key(110, 98, "F#3"); s.step(172, 93, "WHICH LFO", "start"); s.text(172, 112, "I, II or both", 14)
s.arrow("M110 112 V148"); s.knob(110, 178); s.step(172, 173, "HOW MUCH", "start"); s.text(172, 192, "KNOB 1 · middle: none", 14)
s.arrow("M110 203 V234"); s.times(110, 256); s.step(172, 251, "SCALED BY", "start"); s.text(172, 270, "Movement (ALGORITHM)", 14)
s.arrow("M110 274 V306"); s.key(110, 320, "G#3"); s.step(172, 315, "MOVES", "start"); s.text(172, 334, "pitch or X-Wave", 14)
s.arrow("M110 334 C110 362 60 356 60 384", D1); s.arrow("M110 334 C110 362 200 356 200 384", D1, dash=True)
s.text(60, 408, "I pitch", 16, D1, "middle", 600); s.text(60, 426, "key off", 12.5, MUTED, "middle")
s.text(200, 408, "I X-Wave", 16, D1, "middle", 600); s.text(200, 426, "key on", 12.5, MUTED, "middle")
s.save()


# ---- 2. the whole map: one clock, two waves, five places ------------------------------------------------------------
TARGETS = [("I pitch · X-Wave", D1, "F#3", "G#3", "KNOB 1"), ("I cutoff · resonance", D1, "A#3", "C#4", "KNOB 2"),
           ("II pitch · X-Wave", D2, "F#4", "G#4", "KNOB 3"), ("II cutoff · resonance", D2, "A#4", "C#5", "KNOB 4"),
           ("Delay time", PALE, "D#5", "", "Mods KNOB 3")]

s = Svg("lfo-map", 760, 380, "The whole LFO: one clock, two waves, the five places they go (LFO sends, Mods).")
s.step(84, 34, "CLOCK"); s.step(250, 34, "WAVES"); s.step(452, 34, "× MOVEMENT"); s.step(624, 34, "MOVES")
s.pill(84, 190, "Clock", LFO, 96); s.text(84, 162, "SELECT: rate", 13, MUTED, "middle")
s.pill(250, 140, "LFO I"); s.text(250, 176, "OCT− shape", 13, MUTED, "middle")
s.pill(250, 250, "LFO II"); s.text(250, 286, "OCT+ shape", 13, MUTED, "middle")
s.arrow("M132 190 C170 190 170 140 202 140"); s.arrow("M132 190 C170 190 170 250 202 250")
# Speed mod: the waves back into the clock
s.arrow("M206 258 C150 300 84 296 84 211", PALE, dash=True)
s.text(132, 318, "Speed mod (Mods KNOB 1 · 2)", 13, PALE, "middle")
ys = [80, 150, 220, 290, 352]
for y, (name, col, src, what, knob) in zip(ys, TARGETS):
    for fy in (140, 250):
        s.add(f'<path d="M294 {fy} C380 {fy} 400 {y} 470 {y}" fill="none" stroke="{LFO}" stroke-width="1.4" stroke-opacity=".55"/>')
    s.key(500, y, src)
    s.arrow(f"M528 {y} H560", col)
    s.text(568, y - 1, name, 15, col, weight=600)
    s.text(568, y + 16, (f"{what} on · " if what else "") + knob, 12.5)
s.add(f'<line x1="452" y1="52" x2="452" y2="364" stroke="{LFO}" stroke-width="1.2" stroke-dasharray="3 5" stroke-opacity=".7"/>')
s.text(424, 372, "ALGORITHM", 11, MUTED, "end", mono=True)
s.save()

s = Svg("lfo-map-tall", 360, 548, "The whole LFO, top to bottom (the phone's lfo-map.svg).")
s.pill(180, 34, "Clock", LFO, 96); s.text(240, 39, "SELECT: rate", 13)
s.arrow("M180 51 C180 76 100 70 100 96"); s.arrow("M180 51 C180 76 260 70 260 96")
s.pill(100, 114, "LFO I"); s.text(100, 150, "OCT− shape", 13, MUTED, "middle")
s.pill(260, 114, "LFO II"); s.text(260, 150, "OCT+ shape", 13, MUTED, "middle")
s.text(180, 200, "× Movement (ALGORITHM)", 13, LFO, "middle")
ys = [236, 306, 376, 446, 516]
for y, (name, col, src, what, knob) in zip(ys, TARGETS):
    s.key(62, y, src)
    s.text(104, y - 1, name, 15, col, weight=600)
    s.text(104, y + 16, (f"{what} on · " if what else "") + knob, 12.5)
s.add(f'<path d="M100 158 V176 M260 158 V176 M16 176 H260 M16 176 V516" fill="none" stroke="{LFO}" stroke-width="2"/>')
for y in ys:
    s.arrow(f"M16 {y} H32", LFO)
s.save()


# ---- 3. Mods: three pieces -------------------------------------------------------------------------------------------
def speed(s, x, y):
    s.step(x + 112, y + 24, "SPEED MOD")
    s.pill(x + 112, y + 64, "Clock", LFO, 96)
    s.pill(x + 112, y + 154, "LFO I · II", LFO, 110)
    s.arrow(f"M{x + 112} {y + 81} V{y + 134}")
    s.arrow(f"M{x + 167} {y + 154} C{x + 214} {y + 154} {x + 214} {y + 64} {x + 163} {y + 64}", PALE, dash=True)
    s.text(x + 112, y + 200, "KNOB 1: how far, ±3 octaves", 13, MUTED, "middle")
    s.text(x + 112, y + 218, "KNOB 2: which LFO, I, II or both", 13, MUTED, "middle")


def delay(s, x, y):
    s.step(x + 112, y + 24, "DELAY MOD")
    s.pill(x + 112, y + 64, "LFO I · II", LFO, 110)
    s.arrow(f"M{x + 112} {y + 81} V{y + 96}", end=False)
    s.key(x + 112, y + 110, "D#5")
    s.arrow(f"M{x + 112} {y + 124} V{y + 137}")
    s.text(x + 112, y + 160, "Delay time", 16, PALE, "middle", 600)
    s.text(x + 112, y + 200, "KNOB 3: how far, ±25%", 13, MUTED, "middle")
    s.text(x + 112, y + 218, "× Movement; heard with delay mix", 13, MUTED, "middle")


def cross(s, x, y):
    s.step(x + 112, y + 24, "CROSS-MOD · NOT THE LFO")
    s.pill(x + 56, y + 64, "I", D1, 64); s.pill(x + 168, y + 64, "II", D2, 64)
    s.arrow(f"M{x + 90} {y + 58} H{x + 134}", D1); s.arrow(f"M{x + 134} {y + 72} H{x + 90}", D2)
    s.key(x + 112, y + 110, "F#5")
    s.text(x + 112, y + 144, "I moves II's cutoff", 13.5, INK, "middle")
    s.text(x + 112, y + 162, "bend each other (cross-FM)", 13.5, INK, "middle")
    s.text(x + 112, y + 180, "Snarl: the sound bends both", 13.5, INK, "middle")
    s.text(x + 112, y + 210, "KNOB 4: how deep", 13, MUTED, "middle")


s = Svg("lfo-mods", 760, 240, "Mods' three knobs: Speed mod, Delay mod and Cross-mod.")
for i, f in enumerate((speed, delay, cross)):
    s.box(16 + i * 248, 10, 232, 220, dash=(f is cross))
    f(s, 20 + i * 248, 4)
s.save()

s = Svg("lfo-mods-tall", 360, 720, "Mods' three pieces, top to bottom (the phone's lfo-mods.svg).")
for i, f in enumerate((speed, delay, cross)):
    s.box(16, 10 + i * 236, 328, 222, dash=(f is cross))
    f(s, 68, 6 + i * 236)
s.save()
# ---- 4. MIDI in: what arrives, and where it goes --------------------------------------------------------------------
MIDI_ROWS = [("Notes, channel 1", "drone I's pitch", D1), ("Notes, channel 2", "drone II's pitch, or the interval", D2),
             ("Clock", "Swell, LFO, delay, Wander, Scene time", LFO)]

s = Svg("midi-in", 760, 210, "MIDI in: notes on channels 1 and 2 tune the drones; a clock, with Sync on, sets the time.")
s.pill(100, 105, "DAW, drum machine", PALE, 172); s.text(100, 141, "or a keyboard", 13, MUTED, "middle")
s.key(254, 105, "USB · TRS", 96)
s.step(254, 74, "IN")
for y, (what, to, col) in zip((50, 105, 160), MIDI_ROWS):
    s.arrow(f"M302 105 C324 105 324 {y} 342 {y}", col, end=False)
    s.text(350, y + 5, what, 15, col, weight=600)
    s.arrow(f"M476 {y} H500", col)
    s.text(508, y + 5, to, 13.5, INK)
s.text(350, y + 24, "Sync on (Setup 1, KNOB 4)", 12.5)
s.save()

s = Svg("midi-in-tall", 360, 420, "MIDI in, top to bottom (the phone's midi-in.svg).")
s.pill(180, 34, "DAW, drum machine", PALE, 172); s.text(180, 70, "or a keyboard", 13, MUTED, "middle")
s.arrow("M180 78 V96"); s.key(180, 112, "USB · TRS", 96)
s.add(f'<path d="M180 126 V146 M24 146 H180 M24 146 V352" fill="none" stroke="{PALE}" stroke-width="2"/>')
for y, (what, to, col) in zip((180, 260, 340), MIDI_ROWS):
    s.arrow(f"M24 {y} H44", col)
    s.text(52, y + 5, what, 15, col, weight=600)
    words, line = to.split(" "), ""
    rows = []
    for w_ in words:
        if len(line + " " + w_) > 40:
            rows.append(line); line = w_
        else:
            line = (line + " " + w_).strip()
    rows.append(line)
    for j, r in enumerate(rows):
        s.text(52, y + 26 + j * 17, r, 13.5, INK)
s.text(52, 404, "Sync on (Setup 1, KNOB 4)", 12.5)
s.save()
# ---- 5. Hearing your screen: the FM-1, a web page, a voice ---------------------------------------------------------
s = Svg("reader", 760, 180, "The screen reader: the FM-1 over USB to the Screen Reader page, which speaks or hands the words on.")
s.step(80, 34, "PLUG IN"); s.step(330, 34, "OPEN, CONNECT"); s.step(610, 34, "LISTEN")
s.pill(80, 90, "FM-1", PALE, 100)
s.key(190, 90, "USB", 56)
s.arrow("M130 90 H160", PALE, end=False); s.arrow("M218 90 H246", PALE)
s.add(f'<rect x="250" y="62" width="160" height="56" rx="12" fill="none" stroke="{LFO}" stroke-width="1.5"/>')
s.text(330, 86, "Screen Reader", 16, LFO, "middle", 600); s.text(330, 106, "Chrome or Edge", 13, MUTED, "middle")
s.arrow("M410 90 C440 90 440 62 470 62", LFO); s.arrow("M410 90 C440 90 440 120 470 120", LFO)
s.text(480, 67, "its own voice", 15, INK, weight=600)
s.text(480, 125, "your screen reader", 15, INK, weight=600)
s.text(480, 143, "VoiceOver, NVDA, JAWS, TalkBack", 12.5)
s.save()

s = Svg("reader-tall", 360, 330, "The screen reader, top to bottom (the phone's reader.svg).")
s.pill(180, 34, "FM-1", PALE, 100); s.arrow("M180 51 V70", PALE, end=False); s.key(180, 84, "USB", 56); s.arrow("M180 98 V118", PALE)
s.add(f'<rect x="100" y="122" width="160" height="56" rx="12" fill="none" stroke="{LFO}" stroke-width="1.5"/>')
s.text(180, 146, "Screen Reader", 16, LFO, "middle", 600); s.text(180, 166, "Chrome or Edge", 13, MUTED, "middle")
s.arrow("M180 178 C180 200 90 196 90 222", LFO); s.arrow("M180 178 C180 200 270 196 270 222", LFO)
s.text(90, 246, "its own voice", 15, INK, "middle", 600)
s.text(270, 246, "your screen", 15, INK, "middle", 600); s.text(270, 264, "reader", 15, INK, "middle", 600)
s.text(270, 284, "VoiceOver, NVDA,", 12.5, MUTED, "middle"); s.text(270, 300, "JAWS, TalkBack", 12.5, MUTED, "middle")
s.save()
# ---- 6. Playing in a browser: the computer keys ---------------------------------------------------------------------
s = Svg("emu-keys", 900, 384, "The emulator's computer keys: black keys on the number row, scenes on Q to I, gestures on A to K, the buttons around them, the arrows for the left knobs.")
U, P_ = 46, 50
BLACKC, WHITEC, DARK = "#3a3a40", "#d9dadc", "#14000f"

def cap(x, y, w, letter, label, fill, ink, edge=None):
    s.add(f'<rect x="{x}" y="{y}" width="{w}" height="{U}" rx="7" fill="{fill}"' + (f' stroke="{edge}" stroke-width="1.5"' if edge else "") + "/>")
    s.add(f'<text x="{x + 6}" y="{y + 14}" font-family="{MONO}" font-size="10.5" fill="{ink}" fill-opacity=".7">{letter}</text>')
    if label:
        s.text(x + w / 2, y + 36, label, 13 if len(label) < 5 else 11, ink, "middle", 700)

def blank(x, y, w):
    s.add(f'<rect x="{x}" y="{y}" width="{w}" height="{U}" rx="7" fill="none" stroke="{MUTED}" stroke-opacity=".25"/>')

Y = [24, 84, 134, 184, 234, 284]
cap(24, Y[0], 60, "esc", "HOME", CARD, INK, INK)
for i, (k, lab) in enumerate(zip("`1234567890-=", ["", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", ""])):
    x = 24 + i * P_
    cap(x, Y[1], U, k, lab, BLACKC, INK) if lab else blank(x, Y[1], U)
blank(24, Y[2], 75)
for i, k in enumerate("QWERTYUIOP"):
    x = 103 + i * P_
    cap(x, Y[2], U, k, str(i + 1), WHITEC, DARK) if i < 8 else blank(x, Y[2], U)
blank(24, Y[3], 88)
for i, k in enumerate("ASDFGHJKL;"):
    x = 116 + i * P_
    cap(x, Y[3], U, k, str(i + 9), PALE, DARK, LFO if i < 4 else None) if i < 8 else blank(x, Y[3], U)
cap(616, Y[3], 96, "return", "SAVE", CARD, INK, INK)
blank(24, Y[4], 112)
for i, (k, lab) in enumerate(zip("ZXCVBNM", ["OCT−", "OCT+", "SEL", "EDIT", "", "", ""])):
    x = 140 + i * P_
    cap(x, Y[4], U, k, lab, CARD, INK, INK) if lab else blank(x, Y[4], U)
cap(190, Y[5], 300, "space", "PLAY", CARD, INK, INK)
for x, y, k in ((786, Y[4], "↑"), (736, Y[5], "←"), (786, Y[5], "↓"), (836, Y[5], "→")):
    cap(x, y, U, k, "", "none", LFO, LFO)
s.text(812, Y[5] + 64, "↑ ↓ LFO rate", 12.5, LFO, "middle")
s.text(812, Y[5] + 80, "← → Movement", 12.5, LFO, "middle")
s.text(700, Y[1] + 28, "black keys 1–11", 14, INK, weight=600)
s.text(620, Y[2] + 28, "scenes 1–8", 14, INK, weight=600)
s.text(726, Y[3] + 20, "gestures 9–16", 14, PALE, weight=600)
s.text(726, Y[3] + 37, "9–12 press · 13–16 hold", 12, MUTED)
s.save()
print("ok")
