// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
//
// The FM-1 USB-MIDI update protocol (M-UPGRADE compatible) over WebMIDI.
// Step 1: the running firmware reads parts of the package and stages the update
// loader. Step 2: the loader reads the whole image and writes it. In both steps
// the device sends cmd 0x30 read requests on the logical image and we answer.

const HS_QUERY = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const MAXDATA = 512;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const IS_OTA = (id) => /^ota-/i.test(id.model);

// errors carry a code the pages translate: notfound, model, stopped, noloader, lost, noreturn,
// mismatch (detail: the identity the device reports), badreq
function fail(code, msg, detail = "") { const e = new Error(msg); e.code = code; e.detail = detail; return e; }

export function pack7(data) {
  const out = []; let acc = 0, nb = 0;
  for (const b of data) {
    acc |= b << nb; nb += 8;
    while (nb >= 7) { out.push(acc & 0x7F); acc >>>= 7; nb -= 7; }
  }
  if (nb) out.push(acc & 0x7F);
  return out;
}

export function unpack7(s) {
  const out = []; let acc = 0, nb = 0;
  for (const b of s) {
    acc |= (b & 0x7F) << nb; nb += 7;
    while (nb >= 8) { out.push(acc & 0xFF); acc >>>= 8; nb -= 8; }
  }
  return out;
}

function response(addr, data, fl = 0) {
  const len = data.length;
  const body = [0x00, 0x59, 0x30, (len + 8) & 0xFF, ((len + 8) >> 8) & 0xFF, 0, fl,
    addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF,
    len & 0xFF, (len >> 8) & 0xFF, 0, ...data];
  let s = 0;
  for (let i = 6; i < body.length; i++) s += body[i];
  body.push(~s & 0xFF);
  return [0xF0, ...pack7(body), 0xF7];
}

function parseRequest(pkt) {
  if (pkt[0] !== 0xF0 || pkt[pkt.length - 1] !== 0xF7) return null;
  const u = unpack7(pkt.slice(1, -1));
  if (u.length !== 15 || u[0] !== 0 || u[1] !== 0x59 || u[2] !== 0x30) return null;
  let s = 0;
  for (let i = 6; i < 14; i++) s += u[i];
  if ((~s & 0xFF) !== u[14]) return null;
  return { fl: u[6], addr: (u[7] | (u[8] << 8) | (u[9] << 16) | (u[10] << 24)) >>> 0, len: u[11] | (u[12] << 8) | (u[13] << 16) };
}

function parseIdentity(pkt) {
  const d = unpack7(pkt.slice(1, -1));
  if (d.length !== 34 || d[0] !== 0 || d[1] !== 0x59 || d[2] !== 0x11) return null;
  const txt = String.fromCharCode(...d.slice(6, 33)).replace(/\0+$/, "");
  const m = /^([^_]+)_(\d+)$/.exec(txt);
  return m ? { text: txt, model: m[1], version: parseInt(m[2], 10) } : null;
}

// one MIDI in/out pair with a SysEx queue
class Link {
  constructor(input, output) {
    this.input = input; this.output = output; this.q = []; this.waiter = null;
    input.onmidimessage = (ev) => {
      if (ev.data[0] !== 0xF0) return;
      this.q.push(Array.from(ev.data));
      if (this.waiter) { const w = this.waiter; this.waiter = null; w(); }
    };
  }
  get lost() { return this.input.state === "disconnected" || this.output.state === "disconnected"; }
  // false when the port is gone (unplugged): WebMIDI throws on a disconnected output
  send(bytes) {
    try { this.output.send(bytes); return true; } catch (_) { return false; }
  }
  drain() { this.q.length = 0; }
  async read(ms) {
    if (!this.q.length) {
      let tm;
      await Promise.race([new Promise((r) => { this.waiter = r; }), new Promise((r) => { tm = setTimeout(r, ms); })]);
      clearTimeout(tm);
      this.waiter = null;
    }
    return this.q.shift() || null;
  }
  close() { this.input.onmidimessage = null; }
}

async function handshake(link, tries = 3) {
  link.drain();
  for (let t = 0; t < tries; t++) {
    if (!link.send(HS_QUERY)) return null;
    const end = Date.now() + 1000;
    while (Date.now() < end) {
      const p = await link.read(end - Date.now());
      if (!p) break;
      const id = parseIdentity(p);
      if (id) return id;
    }
  }
  return null;
}

// BACKUP / RESTORE (Niagara Drift v0.2 on; firmware/src/app/main_fm1.c backup_cmd, tools/fm1_install.py nd_backup): the
// stored objects as they lie on the flash, read 24 bytes at a time (0x70), written back 128 at a time (0x71), staged
// (0x74) and switched to together on a yes on the FM-1 (0x75; before 0.3, each committed, 0x72, then a restart). The file is tools/fm1_install.py's: either tool restores the other's.
export const ND_OBJECTS = { 0: "sound and bank A", 1: "panel", 2: "bank B", 3: "bank C", 4: "presets saved over" };
const ND_OPTIONAL = new Set([4]);              // firmware older than the Presets bank (v0.2) has no such object
const ND_UNIT = new Set([1]);                  // the panel's calibration belongs to the unit: never in a backup, never
                                               // restored from one (as fm1-base's: a backup goes onto any FM-1)
// zlib's CRC-32 (Python's zlib.crc32): each object's, in the file, checked before anything is written back
const CRC_T = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
export function ndCrc32(a) { let c = 0xFFFFFFFF; for (const b of a) c = CRC_T[(c ^ b) & 0xFF] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; }

function ndMsg(cmd, body = []) {
  let s = 0;
  for (const b of body) s += b;
  const u = [0x00, 0x59, cmd, body.length & 0xFF, (body.length >> 8) & 0xFF, 0, ...body, ~s & 0xFF];
  return [0xF0, ...pack7(u), 0xF7];
}

function ndReply(pkt, cmd) {
  if (!pkt || pkt.length < 2 || pkt[0] !== 0xF0 || pkt[pkt.length - 1] !== 0xF7) return null;
  const u = unpack7(pkt.slice(1, -1));
  if (u.length < 7 || u[0] !== 0 || u[1] !== 0x59 || u[2] !== cmd || (u[3] | (u[4] << 8) | (u[5] << 16)) + 7 !== u.length) return null;
  let s = 0;
  for (let i = 6; i < u.length - 1; i++) s += u[i];
  return (~s & 0xFF) === u[u.length - 1] ? u.slice(6, -1) : null;
}

// send, and the reply to it (want(reply) says which one it is), or null
async function ndTransact(link, cmd, body, want, tries = 3) {
  for (let t = 0; t < tries; t++) {
    if (!link.send(ndMsg(cmd, body))) throw fail("lost", "FM-1 disconnected");
    const end = Date.now() + 1000;
    while (Date.now() < end) {
      const r = ndReply(await link.read(end - Date.now()), cmd);
      if (r && want(r)) return r;
    }
  }
  return null;
}

// {obj: Uint8Array} (empty when none is stored), or null: this firmware can't make a backup (not Niagara Drift). Each
// object is read twice: one that changed meanwhile (a save while it was read) is refused, as Melodee's are
export async function ndBackup(link) {
  const first = await ndReadAll(link);
  if (!first) return null;
  const again = await ndReadAll(link);
  const same = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
  if (Object.keys(first).join() !== Object.keys(again).join() || Object.keys(first).some((k) => !same(first[k], again[k])))
    throw fail("changed", "backup: the FM-1 changed while it was read");
  return first;
}

async function ndReadAll(link) {
  const objs = {};
  for (const k of Object.keys(ND_OBJECTS)) {
    const obj = +k;
    if (ND_UNIT.has(obj)) continue;
    let data = [], off = 0, total = null;
    while (total === null || off < total) {
      const o = off;
      const r = await ndTransact(link, 0x70, [obj, o & 0xFF, o >> 8], (r) => r.length >= 5 && r[0] === obj && (r[1] | (r[2] << 8)) === o);
      if (!r) {
        if (obj === 0 && off === 0) return null;
        if (ND_OPTIONAL.has(obj) && off === 0) { data = null; break; }
        throw fail("lost", `backup: no answer reading the ${ND_OBJECTS[obj]}`);
      }
      total = r[3] | (r[4] << 8);
      const chunk = r.slice(5);
      if (total && !chunk.length) throw fail("lost", `backup: the ${ND_OBJECTS[obj]} came back short`);
      data = data.concat(chunk);
      off += chunk.length;
      if (!total) break;
    }
    if (data) objs[obj] = Uint8Array.from(data);
  }
  return objs;
}

// THE SAFE RESTORE (v0.3; main_fm1.c 0x74 / 0x75): each object into the FM-1's buffer, then staged there, checked
// against its CRC-32 (nothing changes yet); then the FM-1 asks on its screen (onAsk: tell the player), SEL yes, HOME
// no. "restored" (it restarts itself), "cancelled" (HOME, or a minute without an answer). Firmware before it (no
// answer to the first stage) gets the old way: each object written at once, then a restart
export async function ndRestore(link, objs, log = () => {}, onAsk = () => {}) {
  // every object (fm1-base's journal holds them all): one the backup hasn't, empty, so the FM-1 matches the backup;
  // never the calibration (an older backup's: the FM-1 keeps its own)
  const ks = Object.keys(ND_OBJECTS).map(Number).sort((a, b) => a - b).filter((k) => !ND_UNIT.has(k));
  let mask = 0, old = false;
  for (const k of ks) {
    const data = objs[k] || new Uint8Array(0);
    if (old && !data.length) continue;
    for (let off = 0; off < data.length; off += 128) {
      const part = Array.from(data.subarray(off, off + 128)), o = off;
      const r = await ndTransact(link, 0x71, [k, o & 0xFF, o >> 8, ...part], (r) => r.length >= 4 && r[0] === k && (r[1] | (r[2] << 8)) === o);
      if (!r || r[3]) throw fail("lost", `restore: the ${ND_OBJECTS[k]} wasn't accepted`);
    }
    const c = ndCrc32(data), n = data.length;
    if (!old) {
      const r = await ndTransact(link, 0x74, [k, n & 0xFF, n >> 8, c & 0xFF, (c >>> 8) & 0xFF, (c >>> 16) & 0xFF, c >>> 24], (r) => r.length >= 2 && r[0] === k);
      if (!r && !mask) { old = true; log("(this firmware writes them one at a time)"); if (!n) continue; }
      else if (!r || r[1]) throw fail("lost", `restore: the ${ND_OBJECTS[k]} couldn't be staged (${r ? ["", "refused", "damaged on the way", "flash"][r[1]] : "no answer"}): nothing changed`);
      else { mask |= 1 << k; log(`staged: ${ND_OBJECTS[k]} (${n} bytes)`); }
    }
    if (old) {
      const r = await ndTransact(link, 0x72, [k, n & 0xFF, n >> 8], (r) => r.length >= 2 && r[0] === k);
      if (!r || r[1]) throw fail("lost", `restore: the ${ND_OBJECTS[k]} couldn't be written`);
      log(`restored: ${ND_OBJECTS[k]} (${n} bytes)`);
    }
  }
  if (old) { link.send(ndMsg(0x73)); return "restored"; }
  const a = await ndTransact(link, 0x75, [mask], (r) => r.length >= 1);
  if (!a || a[0] !== 0xFF) throw fail("lost", "restore: the FM-1 didn't ask: nothing changed");
  onAsk();
  const end = Date.now() + 70000;              // (it asks for a minute)
  while (Date.now() < end) {
    const r = ndReply(await link.read(end - Date.now()), 0x75);
    if (!r || !r.length || r[0] === 0xFF) continue;
    if (r[0] === 0) return "restored";
    if (r[0] === 1) return "cancelled";
    throw fail("lost", "restore: the FM-1 couldn't switch to the backup: nothing changed");
  }
  throw fail("lost", "restore: no answer from the FM-1");
}

const b64 = (a) => { let s = ""; for (const b of a) s += String.fromCharCode(b); return btoa(s); };
const unb64 = (s) => Uint8Array.from(atob(s), (c) => c.charCodeAt(0));
const two = (n) => String(n).padStart(2, "0");

// the backup file, as tools/fm1_install.py writes it; and its name, as it names it
export function ndBackupFile(objs, identity, when = new Date()) {
  const saved = `${when.getFullYear()}-${two(when.getMonth() + 1)}-${two(when.getDate())} ${two(when.getHours())}:${two(when.getMinutes())}:${two(when.getSeconds())}`;
  const o = {}, crc32 = {};
  for (const k of Object.keys(objs)) { o[k] = b64(objs[k]); crc32[k] = ndCrc32(objs[k]); }
  return JSON.stringify({ niagara_drift_backup: 1, identity, saved, objects: o, crc32 }, null, 1);
}
export function ndBackupName(when = new Date()) {
  return `backup-${when.getFullYear()}${two(when.getMonth() + 1)}${two(when.getDate())}-${two(when.getHours())}${two(when.getMinutes())}${two(when.getSeconds())}.json`;
}
export function ndReadBackup(text) {
  let j;
  try { j = JSON.parse(text); } catch (_) { throw fail("badbackup", "not a Niagara Drift backup"); }
  if (!j || j.niagara_drift_backup !== 1 || typeof j.objects !== "object") throw fail("badbackup", "not a Niagara Drift backup");
  const objs = {}, crcs = j.crc32 || {};      // (none before 2026-10-30)
  for (const [k, v] of Object.entries(j.objects)) {
    if (!(k in ND_OBJECTS)) throw fail("badbackup", "unknown objects in the backup");
    try { objs[+k] = unb64(v); } catch (_) { throw fail("badbackup", "not a Niagara Drift backup"); }
  }
  if (j.crc32 && (Object.keys(crcs).sort().join() !== Object.keys(objs).sort().join() ||
                  Object.keys(objs).some((k) => ndCrc32(objs[k]) !== crcs[k])))
    throw fail("damaged", "the backup is damaged (a checksum doesn't match)");
  return objs;
}

export class Updater {
  constructor(access, log) {
    this.access = access;
    this.log = log || (() => {});
  }

  // every in/out pair with the same port name that answers the handshake
  async find(filter) {
    const outs = [...this.access.outputs.values()];
    for (const input of this.access.inputs.values()) {
      if (input.state === "disconnected") continue;
      if (!/fm-1|felucca|ota|composite|sinco|usb-midi/i.test(input.name || "")) continue;   // never probe other gear
      const output = outs.find((o) => o.name === input.name) || (outs.length === 1 ? outs[0] : null);
      if (!output) continue;
      try { await input.open(); await output.open(); } catch (_) { continue; }
      const link = new Link(input, output);
      const id = await handshake(link, 2);
      if (id && (!filter || filter(id))) return { link, id, name: input.name };
      link.close();
    }
    return null;
  }

  async waitFor(filter, ms) {
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const dev = await this.find(filter);
      if (dev) return dev;
      await sleep(1000);
    }
    return null;
  }

  // serve read requests until the device asks for one of the finish addresses; stops early
  // when the port disappears (lost: true) instead of waiting out idleMs
  async serve(link, image, finishAddr, idleMs, onProgress) {
    let served = 0, last = Date.now();
    for (;;) {
      const pkt = await link.read(1000);
      if (!pkt) {
        if (link.lost) return { served, finished: false, lost: true };
        if (Date.now() - last > idleMs) return { served, finished: false };
        continue;
      }
      const r = parseRequest(pkt);
      if (!r) continue;
      last = Date.now();
      if (r.addr === 0xE0000000 || r.addr === 0xF0000000) {
        link.send(response(r.addr, [...new TextEncoder().encode("success"), 0]));
        if (r.addr === finishAddr) return { served, finished: true };
        continue;
      }
      if (r.len > MAXDATA || r.addr + r.len > image.length) throw fail("badreq", `bad request ${r.addr.toString(16)}+${r.len}`);
      await sleep(10);
      if (!link.send(response(r.addr, Array.from(image.subarray(r.addr, r.addr + r.len)), r.fl))) {
        return { served, finished: false, lost: true };
      }
      served++;
      if (onProgress) onProgress(served, r.addr);
    }
  }

  // full install: running firmware -> loader -> new firmware
  async install(image, product, onStep) {
    const step = onStep || (() => {});
    const dev = await this.find((id) => !IS_OTA(id));
    if (!dev) throw fail("notfound", "FM-1 not found (USB cable, and no other app using it?)");
    const [model] = product.split("_");
    if (dev.id.model !== model) { dev.link.close(); throw fail("model", `the device is ${dev.id.text}, the package is for ${product}`); }
    step("start", dev.id.text);
    dev.link.send(UPGRADE);
    await sleep(2000);
    const s1 = await this.serve(dev.link, image, 0xE0000000, 8000, (n) => step("verify", n));
    dev.link.close();
    if (!s1.finished) throw fail(s1.lost ? "lost" : "stopped", `the device ${s1.lost ? "was disconnected" : "stopped"} after ${s1.served} requests: nothing was written`);
    step("loader");
    await sleep(3000);
    const ota = await this.waitFor(IS_OTA, 30000);
    if (!ota) throw fail("noloader", "the update loader did not appear. Replug the USB cable and press Install again: the device stays in update mode until it is finished.");
    const s2 = await this.write(ota, image, step);
    if (!s2.finished) throw fail(s2.lost ? "lost" : "stopped", `the loader ${s2.lost ? "was disconnected" : "stopped"} after ${s2.served} requests. Replug and press Install again to resume.`);
    step("reboot");
    await sleep(3000);
    const back = await this.waitFor((id) => !IS_OTA(id), 40000);
    if (!back) throw fail("noreturn", "the device did not come back: power-cycle it");
    back.link.close();
    if (back.id.text !== product) throw fail("mismatch", `installed, but the device reports ${back.id.text}`, back.id.text);
    step("done", back.id.text);
    return back.id.text;
  }

  // Niagara Drift running: its saved objects and its identity, or null (another firmware, or update mode)
  async backup() {
    const dev = await this.find((id) => !IS_OTA(id));
    if (!dev) throw fail("notfound", "FM-1 not found (USB cable, and no other app using it?)");
    try {
      const objs = await ndBackup(dev.link);
      return objs ? { objs, identity: dev.id.text } : null;
    } finally { dev.link.close(); }
  }

  // Niagara Drift running: write a backup back (it restarts with it): "restored", "cancelled" (on the FM-1), or false
  // when the firmware can't take one; onAsk: the FM-1 is asking
  async restore(objs, onAsk) {
    const dev = await this.find((id) => !IS_OTA(id));
    if (!dev) throw fail("notfound", "FM-1 not found (USB cable, and no other app using it?)");
    try {
      if (!(await ndTransact(dev.link, 0x70, [0, 0, 0], (r) => r.length >= 5 && r[0] === 0))) return false;
      return await ndRestore(dev.link, objs, this.log, onAsk);
    } finally { dev.link.close(); }
  }

  // step 2 with the loader found by find()/waitFor()
  async write(ota, image, step) {
    ota.link.send(UPGRADE);
    await sleep(2000);
    const s2 = await this.serve(ota.link, image, 0xF0000000, 180000, (n) => step("write", n));
    ota.link.close();
    return s2;
  }

  // resume: the device is already in update mode (loader) -> true when the write finished
  async resume(image, onStep) {
    const ota = await this.find(IS_OTA);
    if (!ota) return false;
    return (await this.write(ota, image, onStep || (() => {}))).finished;
  }
}
