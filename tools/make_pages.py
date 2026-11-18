#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the Niagara Drift site for GitHub Pages (decided 2026-10-11: the emulator page is the front page):

  index.html, worklet.js, omni.wasm, fonts/   the emulator page (build/emu, from web/emu/build.sh), with its
                                row of actions (Install, The manual, Source; the download in the note) put in at <!--ACTIONS-->
  manual/                       the manual's pages (tools/make_manual.py; their sounds from tools/make_manual.py
                                --renders, run first: the release says so if one is missing)
  ja/                           the front page, the install page (ja/install/) and Help (ja/manual/help/) in Japanese
                                (tools/make_ja.py, the installer's TEXT.ja, docs/manual/ja)
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
import subprocess
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "web"))
sys.path.insert(0, str(SRC / "tools"))
from make_site import product_of, strip_module  # noqa: E402  (Felucca's: the package format)

REPO = "https://github.com/evanyerburgh/niagara-drift"

ACTIONS = """<nav class="actions" aria-label="Get Niagara Drift">
  <a class="main" href="install/"><b>Install</b><span>on your FM-1, from Chrome or Edge</span></a>
  <a href="manual/"><b>The manual</b><span>start here, recipes, reference</span></a>
  <a href="__REPO__"><b>Source</b><span>on GitHub, GPL-3.0</span></a>
</nav>
<p class="actions-note">Version __VERSION__. Or <a href="firmware/__PKG__" download>download __PKG__</a> for the terminal installer. Installing is at your own risk; the installer can also put M-VAVE's own firmware back.</p>"""

ACTIONS_JA = """<nav class="actions" aria-label="Niagara Driftを入手">
  <a class="main" href="ja/install/"><b>インストール</b><span>FM-1に、ChromeまたはEdgeから</span></a>
  <a href="ja/manual/help/"><b>ヘルプ</b><span>インストール、元に戻す、フィードバック</span></a>
  <a href="__REPO__"><b>ソース</b><span>GitHubで、GPL-3.0</span></a>
</nav>
<p class="actions-note">バージョン__VERSION__。ターミナル版インストーラー用に<a href="firmware/__PKG__" download>__PKG__をダウンロード</a>することもできます。インストールは自己責任です。インストーラーでM-VAVE純正のファームウェアに戻すこともできます。</p>"""
# the install page's words outside its TEXT (English, Japanese)
JA_INSTALL = [('<html lang="en">', '<html lang="ja">'),
              ("<title>Install Niagara Drift</title>", "<title>Niagara Driftのインストール</title>"),
              ('content="Install Niagara Drift on an M-VAVE FM-1 from Chrome or Edge, over USB; or put M-VAVE\'s firmware back."',
               'content="ChromeまたはEdgeから、USBでM-VAVE FM-1にNiagara Driftをインストール。M-VAVEのファームウェアに戻すこともできます。"'),
              ('<a class="skip" href="#main">Skip to content</a>', '<a class="skip" href="#main">本文へ移動</a>'),
              ('const lang = "en";', 'const lang = "ja";'),
              ("""<p class="foot">Niagara Drift is free, open-source firmware (GPL-3.0, no warranty), not made or endorsed by M-VAVE. This installer
  is Felucca's, by Leo Kuroshita (<a href="https://hugelton.com" target="_blank" rel="noopener">Hügelton Instruments</a>), by way of
  X0X and FoMni by Charles Vestal. M-VAVE and FM-1 are trademarks of their owners.
  <a href="https://github.com/evanyerburgh/niagara-drift">Source</a></p>""",
               """<p class="foot">Niagara Driftは無料・オープンソースのファームウェア（GPL-3.0、無保証）で、M-VAVEによる製品や公認のものではありません。
  このインストーラーはLeo Kuroshita（<a href="https://hugelton.com" target="_blank" rel="noopener">Hügelton Instruments</a>）によるFeluccaのもので、
  Charles VestalのX0XとFoMniを経ています。M-VAVEとFM-1は各所有者の商標です。
  {NOTE} <a href="https://github.com/evanyerburgh/niagara-drift">ソース</a></p>""")]


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
    from make_ja import front_ja  # noqa: E402  (the Japanese front page, from the English one)
    front_j = front_ja(front)
    for mark in ("<!--ACTIONS-->", "<!--NAV-->"):
        if front.count(mark) != 1:
            raise SystemExit(f"web/emu/index.html must contain {mark} once")
    acts = ACTIONS.replace("__PKG__", name).replace("__REPO__", REPO).replace("__VERSION__", html.escape(label))
    news = f'<a class="ver" href="manual/changes/" aria-label="{html.escape(label)}: what’s new">{html.escape(label)}<span class="wn"> · What’s new</span></a>'   # CHANGELOG.md, as the manual's page
    news_j = f'<a class="ver" href="ja/manual/changes/" aria-label="{html.escape(label)}：変更履歴">{html.escape(label)}<span class="wn"> · 変更履歴</span></a>'   # (the page's base is ../)
    front = front.replace("<!--NAV-->", news)     # beside Install and Source, top right
    (out / "index.html").write_text(front.replace("<!--ACTIONS-->", acts), encoding="utf-8")
    acts_j = ACTIONS_JA.replace("__PKG__", name).replace("__REPO__", REPO).replace("__VERSION__", html.escape(label))
    (out / "ja").mkdir(exist_ok=True)
    (out / "ja" / "index.html").write_text(front_j.replace("<!--NAV-->", news_j).replace("<!--ACTIONS-->", acts_j, 1), encoding="utf-8")
    subprocess.run([sys.executable, str(SRC / "tools" / "make_manual.py"), str(out)], check=True)   # out/manual/
    for d in ("install", "firmware"):
        (out / d).mkdir(parents=True)
    shutil.copy(pkg, out / "firmware" / name)
    page = (SRC / "web" / "installer.html").read_text(encoding="utf-8")
    lib = strip_module((SRC / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((SRC / "web" / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": label, "product": product, "pkg": "../firmware/" + name})
    sys.path.insert(0, str(SRC / "tools"))
    from make_manual import JA_NOTE, banner_html  # noqa: E402  (the site's banner and menu, as on the manual's pages)
    top = banner_html("../", "Install", f"Niagara Drift {label} on your FM-1, from Chrome or Edge, over USB", current="", other="ja/install/")
    for mark in ("/*LIB*/", "/*META*/", "<!--TOP-->"):
        if page.count(mark) != 1:
            raise SystemExit(f"installer.html must contain {mark} once")
    page = page.replace("<!--TOP-->", top).replace("/*LIB*/", lib).replace("/*META*/", meta)
    (out / "install" / "index.html").write_text(page, encoding="utf-8")
    # the install page in Japanese (ja/install/): the same page a folder deeper, its words from its TEXT.ja
    ja = (SRC / "web" / "installer.html").read_text(encoding="utf-8").replace('"../', '"../../')
    ja = ja.replace('"../../manual/help/', '"../manual/help/')                 # Help is in Japanese
    meta_j = json.dumps({"version": label, "product": product, "pkg": "../../firmware/" + name})
    top_j = banner_html("../../", "インストール", f"Niagara Drift {label}をFM-1に。ChromeまたはEdgeから、USBで", current="",
                        lang="ja", other="install/")
    for en_, ja_ in JA_INSTALL:
        if ja.count(en_) != 1:
            raise SystemExit(f"installer.html must contain once: {en_[:80]}")
        ja = ja.replace(en_, ja_)
    ja = (ja.replace("<!--TOP-->", top_j).replace("{NOTE}", JA_NOTE.format(up="../../"), 1)
          .replace("/*LIB*/", lib).replace("/*META*/", meta_j))
    (out / "ja" / "install").mkdir(parents=True)
    (out / "ja" / "install" / "index.html").write_text(ja, encoding="utf-8")
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html (the emulator), install/ ({product}), firmware/{name} ({len(raw)} B)")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:4])
