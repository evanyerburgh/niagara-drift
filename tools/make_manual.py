#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The manual's pages for the site, from the Markdown in docs/manual (decided 2026-11-11: the manual as pages of
the site, written as Markdown in the repo so it reads on GitHub too).

  tools/make_manual.py OUT_DIR            the pages into OUT_DIR/manual/ (the emulator's build, the release's site)
  tools/make_manual.py --check            every page read, every link, anchor, picture and recipe checked
  tools/make_manual.py --renders [NAME]   the recipes' screens (docs/manual/media, committed) and sounds
                                          (build/manual-audio, not committed), from docs/manual/renders/*.omni

The Markdown is plain (headings, paragraphs, lists, tables, quotes, links, pictures, `code`, **bold**, *italic*),
with three additions that read well on GitHub too:
  ---\\ntitle: ...\\nintro: ...\\n---     a page's banner (the intro shows without its closing period)
  [[G#3]] [[EDIT]]                      a key or button, in a box
  {#id} {.coming} after a ### heading   a recipe's link name; a recipe not written yet
On a recipe page (docs/manual/recipes/*.md): a ## heading is a group of recipes; each ### heading is a recipe whose
first paragraph, in *italics*, is its line in the list at the top; then what you hear, the moves (1. 2. 3.), a quote
for Good to know, its pictures, and **Listen** (`NAME`) with its times (- 0:03 what to listen for)."""
import hashlib
import html
import math
import re
import shutil
import subprocess
import sys
import wave
from array import array
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
MAN = SRC / "docs" / "manual"
WEB = SRC / "web" / "manual"                       # the pages' style and the banner's bands
AUDIO = SRC / "build" / "manual-audio"
REPO = "https://github.com/evanyerburgh/niagara-drift"
LEVEL_DB = -13.5                                   # every recipe's sound at this loudness (RMS), peaks under -1 dBFS

GH = ('<svg class="gh" viewBox="0 0 24 24" aria-hidden="true"><path fill="currentColor" d="M12 .297c-6.63 0-12 5.373-12 '
      '12 0 5.303 3.438 9.8 8.205 11.385.6.113.82-.258.82-.577 0-.285-.01-1.04-.015-2.04-3.338.724-4.042-1.61-4.042-1.61C4.'
      '422 18.07 3.633 17.7 3.633 17.7c-1.087-.744.084-.729.084-.729 1.205.084 1.838 1.236 1.838 1.236 1.07 1.835 2.809 1.30'
      '5 3.495.998.108-.776.417-1.305.76-1.605-2.665-.3-5.466-1.332-5.466-5.93 0-1.31.465-2.38 1.235-3.22-.135-.303-.54-1.52'
      '3.105-3.176 0 0 1.005-.322 3.3 1.23.96-.267 1.98-.399 3-.405 1.02.006 2.04.138 3 .405 2.28-1.552 3.285-1.23 3.285-1.2'
      '3.645 1.653.24 2.873.12 3.176.765.84 1.23 1.91 1.23 3.22 0 4.61-2.805 5.625-5.475 5.92.42.36.81 1.096.81 2.22 0 1.606-'
      '.015 2.896-.015 3.286 0 .315.21.69.825.57C20.565 22.092 24 17.592 24 12.297c0-6.627-5.373-12-12-12"/></svg>')
HOME = ('<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 11.2 12 3.5l9 7.7" fill="none" stroke="currentColor" '
        'stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/><path d="M5.6 9.6V20h4.6v-5.6h3.6V20h4.6V9.6" '
        'fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"/></svg>')
# the menu, the same on every page: what each holds, for the menu's second line
MENU = [("", "Play it", "the emulator: the firmware itself, in your browser"),
        ("#start-here", "Start here", "on the front page, beside the emulator"),
        ("manual/recipes/", "Recipes", "how to get a sound or a move"),
        ("manual/reference/", "Reference", "every page, knob and key"),
        ("manual/reference/shortcuts/", "Shortcuts", "every combination, hold and tap"),
        ("manual/help/", "Help", "installing, going back, problems"),
        ("manual/about/", "About", "what it does, the details, where it came from")]


def menu_html(up, current):
    """Install, Source and the menu (the front page has the same, web/emu/index.html)."""
    items = "".join(f'<a href="{up}{h}"{" aria-current=\"page\"" if n == current else ""}>{n}<small>{s}</small></a>'
                    for h, n, s in MENU)
    return (f'<a href="{up}install/">Install</a><a href="{REPO}" aria-label="Source on GitHub">Source{GH}</a>'
            f'<details class="menubtn"><summary aria-label="Menu" title="Menu">Menu</summary><div class="panel">{items}</div></details>')


# ---- Markdown ----------------------------------------------------------------------------------------------------

def inline(s):
    """A line's words: `code`, pictures, links, [[keys]], **bold**, *italic*; the rest escaped."""
    keep = []

    def stash(h):
        keep.append(h)
        return f"\x00{len(keep) - 1}\x00"
    s = re.sub(r"`([^`]+)`", lambda m: stash(f"<code>{html.escape(m.group(1))}</code>"), s)
    s = re.sub(r"<a id=\"([\w-]+)\"></a>", lambda m: stash(f'<a id="{m.group(1)}"></a>'), s)
    s = html.escape(s, quote=False)
    s = re.sub(r'!\[([^\]]*)\]\(([^)\s]+)(?: "([^"]*)")?\)',
               lambda m: stash(f'<img src="{m.group(2)}" alt="{html.escape(m.group(1))}">'), s)
    s = re.sub(r"\[\[([^\]]+)\]\]", lambda m: stash(f'<span class="k">{m.group(1)}</span>'), s)
    s = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", lambda m: stash(f'<a href="{m.group(2)}">{m.group(1)}</a>'), s)
    s = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", s)
    s = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<i>\1</i>", s)
    while "\x00" in s:
        s = re.sub(r"\x00(\d+)\x00", lambda m: keep[int(m.group(1))], s)
    return s


def blocks(text):
    """The Markdown's blocks: (kind, data) in order."""
    lines = text.split("\n")
    out, i = [], 0
    while i < len(lines):
        ln = lines[i]
        if not ln.strip():
            i += 1
            continue
        m = re.match(r"(#{1,4})\s+(.*)", ln)
        if m:
            out.append(("h", (len(m.group(1)), m.group(2).strip())))
            i += 1
            continue
        if ln.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                rows.append(lines[i])
                i += 1
            out.append(("table", rows))
            continue
        if ln.startswith(">"):
            q = []
            while i < len(lines) and lines[i].startswith(">"):
                q.append(lines[i][1:].lstrip())
                i += 1
            out.append(("quote", "\n".join(q)))
            continue
        if re.match(r"(\d+\.|-)\s", ln):
            items, ordered = [], ln[0].isdigit()
            while i < len(lines) and lines[i].strip() and not re.match(r"#{1,4}\s|\||>", lines[i]):
                l2 = lines[i]
                if re.match(r"(\d+\.|-)\s", l2):
                    items.append([0, re.sub(r"^(\d+\.|-)\s+", "", l2)])
                elif re.match(r"  +(\d+\.|-)\s", l2):
                    items.append([1, re.sub(r"^\s+(\d+\.|-)\s+", "", l2)])
                else:
                    items[-1][1] += " " + l2.strip()
                i += 1
            out.append(("ol" if ordered else "ul", items))
            continue
        if ln.lstrip().startswith("<"):
            h = []
            while i < len(lines) and lines[i].strip():
                h.append(lines[i])
                i += 1
            out.append(("html", "\n".join(h)))
            continue
        p = []
        while i < len(lines) and lines[i].strip() and not re.match(r"#{1,4}\s|\||>|(\d+\.|-)\s", lines[i]):
            p.append(lines[i].strip())
            i += 1
        out.append(("p", " ".join(p)))
    return out


def slug(s):
    return re.sub(r"[^a-z0-9]+", "-", re.sub(r"<[^>]+>|\*|`|\[\[|\]\]", "", s).lower()).strip("-")


def list_html(kind, items):
    """A list, with a second level under an item (two spaces in)."""
    out, open2 = f"<{kind}>", False
    for n, (lvl, txt) in enumerate(items):
        if lvl == 0:
            if open2:
                out += "</ul>"
                open2 = False
            if n:
                out += "</li>"
            out += f"<li>{inline(txt)}"
        else:
            if not open2:
                out += "<ul>"
                open2 = True
            out += f"<li>{inline(txt)}</li>"
    return out + ("</ul>" if open2 else "") + f"</li></{kind}>"


def table_html(rows):
    cells = [[c.strip() for c in r.strip().strip("|").split("|")] for r in rows]
    head, body = cells[0], [r for r in cells[1:] if not all(re.fullmatch(r":?-+:?", c) for c in r)]
    th = "".join(f"<th>{inline(c)}</th>" for c in head)
    tb = "".join("<tr>" + "".join(f"<td>{inline(c)}</td>" for c in r) + "</tr>" for r in body)
    head_html = f"<thead><tr>{th}</tr></thead>" if any(head) else ""     # a blank header row (| | |) left out
    return f'<div class="tablewrap"><table>{head_html}<tbody>{tb}</tbody></table></div>'


def render(bl, h_shift=0):
    """Blocks to HTML (a heading's level as written, its id from its words or {#id})."""
    out = []
    for kind, d in bl:
        if kind == "h":
            lvl, t = d
            m = re.search(r"\s*\{#([\w-]+)\}\s*$", t)
            hid = m.group(1) if m else slug(t)
            t = re.sub(r"\s*\{[^}]*\}\s*$", "", t)
            out.append(f'<h{lvl + h_shift} id="{hid}">{inline(t)}</h{lvl + h_shift}>')
        elif kind == "p":
            out.append(f"<p>{inline(d)}</p>")
        elif kind in ("ul", "ol"):
            out.append(list_html(kind, d))
        elif kind == "table":
            out.append(table_html(d))
        elif kind == "quote":
            out.append(f'<div class="note">{render(blocks(d))}</div>')
        elif kind == "html":
            out.append(d)
    return "\n".join(out)


def front(text):
    """A page's banner (its title and intro) from the lines between --- and ---."""
    meta = {}
    m = re.match(r"---\n(.*?)\n---\n", text, re.S)
    if m:
        for ln in m.group(1).split("\n"):
            k, _, v = ln.partition(":")
            meta[k.strip()] = v.strip()
        text = text[m.end():]
    return meta, text


# ---- recipes -----------------------------------------------------------------------------------------------------

def recipe_page(text):
    """A recipe page: (groups of (id, title, line, written)), the recipes' HTML)."""
    bl = blocks(text)
    groups, recipes, i = [], [], 0
    while i < len(bl):
        kind, d = bl[i]
        if kind == "h" and d[0] == 2:
            groups.append((d[1], []))
            i += 1
            continue
        if kind == "h" and d[0] == 3:
            t = d[1]
            rid = re.search(r"\{#([\w-]+)", t).group(1)
            coming = ".coming" in t
            title = re.sub(r"\s*\{[^}]*\}\s*$", "", t)
            j = i + 1
            body = []
            while j < len(bl) and not (bl[j][0] == "h" and bl[j][1][0] <= 3):
                body.append(bl[j])
                j += 1
            line = re.sub(r"^\*(.*)\*$", r"\1", body[0][1]) if body and body[0][0] == "p" else ""
            if not groups:
                groups.append(("", []))
            groups[-1][1].append((rid, title, line, not coming))
            if not coming:
                recipes.append(recipe_html(rid, title, body[1:]))
            i = j
            continue
        i += 1
    return groups, recipes


def recipe_html(rid, title, body):
    lead, moves, good, pics, listen = "", "", "", [], None
    k = 0
    while k < len(body):
        kind, d = body[k]
        if kind == "p" and d.startswith("**Listen**"):
            name = re.search(r"`([\w-]+)`", d).group(1)
            rows = body[k + 1][1] if k + 1 < len(body) and body[k + 1][0] == "ul" else []
            listen = (name, [re.match(r"(\d+):(\d\d)\s+(.*)", r[1]).groups() for r in rows])
            k += 2
            continue
        if kind == "p" and d.startswith("!["):
            pics += re.findall(r'!\[([^\]]*)\]\(([^)\s]+) "([^"]*)"\)', d)
        elif kind == "p" and not lead:
            lead = inline(d)
        elif kind == "ol":
            moves = "".join(f"<li>{inline(t)}</li>" for _, t in d)
        elif kind == "quote":
            good = render(blocks(re.sub(r"^\*\*Good to know\.?\*\*\s*", "", d)))
        k += 1
    figs = "".join(f'<figure><img src="../../media/{Path(s).name}" alt="{html.escape(a)}"><figcaption>{inline(c)}</figcaption></figure>'
                   for a, s, c in pics)
    snd = ""
    if listen:
        name, rows = listen
        li = "".join(f'<li><button data-t="{int(mm) * 60 + int(ss)}" aria-label="Play {html.escape(title)} from {mm}:{ss}">'
                     f'{mm}:{ss}</button><span>{inline(w)}</span></li>' for mm, ss, w in rows)
        snd = (f'<div class="listen"><h4 id="{rid}-listen">Listen</h4><audio controls preload="none" '
               f'src="../../media/{name}.mp4" aria-label="{html.escape(title)}: a sound example"></audio><ol>{li}</ol></div>')
    return (f'<section class="recipe" id="{rid}"><div class="recipe-head"><h3>{inline(title)}</h3>'
            f'<button class="share" data-id="{rid}" data-title="{html.escape(title)}" aria-label="Share {html.escape(title)}">Share</button></div>'
            f'<div class="recipe-body"><div><p class="lead">{lead}</p><ol class="moves">{moves}</ol>'
            f'<div class="good"><b>Good to know</b>{good}</div></div><div><div class="screens">{figs}</div>{snd}</div></div></section>')


def onpage(groups, base, label):
    out = ""
    for g, items in groups:
        if g:
            out += f'<h2 class="group" id="{slug(g)}">{inline(g)}</h2>'
        out += '<ul class="onpage">' + "".join(
            (f'<li><a href="{base}#{i}"><b>{inline(t)}</b><span>{inline(s)}</span></a></li>' if done else
             f'<li class="soon"><div><small>coming</small><b>{inline(t)}</b><span>{inline(s)}</span></div></li>')
            for i, t, s, done in items) + "</ul>"
    return f'<nav class="onpage-nav" aria-label="{label}: the recipes">{out}</nav>'


# ---- pages -------------------------------------------------------------------------------------------------------

RECIPE_KINDS = ["sounds", "gestures", "scenes", "time", "performing"]


def page_html(rel, meta, body, current, side=""):
    depth = 1 + (rel.count("/") + 1 if rel else 0)    # folders under the site's root
    up, mup = "../" * depth, "../" * (depth - 1)
    title, intro = meta.get("title", ""), meta.get("intro", "").rstrip(".")
    kicker = meta.get("kicker", "")
    desc = html.escape(meta.get("description", meta.get("intro", "")))
    img = f'<meta property="og:image" content="{up}manual/media/{meta["image"]}">' if meta.get("image") else ""
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)} · Niagara Drift</title>
<meta name="description" content="{desc}"><meta property="og:title" content="{html.escape(title)} · Niagara Drift">
<meta property="og:description" content="{desc}">{img}<meta name="twitter:card" content="summary_large_image">
<link rel="stylesheet" href="{mup}manual.css"></head>
<body>
<a class="skip" href="#main">Skip to content</a>
<div class="top"><div class="top-in">
<nav class="menu" aria-label="Site"><a class="homeicon" href="{up}" aria-label="Home: Niagara Drift, play it in your browser" title="Home">{HOME}</a>{menu_html(up, current)}</nav>
<div class="title"><small>{"Niagara Drift" + (" · " + kicker if kicker else "")}</small><h1>{inline(title)}</h1><p>{inline(intro)}</p></div>
</div></div>
<main id="main" tabindex="-1" class="page{' side' if side else ''}">{side}<div class="prose">{body}</div></main>
<p class="foot">Niagara Drift · free, open-source firmware (GPL-3.0) for the M-VAVE FM-1. Sound examples are rendered from the
firmware's own code on a computer, with headphones' settings, not recorded from an FM-1. <a href="{mup}about/">About</a> ·
<a href="{REPO}">Source on GitHub</a></p>
<script src="{mup}manual.js"></script>
</body></html>"""


def build(out):
    """Every page into out/manual/."""
    dst = out / "manual"
    if dst.exists():
        shutil.rmtree(dst)
    (dst / "media").mkdir(parents=True)
    for f in ("manual.css", "manual.js"):
        shutil.copy(WEB / f, dst / f)
    for f in (MAN / "media").glob("*.png"):
        shutil.copy(f, dst / "media" / f.name)
    for f in (SRC / "docs" / "img").glob("*.png"):
        shutil.copy(f, dst / "media" / f.name)
    for f in AUDIO.glob("*.mp4") if AUDIO.exists() else []:
        shutil.copy(f, dst / "media" / f.name)
    pages = 0
    for md in sorted(MAN.rglob("*.md")):
        if md.relative_to(MAN).as_posix().startswith("renders/"):
            continue
        rel = page_rel(md)
        meta, text = front(md.read_text(encoding="utf-8"))
        kind = rel.split("/")[0] if rel else ""
        current = {"recipes": "Recipes", "help": "Help", "about": "About"}.get(kind, "Reference" if kind == "reference" else "")
        if rel == "reference/shortcuts":
            current = "Shortcuts"
        side = ""
        if rel.startswith("recipes/") and rel.split("/")[1] in RECIPE_KINDS:
            groups, recipes = recipe_page(text)
            first = text.split("\n## ", 1)[0].split("\n### ", 1)[0]     # the words before the first group or recipe
            body = (tabs(rel.split("/")[1]) + render(blocks(first)) + onpage(groups, "", meta.get("title", ""))
                    + "".join(recipes))
            meta.setdefault("kicker", "Recipes")
        elif rel == "recipes":
            body = tabs(None, "") + render(blocks(text), 0) + all_recipes()
        else:
            body = render(blocks(text))
            if kind == "reference":
                side = reference_side(rel)
                meta.setdefault("kicker", "Reference")
        page = page_html(rel, meta, rewrite_links(body, md), current, side)
        p = dst / rel / "index.html" if rel else dst / "index.html"
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(page, encoding="utf-8")
        pages += 1
    missing = sorted({m for p in dst.rglob("index.html") for m in re.findall(r'src="[^"]*media/([\w-]+\.mp4)"', p.read_text())}
                     - {f.name for f in (dst / "media").glob("*.mp4")})
    print(f"manual: {dst}: {pages} pages" + (f"; no sound yet for {len(missing)} recipes (tools/make_manual.py --renders)"
                                               if missing else ""))


def tabs(cur, base="../"):
    out = '<nav class="tabs" aria-label="Recipe kinds">'
    for k in RECIPE_KINDS:
        meta, _ = front((MAN / "recipes" / f"{k}.md").read_text(encoding="utf-8"))
        out += f'<a href="{base}{k}/"{" aria-current=\"page\"" if k == cur else ""}>{meta["title"]}</a>'
    return out + "</nav>"


def all_recipes():
    out = ""
    for k in RECIPE_KINDS:
        meta, text = front((MAN / "recipes" / f"{k}.md").read_text(encoding="utf-8"))
        groups, _ = recipe_page(text)
        flat = [("", [it for _, items in groups for it in items])]
        out += f'<h2 id="{k}">{inline(meta["title"])}</h2><p class="muted">{inline(meta.get("intro", "").rstrip("."))}</p>'
        out += onpage(flat, f"{k}/", meta["title"])
    return out


def reference_side(rel):
    """The reference's pages at the side, in the order of docs/manual/reference/index.md's list."""
    order = re.findall(r"\]\(([\w-]+)\.md\)", (MAN / "reference" / "index.md").read_text(encoding="utf-8"))
    here = rel.split("/")[1] if "/" in rel else ""
    li = ""
    for n in order:
        meta, _ = front((MAN / "reference" / f"{n}.md").read_text(encoding="utf-8"))
        cur = ' aria-current="page"' if n == here else ""
        li += f'<li><a href="{"../" if here else ""}{n}/"{cur}>{inline(meta["title"])}</a></li>'
    return f'<nav class="toc" aria-label="The reference"><h2>Reference</h2><ul>{li}</ul></nav>'


def page_rel(md):
    """docs/manual/X/index.md and X.md are both the page manual/X/ ("" for the manual's first page)."""
    rel = md.relative_to(MAN).with_suffix("").as_posix()
    return "" if rel == "index" else rel[:-6] if rel.endswith("/index") else rel


def rewrite_links(body, md):
    """Links written between the Markdown files (../help.md, gestures.md#keep) as the site's pages."""
    rel = page_rel(md)
    depth = rel.count("/") + 1 if rel else 0

    def fix(m):
        href = m.group(1)
        path, _, frag = href.partition("#")
        if re.match(r"[a-z]+:", href) or not path.endswith(".md"):
            return m.group(0)
        target = (md.parent / path).resolve()
        if not target.is_relative_to(MAN):         # the repo's own files (CHANGELOG.md, BUILDING.md): on GitHub
            return f'href="{REPO}/blob/main/{target.relative_to(SRC).as_posix()}{"#" + frag if frag else ""}"'
        t = page_rel(MAN / target.relative_to(MAN))
        url = "../" * depth + (t + "/" if t else "")
        return f'href="{url or "./"}{"#" + frag if frag else ""}"'
    body = re.sub(r'href="([^"]+)"', fix, body)
    # pictures written for GitHub (docs/img, docs/manual/media) are in the site's manual/media
    return re.sub(r'src="(?:\.\./)*(?:img|media)/([\w.-]+\.png)"', lambda m: f'src="{"../" * depth}media/{m.group(1)}"', body)


# ---- the check ---------------------------------------------------------------------------------------------------

def check():
    """Build into a scratch folder and check every page's links, anchors and pictures; every recipe's render."""
    import tempfile
    bad = []
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        build(out)
        site = out / "manual"
        for p in site.rglob("index.html"):
            t = p.read_text(encoding="utf-8")
            ids = set(re.findall(r'id="([^"]+)"', t))
            for img in re.findall(r"<img [^>]*>", t):
                if 'alt="' not in img:
                    bad.append(f"{p.relative_to(out)}: a picture without alt text: {img[:60]}")
            for href in re.findall(r'(?:href|src)="([^"]+)"', t):
                if re.match(r"[a-z]+:|data:", href):
                    continue
                path, _, frag = href.partition("#")
                if not path:
                    if frag and frag not in ids:
                        bad.append(f"{p.relative_to(out)}: #{frag} isn't on the page")
                    continue
                if path.endswith(".mp4"):
                    name = Path(path).stem
                    if not (MAN / "renders" / f"{name}.omni").exists():
                        bad.append(f"{p.relative_to(out)}: {name}.mp4 has no render (docs/manual/renders/{name}.omni)")
                    continue
                tgt = (p.parent / path).resolve()
                if path.startswith("../") and not str(tgt).startswith(str(site.resolve())):
                    continue                   # the front page, install/: outside the manual
                if tgt.is_dir():
                    tgt = tgt / "index.html"
                if not tgt.exists():
                    bad.append(f"{p.relative_to(out)}: {href}: nothing there")
                elif frag and frag not in re.findall(r'id="([^"]+)"', tgt.read_text(encoding="utf-8")):
                    bad.append(f"{p.relative_to(out)}: {href}: no #{frag} there")
    for b in bad:
        print("manual: " + b)
    if bad:
        raise SystemExit(f"manual: {len(bad)} problems")
    print("manual: every link, anchor, picture and render checked")


# ---- the recipes' screens and sounds -----------------------------------------------------------------------------

def renders(only=None):
    """Each docs/manual/renders/NAME.omni through the host: its shots NAME-0.png .. into docs/manual/media (twice the
    size, sharp), its WAV leveled and made AAC into build/manual-audio/NAME.mp4 (macOS afconvert, or ffmpeg)."""
    host = SRC / "build" / "host" / "omni_host"
    if not host.exists():
        raise SystemExit("no build/host/omni_host: run sh host/build_host.sh")
    from PIL import Image    # noqa: E402  (the .venv's; only for the renders)
    AUDIO.mkdir(parents=True, exist_ok=True)
    tmp = SRC / "build" / "manual-renders"
    for s in sorted((MAN / "renders").glob("*.omni")):
        if only and s.stem not in only:
            continue
        o = tmp / s.stem
        o.mkdir(parents=True, exist_ok=True)
        for f in o.glob("*"):
            f.unlink()
        r = subprocess.run([str(host), str(s), str(o)], capture_output=True, text=True)
        if r.returncode:
            raise SystemExit(f"{s.name}: the host failed:\n{r.stdout}{r.stderr}")
        for png in sorted(o.glob("*.png")):
            Image.open(png).resize((480, 480), Image.NEAREST).save(MAN / "media" / png.name)
        wav = o / f"{s.stem}.wav"
        level(wav, o / "level.wav")
        mp4 = AUDIO / f"{s.stem}.mp4"
        if shutil.which("afconvert"):
            subprocess.run(["afconvert", "-f", "m4af", "-d", "aac", "-b", "160000", str(o / "level.wav"), str(mp4)], check=True)
        else:
            subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(o / "level.wav"), "-c:a", "aac", "-b:a", "160k",
                            "-f", "mp4", str(mp4)], check=True)
        print(f"render: {s.stem}: {len(list(o.glob('*.png')))} screens, {mp4.stat().st_size // 1024} KB")


def level(src, dst):
    """The WAV at LEVEL_DB RMS, raised only as far as its peaks stay under -1 dBFS."""
    with wave.open(str(src)) as w:
        p, a = w.getparams(), array("h", w.readframes(w.getnframes()))
    pk = max(1, max(abs(x) for x in a)) / 32768
    rms = max(1e-9, math.sqrt(sum(x * x for x in a) / len(a)) / 32768)
    g = min(10 ** ((LEVEL_DB - 20 * math.log10(rms)) / 20), 10 ** ((-1 - 20 * math.log10(pk)) / 20))
    with wave.open(str(dst), "wb") as w:
        w.setparams(p)
        w.writeframes(array("h", (int(round(x * g)) for x in a)).tobytes())


def render_times(path):
    """A render script's "# @ words" marks, at their times from the WAV's start (the host's own step lengths), to
    write a recipe's listening times from."""
    t, out, pend = None, [], None
    for line in open(path):
        s = line.strip()
        if s.startswith("# @"):
            pend = s[3:].strip()
            continue
        if not s or s.startswith("#"):
            continue
        w = s.split()
        if w[0] == "wav":
            t = 0.0
            continue
        if t is not None and pend is not None:
            out.append((t, pend))
            pend = None
        d = {"wait": lambda: int(w[1]), "tap": lambda: 40, "tapkey": lambda: 90, "press": lambda: 2, "release": lambda: 2,
             "turn": lambda: abs(int(w[2])) * (int(w[3]) if len(w) > 3 else 80)}.get(w[0], lambda: 0)()
        if t is not None:
            t += d / 1000
    return out


if __name__ == "__main__":
    a = sys.argv[1:]
    if a[:1] == ["--check"]:
        check()
    elif a[:1] == ["--renders"]:
        renders(set(a[1:]) or None)
    elif a[:1] == ["--times"]:
        for f in a[1:]:
            print(f)
            for t, w in render_times(f):
                print(f"  - {int(t) // 60}:{int(t) % 60:02d} {w}")
    elif len(a) == 1:
        build(Path(a[0]))
    else:
        sys.exit(__doc__)
