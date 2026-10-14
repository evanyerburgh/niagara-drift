#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""How hard the FM-1's audio works, over USB-MIDI (Niagara Drift's firmware, read-only).

  fm1_cpu.py [SECONDS]      poll for SECONDS (default 10), then print the load and the longest render

The load is the share of each audio block's time spent rendering it (smoothed over ~16 blocks);
the longest render is since the FM-1 started. Late blocks are dropouts. Needs mido with python-rtmidi.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fm1_install import PORT_RE, MidoBackend, pack7, pair_output, unpack7  # noqa: E402

QUERY = b"\xF0" + pack7(bytes([0x00, 0x59, 0x7E, 0, 0, 0, 0xFF])) + b"\xF7"


def parse(pkt):
    d = unpack7(pkt[1:-1])
    if len(d) != 39 or d[:3] != b"\x00\x59\x7E" or ~sum(d[6:38]) & 0xFF != d[38]:
        return None
    w = [int.from_bytes(d[6 + 4 * i:10 + 4 * i], "little") for i in range(8)]
    return dict(zip(["cpu_q8", "max_us", "budget_us", "late", "halves", "nested", "boots", "last_us"], w))


def main():
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
    be = MidoBackend()
    ins = [n for n in be.input_names() if PORT_RE.search(n)]
    if not ins:
        sys.exit("fm1_cpu: no FM-1 MIDI port")
    out = pair_output(ins[0], [n for n in be.output_names() if PORT_RE.search(n)])
    link = be.open(ins[0], out)
    loads, first, last = [], None, None
    try:
        end = time.monotonic() + secs
        while time.monotonic() < end:
            link.drain()
            link.send(QUERY)
            t = time.monotonic() + 1.0
            while (left := t - time.monotonic()) > 0:
                p = link.read(left)
                r = p and parse(p)
                if r:
                    first = first or r
                    last = r
                    loads.append(r["cpu_q8"] * 100 / 256)
                    break
            time.sleep(0.25)
    finally:
        link.close()
    if not last:
        sys.exit("fm1_cpu: no reply (not Niagara Drift, or an older build)")
    b = last["budget_us"]
    print(f"load      {sum(loads) / len(loads):5.1f} % average, {min(loads):.1f} .. {max(loads):.1f} % "
          f"({len(loads)} readings over {secs:g} s)")
    print(f"longest   {last['max_us']} us of {b} us a block ({100 * last['max_us'] / b:.1f} %, since start)")
    print(f"late      {last['late'] - first['late']} blocks in these {secs:g} s ({last['late']} since start)")
    print(f"blocks    {last['halves'] - first['halves']} rendered; boots {last['boots']}")


if __name__ == "__main__":
    main()
