// SPDX-License-Identifier: GPL-3.0-only
// The manual's pages (tools/make_manual.py): the banner's two bands drifting from its top left to its bottom right
// (violet, and one that darkens), the words pale where they cross, as the front page's name is (reduced motion: still);
// the menu closing on a click elsewhere or Escape; a recipe's listening times.
"use strict";
(() => {
  const top = document.querySelector(".top"), inner = top && top.querySelector(".top-in");
  if (!top || !inner) return;
  const NS = "http://www.w3.org/2000/svg";
  const el = (n, a, p) => { const e = document.createElementNS(NS, n); for (const k in a) e.setAttribute(k, a[k]); p.appendChild(e); return e; };
  const svg = el("svg", { class: "top-bands", "aria-hidden": "true" }, top);
  top.insertBefore(svg, top.firstChild);
  const defs = el("defs", {}, svg), clip = el("clipPath", { id: "topclip", clipPathUnits: "userSpaceOnUse" }, defs);
  // the words again, pale, over the first copy; only the bands' shapes show of them (the menu isn't copied)
  const over = document.createElement("div");
  over.className = "top-over"; over.setAttribute("aria-hidden", "true");
  over.appendChild(inner.cloneNode(true));
  over.inert = true;                         // a copy for the eye only: no focus, nothing read
  over.querySelectorAll("a, button, summary").forEach((a) => a.setAttribute("tabindex", "-1"));
  top.appendChild(over);
  // the bands: two ribbons from the banner's top left to its bottom right, crossing as they drift
  const B = [{ y0: -.12, y1: 1.12, w: .17, amp: .07, len: .55, ph: 0, dark: 0 },
             { y0: .3, y1: .82, w: .15, amp: .1, len: .7, ph: 2.2, dark: 1 }];
  const paths = B.map((b) => el("path", b.dark ? { class: "dark" } : {}, svg));
  const cuts = B.map(() => el("path", {}, clip));
  const still = matchMedia("(prefers-reduced-motion: reduce)"), t1 = performance.now();
  function shape(b, W, H, t) {
    const T = [], U = [], n = Math.ceil(W / 12);
    for (let i = 0; i <= n; i++) {
      const f = -.04 + 1.08 * i / n, x = W * f, a = 2 * Math.PI * f / b.len + b.ph + t * .06;
      const cy = H * (b.y0 + (b.y1 - b.y0) * f + b.amp * Math.sin(a) + .05 * Math.sin(t * .021 + b.ph));
      const hw = H * b.w / 2 * (1 + .25 * Math.sin(2 * Math.PI * f / (b.len * .6) - t * .05));
      T.push(x.toFixed(1) + "," + (cy - hw).toFixed(1));
      U.push(x.toFixed(1) + "," + (cy + hw).toFixed(1));
    }
    return "M" + T.join("L") + "L" + U.reverse().join("L") + "Z";
  }
  function draw(now) {
    const W = top.clientWidth, H = top.clientHeight, t = 30 + (still.matches ? 0 : (now - t1) / 1000);
    if (!W || !H) return;                    // no size yet (n = 0 in shape): nothing to draw
    svg.setAttribute("viewBox", `0 0 ${W} ${H}`);
    B.forEach((b, i) => { const d = shape(b, W, H, t); paths[i].setAttribute("d", d); cuts[i].setAttribute("d", d); });
  }
  let last = 0;
  function tick(now) { if (now - last > 60) { draw(now); last = now; } if (!still.matches) requestAnimationFrame(tick); }
  draw(t1);
  requestAnimationFrame(tick);
  still.addEventListener("change", () => requestAnimationFrame(tick));
  addEventListener("resize", () => draw(performance.now()));
  addEventListener("load", () => draw(performance.now()));   // still (reduced motion) and sized only now
})();
document.addEventListener("click", (e) => document.querySelectorAll("details.menubtn[open]").forEach((d) => { if (!d.contains(e.target)) d.open = false; }));
document.addEventListener("keydown", (e) => { if (e.key === "Escape") document.querySelectorAll("details.menubtn[open]").forEach((d) => d.open = false); });
// two screens stacked taller than the recipe beside them: side by side instead, when that's shorter (wide pages only)
function pairs() {
  document.querySelectorAll(".recipe-body").forEach((b) => {
    b.classList.remove("pair");
    if (innerWidth <= 820 || b.querySelectorAll(".screens figure").length !== 2) return;
    const tall = () => Math.max(b.querySelector(".screens").offsetHeight,
      b.querySelector(".how").offsetHeight + (b.querySelector(".listen") ? b.querySelector(".listen").offsetHeight + 24 : 0));
    const stacked = tall();
    b.classList.add("pair");
    if (tall() >= stacked) b.classList.remove("pair");
  });
}
addEventListener("load", () => {   // then back to the linked recipe, which the layout's change moved
  pairs();
  const t = location.hash && document.getElementById(decodeURIComponent(location.hash.slice(1)));
  if (t) t.scrollIntoView({ behavior: "instant" });
});
addEventListener("resize", pairs);
// the reference's list and a recipe page's: open on a wide screen, folded on a phone (make_manual.py)
const wide = matchMedia("(min-width: 821px)");
wide.addEventListener("change", () => document.querySelectorAll(".toc details, .onpage-nav details").forEach((d) => { d.open = wide.matches; }));
// each sound's player in the site's colors over the hidden <audio> (the browser's own player differs by browser and
// was the one gray thing on the page): play / pause, a bar to seek, the time; one sound at a time
const fmt = (s) => `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, "0")}`;
document.querySelectorAll(".listen audio").forEach((a) => {
  const p = document.createElement("div"), name = (a.getAttribute("aria-label") || "").replace(/: a sound example$/, "");
  p.className = "pl";
  p.setAttribute("role", "group");
  p.setAttribute("aria-label", a.getAttribute("aria-label") || "A sound example");
  p.innerHTML = '<button type="button" class="pp"></button><input type="range" min="0" max="1" step="0.1" value="0"><span class="tm">0:00</span>';
  const b = p.querySelector("button"), r = p.querySelector("input"), tm = p.querySelector(".tm");
  b.setAttribute("aria-label", "Play " + name);
  r.setAttribute("aria-label", "Position in " + name);
  a.after(p);
  a.controls = false;
  a.hidden = true;
  const show = () => {
    const d = a.duration || 0;
    r.max = d || 1;
    r.value = a.currentTime;
    r.style.setProperty("--p", d ? (a.currentTime / d * 100) + "%" : "0%");
    tm.textContent = fmt(a.currentTime) + (d ? " / " + fmt(d) : "");
  };
  b.addEventListener("click", () => { if (a.paused) a.play(); else a.pause(); });
  a.addEventListener("play", () => {
    b.classList.add("on"); b.setAttribute("aria-label", "Pause " + name);
    document.querySelectorAll(".listen audio").forEach((o) => { if (o !== a) o.pause(); });
  });
  a.addEventListener("pause", () => { b.classList.remove("on"); b.setAttribute("aria-label", "Play " + name); });
  for (const e of ["timeupdate", "loadedmetadata", "seeked", "ended"]) a.addEventListener(e, show);
  r.addEventListener("input", () => { a.currentTime = +r.value; show(); });
  show();
});
document.querySelectorAll(".listen").forEach((l) => {
  const a = l.querySelector("audio");
  l.querySelectorAll("button[data-t]").forEach((b) => b.addEventListener("click", () => { a.currentTime = +b.dataset.t; a.play(); }));
});
// the feedback form (Help): sent to Formspree from the page, with the page and the browser; the answer said in place
document.querySelectorAll("form.feedback").forEach((f) => {
  const st = f.querySelector(".feedback-status"), go = f.querySelector("button[type=submit]");
  f.addEventListener("submit", async (e) => {
    e.preventDefault();
    const d = new FormData(f);
    d.append("Page", location.href);
    d.append("Browser", navigator.userAgent);
    d.append("_subject", "Niagara Drift feedback");
    go.disabled = true;
    st.textContent = "Sending";
    try {
      const r = await fetch(f.action, { method: "POST", body: d, headers: { Accept: "application/json" } });
      if (!r.ok) throw new Error((await r.json().catch(() => ({}))).error || r.status);
      f.reset();
      st.textContent = "Sent. Thank you.";
    } catch (err) {
      st.textContent = "Not sent (" + err.message + "). Try again, or open an issue on GitHub.";
    }
    go.disabled = false;
  });
});
