#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the Niagara Drift site for GitHub Pages (decided 2026-10-11: the emulator page is the front page):

  index.html, worklet.js, omni.wasm, fonts/   the emulator page (build/emu, from web/emu/build.sh), with its
                                row of actions (Install, Download, Full manual, Source) put in at <!--ACTIONS-->
  install/index.html            the web installer (web/installer.html, with fm1pkg.js, fm1ota.js and the
                                package's metadata inlined; Chrome or Edge, Web MIDI)
  firmware/niagara-drift-VERSION.fwsc   the package the installer writes; also the download

  tools/make_pages.py build/niagara-drift-0.1-beta.fwsc 0.1-beta OUT_DIR

The package must be a release build (./build.sh --release X.Y): its identity, FM-1_8XXYYZZ, is what
the installer checks the download against and what the FM-1 reports after the install."""
import html
import json
import re
import shutil
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "web"))
from make_site import product_of, strip_module  # noqa: E402  (Felucca's: the package format)

REPO = "https://github.com/evanyerburgh/niagara-drift"

ACTIONS = """<nav class="actions" aria-label="Get Niagara Drift">
  <a href="install/"><b>Install</b><span>on your FM-1, from Chrome or Edge</span></a>
  <a href="firmware/__PKG__" download><b>Download</b><span>__PKG__</span></a>
  <a href="__REPO__#readme"><b>Full manual</b><span>every page, knob and key</span></a>
  <a href="__REPO__"><b>Source</b><span>on GitHub, GPL-3.0</span></a>
</nav>
<p class="actions-note">Version __VERSION__, tested on one FM-1 so far. Installing is at your own risk; the installer can also put
  M-VAVE's own firmware back.</p>"""


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_8\d{6}", product):
        raise SystemExit(f"{pkg}: identity {product!r}: make a release build (./build.sh --release X.Y)")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it")
    name = f"niagara-drift-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    label = version.replace("-", " ")
    emu = SRC / "build" / "emu"
    if not (emu / "omni.wasm").exists():
        raise SystemExit("no build/emu/omni.wasm: run web/emu/build.sh")
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(emu, out)                    # the front page: the emulator page and its files
    front = (out / "index.html").read_text(encoding="utf-8")
    if front.count("<!--ACTIONS-->") != 1:
        raise SystemExit("web/emu/index.html must contain <!--ACTIONS--> once")
    acts = ACTIONS.replace("__PKG__", name).replace("__REPO__", REPO).replace("__VERSION__", html.escape(label))
    (out / "index.html").write_text(front.replace("<!--ACTIONS-->", acts), encoding="utf-8")
    for d in ("install", "firmware"):
        (out / d).mkdir(parents=True)
    shutil.copy(pkg, out / "firmware" / name)
    page = (SRC / "web" / "installer.html").read_text(encoding="utf-8")
    lib = strip_module((SRC / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((SRC / "web" / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": label, "product": product, "pkg": "../firmware/" + name})
    for mark in ("/*LIB*/", "/*META*/"):
        if page.count(mark) != 1:
            raise SystemExit(f"installer.html must contain {mark} once")
    (out / "install" / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html (the emulator), install/ ({product}), firmware/{name} ({len(raw)} B)")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:4])
