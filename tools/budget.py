#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A PR's budget line: what this branch costs the FM-1, next to main's.

    tools/budget.py            after ./build.sh: prints the line (Markdown) for the PR body

RAM, pool headroom and image size come from the firmware build (build/budget.json, written by
tools/build.py); the engine's cost from tests/host/drone_cost.c (host instructions a sample, macOS).
main's numbers come from a build of main in a worktree, build/base (built once per main commit).
Run with the exports (CLAUDE.md / BUILDING.md): it builds C and runs the firmware build.
"""
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE = ROOT / "build" / "base"


def run(*cmd, cwd=ROOT):
    return subprocess.run(cmd, cwd=cwd, check=True, capture_output=True, text=True).stdout


def cost(src):
    """{case: instr/sample} from src's drone_cost (a checkout's root)."""
    exe = src / "build" / "host" / "drone_cost_budget"
    exe.parent.mkdir(parents=True, exist_ok=True)
    cc = os.environ.get("CC", "cc").split()
    run(*cc, "-O2", "-ffp-contract=off", "-std=c99", "-D_DARWIN_C_SOURCE", "-DOM_HOST",
        "-Ifirmware/src/dsp", "-o", str(exe), "tests/host/drone_cost.c", "firmware/src/dsp/drone.c", cwd=src)
    out = subprocess.run([str(exe)], cwd=src, capture_output=True, text=True).stdout
    return {m.group(1): int(m.group(2)) for m in re.finditer(r"^  (.+?)\s+(\d+) instr/sample", out, re.M)}


def base_build():
    rev = run("git", "rev-parse", "main").strip()
    if not BASE.exists():
        run("git", "worktree", "add", "--detach", str(BASE), "main")
    else:
        run("git", "checkout", "-q", "--detach", rev, cwd=BASE)
    stamp, budget = BASE / "build" / "budget.rev", BASE / "build" / "budget.json"
    if not (budget.exists() and stamp.exists() and stamp.read_text().strip() == rev):
        print(f"budget: building main ({rev[:7]}) in {BASE.relative_to(ROOT)} ...", file=sys.stderr)
        out = subprocess.run(["sh", "build.sh"], cwd=BASE, check=True, capture_output=True,
                             text=True).stdout
        m = re.search(r"image (\d+) B; RAM \.data\+\.bss (\d+) B of (\d+); pool (\d+) B of (\d+)", out)
        if not m:                                 # read from the build's own line: main may predate budget.json
            sys.exit("budget: main's build printed no sizes")
        budget.write_text(json.dumps(dict(zip(("image", "ram", "ram_max", "pool", "pool_max"),
                                              map(int, m.groups())))) + "\n")
        stamp.write_text(rev + "\n")
    return rev, json.loads(budget.read_text())


def kb(b):
    return f"{b / 1024:.1f} KB"


def delta(new, old, fmt):
    d = new - old
    return "same" if d == 0 else ("+" if d > 0 else "−") + fmt(abs(d))


def main():
    head_f = ROOT / "build" / "budget.json"
    if not head_f.exists():
        sys.exit("budget: no build/budget.json: run ./build.sh first")
    head = json.loads(head_f.read_text())
    rev, base = base_build()
    hc, bc = cost(ROOT), cost(BASE)
    worst = max(hc, key=hc.get)
    pct = lambda i: f"{i * 0.017:.1f} %"                         # drone_cost's device estimate
    print(f"**Budget** (against main {rev[:7]}): "
          f"RAM {kb(head['ram'])} of {kb(head['ram_max'])} ({delta(head['ram'], base['ram'], kb)}) · "
          f"pool headroom {kb(head['pool_max'] - head['pool'])} ({delta(base['pool'], head['pool'], kb)}) · "
          f"image {kb(head['image'])} ({delta(head['image'], base['image'], kb)}) · "
          f"engine worst case {hc[worst]:,} instr/sample, ~{pct(hc[worst])} of the FM-1 "
          f"({(hc[worst] - bc.get(worst, hc[worst])) / bc.get(worst, hc[worst]) * 100:+.1f} %)")
    print("\n<details><summary>Engine cost by case</summary>\n")
    print("| Case | main | this PR | change |\n|---|---:|---:|---:|")
    for name, v in hc.items():
        b = bc.get(name)
        print(f"| {name} | {b if b is not None else '–'} | {v} | "
              f"{f'{(v - b) / b * 100:+.1f} %' if b else 'new'} |")
    print("\n</details>")


if __name__ == "__main__":
    main()
