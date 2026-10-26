#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The MIDI clock as the FM-1 hears it, over USB-MIDI (Niagara Drift's firmware, read-only; v0.3, step 3).

  fm1_clock.py [SECONDS]    poll for SECONDS (default 30): the tempo and the jitter twice a second, the audio load
                            beside them, then a summary

The tempo and jitter are the FM-1's own reading (the last two beats: a straight line through the pulses' times,
how far each sits off it). Send a clock to the FM-1 from a DAW meanwhile. Needs mido with python-rtmidi.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fm1_install import PORT_RE, MidoBackend, pack7, pair_output, unpack7  # noqa: E402


def query(cmd):
    return b"\xF0" + pack7(bytes([0x00, 0x59, cmd, 0, 0, 0, 0xFF])) + b"\xF7"


def parse(pkt, cmd):
    d = unpack7(pkt[1:-1])
    if len(d) != 39 or d[:3] != bytes([0x00, 0x59, cmd]) or ~sum(d[6:38]) & 0xFF != d[38]:
        return None
    return [int.from_bytes(d[6 + 4 * i:10 + 4 * i], "little") for i in range(8)]


def ask(link, cmd):
    link.drain()
    link.send(query(cmd))
    t = time.monotonic() + 1.0
    while (left := t - time.monotonic()) > 0:
        p = link.read(left)
        r = p and parse(p, cmd)
        if r:
            return r
    return None


def main():
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 30.0
    be = MidoBackend()
    ins = [n for n in be.input_names() if PORT_RE.search(n)]
    if not ins:
        sys.exit("fm1_clock: no FM-1 MIDI port")
    out = pair_output(ins[0], [n for n in be.output_names() if PORT_RE.search(n)])
    link = be.open(ins[0], out)
    rows, first_cpu, last_cpu = [], None, None
    try:
        end = time.monotonic() + secs
        print("  time   tempo     jitter (RMS / largest)   pulses   load")
        while time.monotonic() < end:
            c = ask(link, 0x7C)
            u = ask(link, 0x7E)
            if c is None:
                sys.exit("fm1_clock: no reply (not a build with the clock readout)")
            if u:
                first_cpu = first_cpu or u
                last_cpu = u
            t = secs - (end - time.monotonic())
            lost = c[7] == 0xFFFFFFFF or c[7] > 500
            load = f"{u[0] * 100 / 256:5.1f} %" if u else "    -"
            if lost:
                print(f"{t:6.1f}   no clock{'':31}{load}")
            elif c[3] < 48:
                print(f"{t:6.1f}   listening ({c[3]} pulses){'':16}{load}")
            else:
                rows.append((c[0] / 100, c[1] / 1000, c[2] / 1000))
                print(f"{t:6.1f}   {c[0] / 100:6.2f}   {c[1] / 1000:5.2f} / {c[2] / 1000:5.2f} ms{'':12}{c[3]:6d}   {load}")
            time.sleep(0.5)
        print(f"Start {c[4]}, Continue {c[5]}, Stop {c[6]} received since the FM-1 started")
    finally:
        link.close()
    if rows:
        bpm = [r[0] for r in rows]
        print(f"tempo     {sum(bpm) / len(bpm):.2f} BPM average, {min(bpm):.2f} .. {max(bpm):.2f}")
        print(f"jitter    {sum(r[1] for r in rows) / len(rows):.2f} ms RMS average, largest {max(r[2] for r in rows):.2f} ms")
    if first_cpu and last_cpu:
        print(f"late      {last_cpu[3] - first_cpu[3]} blocks in these {secs:g} s")


if __name__ == "__main__":
    main()
