#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The engine's cost, one checkout against another, counted by valgrind (for CI on Linux).

    tools/ci_cost.py BASE HEAD     writes a Markdown table to $GITHUB_STEP_SUMMARY (or stdout)

Each case of tests/host/drone_cost.c runs under callgrind, counting only cost() (the 2.5 s it renders);
a case more than WARN % dearer than in BASE gets a warning on the PR. Linux instructions aren't the Mac's
arm64 ones, so the change is the number to read, not the count.
"""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SAMPLES = int(2.5 * 44100 / 256) * 256        # cost(): 2.5 s in blocks of 256
WARN = 2.0                                    # % dearer than main: a warning


def build(src, out):
    subprocess.run(["cc", "-O2", "-g", "-ffp-contract=off", "-std=c99", "-D_DEFAULT_SOURCE", "-DOM_HOST",
                    "-Ifirmware/src/dsp", "-o", str(out), "tests/host/drone_cost.c", "firmware/src/dsp/drone.c",
                    "-lm"], cwd=src, check=True)


def count(exe, *args):
    """Instructions inside cost() (case N), or of the whole run (no case: an older main's whole table)."""
    tog = ["--toggle-collect=cost*"] if args else []
    with tempfile.NamedTemporaryFile(suffix=".cg") as f:
        subprocess.run(["valgrind", "--tool=callgrind", *tog, f"--callgrind-out-file={f.name}",
                        str(exe), *args], check=True, capture_output=True)
        m = re.search(rb"^(?:summary|totals): (\d+)", Path(f.name).read_bytes(), re.M)
        return int(m.group(1))


def cases(src):
    """The case names, or None when that checkout's drone_cost can't run one case alone (older main)."""
    s = (Path(src) / "tests/host/drone_cost.c").read_text()
    if "drone_cost N" not in s:
        return None
    body = s[s.index("names[] = {"):s.index("};", s.index("names[] = {"))]
    return re.findall(r'"([^"]+)"', body)


def main():
    base, head = map(Path, sys.argv[1:3])
    tmp = Path(tempfile.mkdtemp())
    build(base, tmp / "base")
    build(head, tmp / "head")
    bn, hn = cases(base), cases(head)
    rows, warn = [], []
    if bn and hn:
        for i, name in enumerate(hn):
            h = count(tmp / "head", str(i)) / SAMPLES
            b = count(tmp / "base", str(bn.index(name))) / SAMPLES if name in bn else None
            rows.append((name, b, h))
    else:                                     # main predates one case at a time: the whole table's total
        n = 8 * SAMPLES                       # the whole run, per sample of its 8 cases, setup included
        rows.append(("all cases together (main can't run them one by one yet)",
                     count(tmp / "base") / n, count(tmp / "head") / n))
    out = ["### The engine's cost against main (valgrind, Linux instructions a sample)", "",
           "| Case | main | this PR | change |", "|---|---:|---:|---:|"]
    for name, b, h in rows:
        ch = (h - b) / b * 100 if b else None
        out.append(f"| {name} | {f'{b:,.0f}' if b else '–'} | {h:,.0f} | "
                   f"{f'{ch:+.1f} %' if ch is not None else 'new'} |")
        if ch is not None and ch > WARN:
            warn.append(f"{name}: {ch:+.1f} %")
    out.append("")
    text = "\n".join(out) + "\n"
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write(text)
    print(text)
    for w in warn:                            # an annotation on the PR's checks, not a failure
        print(f"::warning title=Engine cost up::{w} (more than {WARN:g} % over main)")


if __name__ == "__main__":
    main()
