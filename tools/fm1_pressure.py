#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Watch the FM-1's key pressure live, over USB-MIDI (Niagara Drift's test build, read-only).

  fm1_pressure.py [SECONDS] [--trace]   listen for SECONDS (default 60), print each press (--trace: every
                                        reading of it, in order), then a summary per key

The firmware sends, for each held key on the lower 20 (F3 .. C5), its pressure as polyphonic aftertouch
(the key as a note, F3 = 53) and its raw driven reading / 8 as CC (the key's number) on channel 16.
A lower raw reading is a harder press. Needs mido with python-rtmidi.
"""
import sys
import time

import mido

NAMES = ["F", "F#", "G", "G#", "A", "A#", "B", "C", "C#", "D", "D#", "E"]


def key_name(k):
    return f"{NAMES[k % 12]}{3 + (k + 5) // 12}"


def main():
    trace = "--trace" in sys.argv
    args = [a for a in sys.argv[1:] if a != "--trace"]
    secs = float(args[0]) if args else 60.0
    ports = [n for n in mido.get_input_names() if "fm-1" in n.lower() or "niagara" in n.lower()]
    if not ports:
        sys.exit("fm1_pressure: no FM-1 MIDI port")
    raw, pct = {}, {}            # the press under way: key -> list of readings
    seen = {}                    # key -> (lightest, firmest raw, highest %) over all presses
    with mido.open_input(ports[0]) as port:
        print(f"listening on {ports[0]} for {secs:g} s: press keys F3 .. C5")
        end = time.monotonic() + secs
        while time.monotonic() < end:
            for m in port.iter_pending():
                if m.type == "control_change" and m.channel == 15 and m.control < 27:
                    k = m.control
                    if m.value:
                        raw.setdefault(k, []).append(m.value * 8)
                    elif k in raw:                         # let go: report the press
                        r, p = raw.pop(k), pct.pop(k, [0])
                        lo, hi, top = seen.get(k, (0, 9999, 0))
                        seen[k] = (max(lo, max(r)), min(hi, min(r)), max(top, max(p)))
                        print(f"{key_name(k):4} raw {max(r):4} .. {min(r):4}   pressure up to "
                              f"{round(max(p) * 100 / 127):3}%", flush=True)
                        if trace:                          # the press as it went, every reading in order
                            print("     " + " ".join(str(v) for v in r), flush=True)
                elif m.type == "polytouch" and 53 <= m.note < 80 and m.value:
                    pct.setdefault(m.note - 53, []).append(m.value)
            time.sleep(0.002)
    print("\nper key: the lightest and firmest raw readings seen, and the highest pressure")
    for k in sorted(seen):
        lo, hi, top = seen[k]
        print(f"{key_name(k):4} {lo:4} .. {hi:4}   up to {round(top * 100 / 127)}%")


if __name__ == "__main__":
    main()
