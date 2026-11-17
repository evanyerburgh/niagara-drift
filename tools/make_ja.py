#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The front page in Japanese (2026-11-17, the first part: the front page, the install page and Help), made from the
English one (web/emu/index.html), so the two never drift apart in anything but their words.

  tools/make_ja.py SITE_DIR      SITE_DIR/ja/index.html from SITE_DIR/index.html (web/emu/build.sh, tools/make_pages.py)

Each English text below must be in the page exactly as many times as said, or the build stops: an English change
that isn't carried over here is caught, not left half translated. The page gets <base href="../">, so it loads the
emulator's files (worklet.js, omni.wasm, fonts/) from the site's root; its own links are written from there (ja/...).
Terms: docs/translations/ja-glossary.md."""
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "tools"))
from make_manual import JA_NOTE, menu_html  # noqa: E402

SPEC_EN = """Two drones that breathe and drift on their own. Play them with knobs, gestures and scenes, from a calm, glowing hum
    to harsh, beating, self-oscillating sound."""
SPEC_JA = "ひとりでに呼吸し、漂う2つのドローン。ノブ、ジェスチャー、シーンで弾き、穏やかに光るハムから、荒々しくうなり、自己発振する音まで。"
DESC_EN = ("Drone synthesizer firmware for the M-VAVE FM-1: two drones that breathe and drift on their own, played with "
           "knobs, gestures and scenes, from a calm, glowing hum to harsh, beating, self-oscillating sound.")
DESC_JA = ("M-VAVE FM-1用のドローン・シンセ・ファームウェア。ひとりでに呼吸し、漂う2つのドローンを、ノブ、ジェスチャー、"
           "シーンで弾きます。穏やかに光るハムから、荒々しくうなり、自己発振する音まで。")

# (English, Japanese, how many times the English is in the page)
PAIRS = [
    ('<html lang="en">', '<html lang="ja">', 1),
    ("<title>Niagara Drift</title>", "<title>Niagara Drift（日本語）</title>", 1),
    (DESC_EN, DESC_JA, 2),
    ('content="https://evanyerburgh.github.io/niagara-drift/">', 'content="https://evanyerburgh.github.io/niagara-drift/ja/">', 1),
    ('<p class="tag">Drone synthesizer firmware for the M-VAVE FM-1</p>', '<p class="tag">M-VAVE FM-1用のドローン・シンセ・ファームウェア</p>', 2),
    (SPEC_EN, SPEC_JA, 2),
    ('<span class="here">Play it here ↓</span>', '<span class="here">ここで弾く ↓</span>', 2),
    ('aria-label="The FM-1: knobs, buttons, screen and keys"', 'aria-label="FM-1：ノブ、ボタン、画面、鍵盤"', 1),
    ('aria-label="The FM-1 screen"', 'aria-label="FM-1の画面"', 1),
    ('<button id="go">Switch on</button>', '<button id="go">電源を入れる</button>', 1),
    ('<span id="status">Off</span>', '<span id="status">オフ</span>', 1),
    ("Press <kbd>PLAY</kbd> to start the drones", "<kbd>PLAY</kbd>を押すとドローンが鳴り始めます", 1),
    ("Turn your phone sideways for bigger keys", "スマートフォンを横にすると鍵盤が大きくなります", 1),
    ('href="#keys">Computer keys ↓</a>', 'href="ja/#keys">パソコンのキー ↓</a>', 1),
    ('aria-label="Factory reset" title="Factory reset: forget your sound and scenes, and start again from the factory settings">Reset</button>',
     'aria-label="初期化" title="初期化：サウンドとシーンを消して、初期設定からやり直します">リセット</button>', 1),
    ('aria-label="The manual"', 'aria-label="マニュアル"', 2),
    ("<h2 data-recolor>Start here</h2>", "<h2 data-recolor>はじめに</h2>", 1),
    ('The drones wait for <kbd>PLAY</kbd>, and the white keys hold sounds, not notes.',
     'ドローンは<kbd>PLAY</kbd>を押すまで待っています。白鍵が呼び出すのは音程ではなく、サウンドです。', 1),
    ("<li>Tap <kbd>PLAY</kbd> and the drones fade in</li>", "<li><kbd>PLAY</kbd>をタップすると、ドローンがフェード・インします</li>", 1),
    ("<li>Tap white keys 1–8. There's a preset on each</li>", "<li>白鍵1–8をタップします。それぞれにプリセットが入っています</li>", 1),
    ("<li>Hold a gesture key (the top eight white keys), then let go</li>", "<li>ジェスチャー・キー（上の8つの白鍵）を長押しして、離します</li>", 1),
    ("<li>Turn KNOB 1–4 to change each drone's level and cutoff</li>", "<li>KNOB 1–4を回して、各ドローンのレベルとカットオフを変えます</li>", 1),
    ("<li>To keep your sound, hold <kbd>SEL</kbd> and tap <kbd>G4</kbd> (bank 1), then hold <kbd>SAVE</kbd> and tap a white key to save it there</li>",
     "<li>サウンドを残すには、<kbd>SEL</kbd>を押しながら<kbd>G4</kbd>（バンク1）をタップし、次に<kbd>SAVE</kbd>を押しながら白鍵をタップして、そこに保存します</li>", 1),
    ("""<b>Or build it up yourself:</b> start from one drone with everything off, and add a control
        at a time. <a href="manual/recipes/scratch/">Build a sound from scratch →</a>""",
     """<b>自分で組み立てることもできます：</b>すべてオフのドローン1つから始めて、コントロールを1つずつ加えます。
        <a href="manual/recipes/scratch/">ゼロからサウンドを作る（英語）→</a>""", 1),
    ("<small>I want a sound</small><b>Recipes</b><span>Ticks, metallic bells, warbling echoes, gestures, scenes, transitions</span>",
     "<small>音を作りたい</small><b>レシピ（英語）</b><span>クリック、金属的なベル、揺れるエコー、ジェスチャー、シーン、トランジション</span>", 1),
    ("<small>What does this do</small><b>Reference</b><span>Every page, knob and key, in the order of the buttons</span>",
     "<small>これは何をする？</small><b>リファレンス（英語）</b><span>すべてのページ、ノブ、キーを、ボタンの順に</span>", 1),
    ("<small>Quick lookup</small><b>Shortcuts</b><span>Every key combination, hold and tap</span>",
     "<small>すぐに調べる</small><b>ショートカット（英語）</b><span>キーの組み合わせ、長押しとタップ</span>", 1),
    ('<a href="manual/help/"><svg', '<a href="ja/manual/help/"><svg', 2),     # the door and the menu's (the menu is redrawn below)
    ("<small>Stuck</small><b>Help</b><span>Installing, going back, problems</span>",
     "<small>困ったとき</small><b>ヘルプ</b><span>インストール、元に戻す、トラブル</span>", 1),
    ("<h2 data-recolor>Recipes to try</h2>", "<h2 data-recolor>試してみたいレシピ（英語）</h2>", 1),
    ('alt="The Sub · Drive page, drone I\'s sub at -4 oct"><b>Ticks and clicks</b><span>a sub under 20 Hz turns into a pulse</span>',
     'alt="Sub · Driveページ、ドローン Iのサブが-4 oct"><b>クリックとチック</b><span>20 Hz以下のサブがパルスに変わる</span>', 1),
    ('alt="The Mods page, cross-mod bend each other, X-mod 85"><b>Metallic bells</b><span>cross-FM: two high drones ringing into bells</span>',
     'alt="Modsページ、クロス・モジュレーションで互いを曲げる、X-mod 85"><b>金属的なベル</b><span>クロスFM：高い2つのドローンがベルのように鳴る</span>', 1),
    ('alt="The Mods page, the LFO moving the delay\'s time"><b>Echoes that warble</b><span>the LFO bending every echo</span>',
     'alt="Modsページ、LFOがディレイのタイムを動かす"><b>揺れるエコー</b><span>LFOがすべてのエコーを曲げる</span>', 1),
    ('alt="The Space page, reverb Length Inf, freeze"><b>Freeze a chord, play over it</b><span>reverb at Inf, new notes on top</span>',
     'alt="Spaceページ、リバーブのLengthがInf、フリーズ"><b>和音をフリーズして、その上で弾く</b><span>リバーブをInfにして、新しい音を重ねる</span>', 1),
    ('<h3>Computer keys <span class="g-sub">in this browser</span></h3>', '<h3>パソコンのキー <span class="g-sub">このブラウザーで</span></h3>', 1),
    ("<dt>Turn a knob</dt><dd>drag or scroll</dd>", "<dt>ノブを回す</dt><dd>ドラッグまたはスクロール</dd>", 1),
    ("<dt>Hold a button</dt><dd><kbd>Shift</kbd>-click or right-click; click again to let go</dd>",
     "<dt>ボタンを押し続ける</dt><dd><kbd>Shift</kbd>+クリックまたは右クリック。もう一度クリックで離す</dd>", 1),
    ("<dt>Start or stop</dt>", "<dt>スタート／ストップ</dt>", 1),
    ("<dt>Black keys</dt>", "<dt>黒鍵</dt>", 1),
    ("<dt>White keys</dt>", "<dt>白鍵</dt>", 1),
    ("<dt>SAVE · SEL (hold)</dt>", "<dt>SAVE · SEL（押し続ける）</dt>", 1),
    ("<dt>Any knob</dt><dd><kbd>Tab</kbd> to it, then <kbd>↑</kbd><kbd>↓</kbd></dd>",
     "<dt>どのノブでも</dt><dd><kbd>Tab</kbd>で選んで、<kbd>↑</kbd><kbd>↓</kbd></dd>", 1),
    ("""<p><b>Niagara Drift</b> is free, open-source firmware (GPL‑3.0) for the M-VAVE FM‑1, made by Evan Yerburgh; not made or endorsed by M-VAVE.
    <a href="manual/about/">About</a> · <a href="https://github.com/evanyerburgh/niagara-drift" aria-label="Source on GitHub">Source on GitHub</a></p>""",
     """<p><b>Niagara Drift</b>は、Evan YerburghによるM-VAVE FM‑1用の無料・オープンソースのファームウェア（GPL‑3.0）です。M-VAVEによる製品や公認のものではありません。
    <a href="manual/about/">概要（英語）</a> · <a href="https://github.com/evanyerburgh/niagara-drift">GitHubのソース</a></p>""", 1),
    # the script's words
    ('go.textContent = "Starting...";', 'go.textContent = "起動中…";', 1),
    ("cpu.textContent = `Niagara Drift uses ${m.load}% of this computer's time`;",
     "cpu.textContent = `Niagara Driftはこのパソコンの処理時間の${m.load}%を使っています`;", 1),
    ('status.textContent = ac.sampleRate === 44100 ? "On" : `On (the browser runs audio at ${ac.sampleRate} Hz)`;',
     'status.textContent = ac.sampleRate === 44100 ? "オン" : `オン（ブラウザーのオーディオは${ac.sampleRate} Hzで動いています）`;', 1),
    ('go.textContent = "Switch on";', 'go.textContent = "電源を入れる";', 1),
    ('status.textContent = "Could not start: " + (err && err.message || err) + ". Chrome, Edge, Firefox and Safari should all work.";',
     'status.textContent = "起動できませんでした：" + (err && err.message || err) + "。Chrome、Edge、Firefox、Safariで動くはずです。";', 1),
    ('textContent = "Click again to forget everything";', 'textContent = "もう一度クリックすると、すべて消去します";', 1),
]


def front_ja(page):
    """The Japanese front page from the English one."""
    for en, ja, n in PAIRS:
        if page.count(en) not in (n if isinstance(n, tuple) else (n,)):
            raise SystemExit(f"make_ja: web/emu/index.html has {page.count(en)} of (want {n}):\n  {en[:120]}")
        page = page.replace(en, ja)
    page = page.replace("<head>", '<head>\n<base href="../">', 1)
    # the menu row in Japanese (from the site's root, as <base> reads it); English links back to the English page
    nav = re.search(r"<nav><!--NAV-->.*?</details></nav>", page, re.S)
    if not nav:
        raise SystemExit("make_ja: no <nav><!--NAV--> ... </details></nav> in the front page")
    page = page.replace(nav.group(0), "<nav><!--NAV-->" + menu_html("", "Play it", "ja", "") + "</nav>")
    page = page.replace('<a href="manual/help/">', '<a href="ja/manual/help/">')
    page = page.replace("<!--ACTIONS-->", "<!--ACTIONS-->\n" + JA_NOTE.format(up="").replace('class="note ja-note"', 'class="ja-note"'), 1)
    left = re.findall(r'href="#[\w-]*"', page)
    if left:
        raise SystemExit(f"make_ja: in-page links read through <base> go to the English page: {left}")
    return page


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    site = Path(sys.argv[1])
    out = site / "ja" / "index.html"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(front_ja((site / "index.html").read_text(encoding="utf-8")), encoding="utf-8")
    print(f"ja: {out}")
