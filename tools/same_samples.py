#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Same samples: does a change leave the sound and the screen as they were?

    tools/same_samples.py [BASE]          this tree against BASE (default: main, built in build/base)
    tools/same_samples.py BASE HEAD       two trees (CI: the PR's base and the PR)

Builds the host simulator and the engine's renderer in both trees, plays the scripts (tests/golden/*.omni,
tests/scenarios/*.omni) and host/drone_render's scenes through both, and compares everything they write: every WAV
sample by sample, every screenshot pixel by pixel. The host is deterministic, so a change that keeps the sound gives
the same samples. Each file is:
    same        identical
    close       every 256-sample block differs by at most the format's last bit (rounding), or lies 60 dB under that
                block's own level; screens identical
    different   anything more: listen to it
A script that differs between the trees (renumbered, extended) isn't compared: its files are listed as "script changed".
A script only in HEAD (a new render) is played by both. Exit status 1 when anything is different, when a run fails,
or when nothing was compared, unless --allow-change (a PR that means to change the sound). Needs numpy (and Pillow
for a closer look at screens).
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
BLOCK = 256
REL_DB = -60.0


def sh(cmd, cwd):
    r = subprocess.run(cmd, cwd=cwd, shell=True, capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"same_samples: `{cmd}` failed in {cwd}:\n{r.stdout[-3000:]}{r.stderr[-3000:]}")


def build(tree):
    """omni_host and drone_render for a tree; returns their paths."""
    cc = os.environ.get("CC", "cc")
    sh("sh host/build_host.sh", tree)
    out = tree / "build" / "host" / "drone_render_same"
    sh(f"{cc} -O2 -ffp-contract=off -std=c99 -DOM_HOST -Ifirmware/src/dsp -o {out} host/drone_render.c "
       f"firmware/src/dsp/drone.c -lm", tree)
    return tree / "build" / "host" / "omni_host", out


def run_all(tree, scripts, out, problems, new=frozenset()):
    """Each script through this tree's host. A golden script must run clean; a scenario may fail its expectations
    (the base doesn't know a PR's new behaviour), but must not crash or stop early; a script new in this branch
    (new: their names) may use a host command the base hasn't got yet: there, it stops, and only its files differ."""
    host, render = build(tree)
    for s in scripts:
        d = out / s.parent.name / s.stem
        d.mkdir(parents=True, exist_ok=True)
        r = subprocess.run([str(host), str(s), str(d)], cwd=tree, capture_output=True, text=True)
        crashed = r.returncode < 0 or ("unknown command" in r.stdout + r.stderr and s.name not in new)
        if crashed or (r.returncode and s.parent.name == "golden"):
            tail = (r.stdout + r.stderr).strip().splitlines()[-5:]
            problems.append(f"{tree.name}: {s.parent.name}/{s.name} exited {r.returncode}: " + " / ".join(tail))
    d = out / "drone_render"
    d.mkdir(parents=True, exist_ok=True)
    r = subprocess.run([str(render), str(d)], cwd=tree, capture_output=True, text=True)
    if r.returncode:
        problems.append(f"{tree.name}: drone_render exited {r.returncode}: {r.stderr.strip()[-300:]}")


def wav_samples(p):
    with wave.open(str(p)) as w:
        n, sw, ch = w.getnframes(), w.getsampwidth(), w.getnchannels()
        b = w.readframes(n)
    if sw == 3:
        u = np.frombuffer(b, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        x = (u[:, 0] | u[:, 1] << 8 | u[:, 2] << 16) << 8 >> 8
    else:
        x = np.frombuffer(b, dtype={2: np.int16, 4: np.int32}[sw]).astype(np.int64)
    return x.reshape(-1, ch).astype(np.float64), float(2 ** (8 * sw - 1))


def compare_wav(a, b):
    if a.read_bytes() == b.read_bytes():
        return "same", ""
    xa, fa = wav_samples(a)
    xb, fb = wav_samples(b)
    if fa != fb or xa.shape != xb.shape:
        return "different", f"length or format changed ({len(xa)} vs {len(xb)} frames)"
    d = np.abs(xa - xb).max(axis=1)
    lvl = np.abs(xa).max(axis=1)
    n = len(d) // BLOCK * BLOCK
    if n:                                             # per block: the largest difference against the block's own peak
        db_, lb = d[:n].reshape(-1, BLOCK).max(axis=1), lvl[:n].reshape(-1, BLOCK).max(axis=1)
    else:
        db_, lb = d.max(keepdims=True), lvl.max(keepdims=True)
    ok = (db_ <= 1.0) | (db_ <= lb * 10 ** (REL_DB / 20))
    peak = 20 * np.log10(max(d.max(), 1e-9) / fa)
    first = int(np.argmax(d > 0)) / 44100
    if ok.all():
        how = "the last bit only (rounding)" if (db_ <= 1.0).all() else "60 dB under the sound in every block"
        return "close", f"{how}: largest difference {peak:.0f} dBFS"
    bad = int(np.argmax(~ok))
    return "different", (f"largest difference {peak:.0f} dBFS, from {first:.2f} s; first block over the limit at "
                         f"{bad * BLOCK / 44100:.2f} s")


def compare_png(a, b):
    if a.read_bytes() == b.read_bytes():
        return "same", ""
    try:
        from PIL import Image, ImageChops
    except ImportError:
        return "different", "bytes differ (no Pillow to look closer)"
    ia, ib = Image.open(a).convert("RGB"), Image.open(b).convert("RGB")
    if ia.size != ib.size:
        return "different", "size changed"
    diff = ImageChops.difference(ia, ib)
    box = diff.getbbox()
    if not box:
        return "same", ""
    n = int(np.count_nonzero(np.asarray(diff).any(axis=2)))
    return "different", f"{n} pixels in {box}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trees", nargs="*")
    ap.add_argument("--allow-change", action="store_true")
    a = ap.parse_args()
    if len(a.trees) == 2:
        base, head = (Path(t).resolve() for t in a.trees)
    else:
        head = ROOT
        base = Path(a.trees[0]).resolve() if a.trees else ROOT / "build" / "base"
        if not a.trees:                                  # main, in a worktree (shared with tools/budget.py)
            if not base.exists():
                subprocess.run(["git", "worktree", "add", "--detach", str(base), "main"], cwd=ROOT, check=True,
                               stdout=subprocess.DEVNULL)
            else:
                subprocess.run(["git", "checkout", "-q", "--detach", "main"], cwd=base, check=True)
    rel = sorted(p.relative_to(head) for d in ("golden", "scenarios") for p in (head / "tests" / d).glob("*.omni"))
    changed = {r for r in rel if (base / r).exists() and (base / r).read_bytes() != (head / r).read_bytes()}
    scripts = [head / r for r in rel if r not in changed]   # the same script (or a new one) played by both
    tmp = Path(tempfile.mkdtemp(prefix="same-"))
    problems = []
    run_all(base, scripts, tmp / "base", problems, {r.name for r in rel if not (base / r).exists()})
    run_all(head, scripts, tmp / "head", problems)
    rows, counts = [], {"same": 0, "close": 0, "different": 0}
    files = lambda side: {p.relative_to(tmp / side) for p in (tmp / side).rglob("*") if p.suffix in (".wav", ".png")}
    names = sorted(files("base") | files("head"))
    for n in names:
        pa, pb = tmp / "base" / n, tmp / "head" / n
        if not pa.exists() or not pb.exists():
            v, why = "different", "only in " + ("this branch" if pb.exists() else "the base")
        else:
            v, why = (compare_wav if n.suffix == ".wav" else compare_png)(pa, pb)
        counts[v] += 1
        if v != "same":
            rows.append((v, str(n), why))
    golden = [r for r in rel if r.parts[1] == "golden" and r not in changed]
    for r in golden:                                      # every golden script must have produced its files
        if not any(n.parts[0] == "golden" and n.parts[1] == r.stem for n in names):
            problems.append(f"{r} produced nothing")
    if not names:
        problems.append("nothing was compared")
    out = [f"### Same samples: {counts['same']} same, {counts['close']} close, {counts['different']} different "
           f"(of {len(names)} sounds and screens, from {len(scripts)} scripts and the engine's scenes)", ""]
    if changed:
        out.append("Not compared, the script changed in this branch: " + ", ".join(f"`{r}`" for r in sorted(changed)))
        out.append("")
    if problems:
        out += ["**Runs that went wrong:**", ""] + [f"- {p}" for p in problems] + [""]
    if rows:
        out += ["| | File | |", "|---|---|---|"] + [f"| {v} | {n} | {w} |" for v, n, w in rows]
    elif not problems:
        out.append("Every sound and screen is identical to the base.")
    text = "\n".join(out) + "\n"
    print(text)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write(text)
    keep = ROOT / "build" / "same-samples"
    if counts["different"] or counts["close"]:                # both versions, kept to listen to (locally)
        shutil.rmtree(keep, ignore_errors=True)
        shutil.copytree(tmp, keep)
        print(f"(both versions kept in {keep})")
    shutil.rmtree(tmp, ignore_errors=True)
    if problems:
        for p in problems:
            print(f"::error title=Same samples: a run went wrong::{p}")
        sys.exit(1)
    if counts["different"] and not a.allow_change:
        for v, n, _ in rows:
            if v == "different":
                print(f"::error title=Sound or screen changed::{n}")
        sys.exit(1)


if __name__ == "__main__":
    main()
