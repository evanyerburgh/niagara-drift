#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Same samples: does a change leave the sound and the screen as they were?

    tools/same_samples.py [BASE]          this tree against BASE (default: main, built in build/base)
    tools/same_samples.py BASE HEAD       two trees (CI: the PR's base and the PR)

Builds the host simulator and the engine's renderer in both trees, plays the same scripts through both (HEAD's
tests/golden/*.omni and tests/scenarios/*.omni, and host/drone_render's scenes), and compares everything they write:
every WAV sample by sample, every screenshot pixel by pixel. The host is deterministic, so a change that keeps the
sound gives the same samples. Each file is:
    same        identical
    close       sound within -90 dBFS of the base everywhere (float rounding: a reordered sum), screens identical
    different   anything more: listen to it
Exit status 1 when anything is different (or missing), unless --allow-change (a PR that means to change the sound).
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CLOSE_DB = -90.0


def sh(cmd, cwd, env=None):
    subprocess.run(cmd, cwd=cwd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, env=env, shell=True)


def build(tree):
    """omni_host and drone_render for a tree; returns their paths."""
    env = dict(os.environ)
    cc = env.get("CC", "cc")
    sh("sh host/build_host.sh", tree, env)
    out = Path(tree) / "build" / "host" / "drone_render_same"
    sh(f"{cc} -O2 -ffp-contract=off -std=c99 -DOM_HOST -Ifirmware/src/dsp -o {out} host/drone_render.c "
       f"firmware/src/dsp/drone.c -lm", tree, env)
    return Path(tree) / "build" / "host" / "omni_host", out


def run_all(tree, scripts, out):
    host, render = build(tree)
    for s in scripts:
        d = out / s.parent.name / s.stem
        d.mkdir(parents=True, exist_ok=True)
        subprocess.run([str(host), str(s), str(d)], cwd=tree, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    d = out / "drone_render"
    d.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(render), str(d)], cwd=tree, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def wav_samples(p):
    with wave.open(str(p)) as w:
        n, sw, ch = w.getnframes(), w.getsampwidth(), w.getnchannels()
        b = w.readframes(n)
    if sw == 2:
        return list(struct.unpack(f"<{len(b) // 2}h", b)), 32768.0, ch
    if sw == 3:
        v = [int.from_bytes(b[i:i + 3], "little", signed=True) for i in range(0, len(b), 3)]
        return v, 8388608.0, ch
    return list(struct.unpack(f"<{len(b) // 4}i", b)), 2147483648.0, ch


def db(x):
    import math
    return 20.0 * math.log10(x) if x > 0 else -999.0


def compare_wav(a, b):
    if a.read_bytes() == b.read_bytes():
        return "same", ""
    sa, fa, ca = wav_samples(a)
    sb, fb, cb = wav_samples(b)
    if (fa, ca) != (fb, cb) or len(sa) != len(sb):
        return "different", f"length or format changed ({len(sa) // ca} vs {len(sb) // cb} frames)"
    peak = max(abs(x - y) for x, y in zip(sa, sb)) / fa
    first = next(i for i, (x, y) in enumerate(zip(sa, sb)) if x != y) // ca
    verdict = "close" if db(peak) < CLOSE_DB else "different"
    return verdict, f"largest difference {db(peak):.0f} dBFS, from {first / 44100:.2f} s"


def compare_png(a, b):
    if a.read_bytes() == b.read_bytes():
        return "same", ""
    try:
        from PIL import Image, ImageChops
        ia, ib = Image.open(a).convert("RGB"), Image.open(b).convert("RGB")
        if ia.size != ib.size:
            return "different", "size changed"
        box = ImageChops.difference(ia, ib).getbbox()
        if not box:
            return "same", ""
        n = sum(1 for p in ImageChops.difference(ia, ib).getdata() if p != (0, 0, 0))
        return "different", f"{n} pixels in {box}"
    except ImportError:
        return "different", "bytes differ (no Pillow to look closer)"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trees", nargs="*")
    ap.add_argument("--allow-change", action="store_true")
    a = ap.parse_args()
    if len(a.trees) == 2:
        base, head = map(Path, a.trees)
    else:
        head = ROOT
        base = Path(a.trees[0]) if a.trees else ROOT / "build" / "base"
        if not a.trees:                                  # main, in a worktree (shared with tools/budget.py)
            if not base.exists():
                subprocess.run(["git", "worktree", "add", "--detach", str(base), "main"], cwd=ROOT, check=True,
                               stdout=subprocess.DEVNULL)
            else:
                subprocess.run(["git", "checkout", "-q", "--detach", "main"], cwd=base, check=True)
    scripts = sorted((head / "tests" / "golden").glob("*.omni")) + sorted((head / "tests" / "scenarios").glob("*.omni"))
    tmp = Path(tempfile.mkdtemp(prefix="same-"))
    run_all(base, scripts, tmp / "base")
    run_all(head, scripts, tmp / "head")
    rows, counts = [], {"same": 0, "close": 0, "different": 0}
    names = sorted({p.relative_to(tmp / "base") for p in (tmp / "base").rglob("*") if p.suffix in (".wav", ".png")} |
                   {p.relative_to(tmp / "head") for p in (tmp / "head").rglob("*") if p.suffix in (".wav", ".png")})
    for n in names:
        pa, pb = tmp / "base" / n, tmp / "head" / n
        if not pa.exists() or not pb.exists():
            v, why = "different", "only in " + ("this branch" if pb.exists() else "the base")
        else:
            v, why = (compare_wav if n.suffix == ".wav" else compare_png)(pa, pb)
        counts[v] += 1
        if v != "same":
            rows.append((v, str(n), why))
    out = [f"### Same samples: {counts['same']} same, {counts['close']} close, {counts['different']} different "
           f"(of {len(names)} sounds and screens)", ""]
    if rows:
        out += ["| | File | |", "|---|---|---|"] + [f"| {v} | {n} | {w} |" for v, n, w in rows]
    else:
        out.append("Every sound and screen is identical to the base.")
    text = "\n".join(out) + "\n"
    print(text)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write(text)
    keep = ROOT / "build" / "same-samples"
    if counts["different"] or counts["close"]:                # what differs, kept to listen to (locally)
        shutil.rmtree(keep, ignore_errors=True)
        shutil.copytree(tmp, keep)
        print(f"(both versions kept in {keep.relative_to(ROOT) if keep.is_relative_to(ROOT) else keep})")
    shutil.rmtree(tmp, ignore_errors=True)
    if counts["different"] and not a.allow_change:
        for v, n, _ in rows:
            if v == "different":
                print(f"::error title=Sound or screen changed::{n}")
        sys.exit(1)


if __name__ == "__main__":
    main()
