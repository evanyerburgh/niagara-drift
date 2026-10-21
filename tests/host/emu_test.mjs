// SPDX-License-Identifier: GPL-3.0-only
// The browser build in Node: boot it, press PLAY, expect the drone to fade in, and screen frames.
import fs from "fs";
const wasm = fs.readFileSync(process.argv[2]);
let mem;
const wasi = new Proxy({ clock_time_get: (id, p, out) => { new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0; },
  fd_write: (fd, iov, n, out) => { new DataView(mem.buffer).setUint32(out, 0, true); return 0; } }, { get: (t, k) => k in t ? t[k] : () => 0 });
const { instance } = await WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasi, env: new Proxy({}, { get: () => () => 0 }) });
const ex = instance.exports; mem = ex.memory;
if (ex._initialize) ex._initialize();
ex.web_master(2800); ex.web_boot();
const PLAY = 10;
let silent = 0, peak = 0;
for (let i = 0; i < 1200; i++) {                     // 128 frames a call: ~3.5 s
  if (i === 100) ex.web_buttons(1 << PLAY);
  if (i === 110) ex.web_buttons(0);
  ex.web_render(128);
  const l = new Float32Array(mem.buffer, ex.web_out_l(), 128);
  for (const v of l) {
    if (i < 100) silent = Math.max(silent, Math.abs(v));
    else peak = Math.max(peak, Math.abs(v));
  }
}
// the black keys' names follow the page: the LFO layer on the LFO pages (the page's labels read web_keys_lfo)
const tap = (bit) => { ex.web_buttons(1 << bit); for (let i = 0; i < 10; i++) ex.web_render(128); ex.web_buttons(0); for (let i = 0; i < 60; i++) ex.web_render(128); };
const LFO = 3, HOME = 6, before = ex.web_keys_lfo();
tap(LFO); const onLfo = ex.web_keys_lfo();
tap(HOME); const back = ex.web_keys_lfo();
console.log("before PLAY", silent.toFixed(3), "after", peak.toFixed(3), "blits", ex.web_blits(), "keys' LFO layer", before, onLfo, back);
process.exit(silent === 0 && peak > 0.05 && peak < 1 && ex.web_blits() > 10 && !before && onLfo && !back ? 0 : 1);
