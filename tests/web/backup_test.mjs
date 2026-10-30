// SPDX-License-Identifier: GPL-3.0-only
// The web installer's backup and restore (web/fm1ota.js) against a pretend FM-1 that answers as the firmware's
// backup_cmd does (firmware/src/app/main_fm1.c): every object read back as stored, written back as given, the
// file as tools/fm1_install.py writes it. node tests/web/backup_test.mjs [FILE]: FILE gets a backup file (for
// tools/fm1_install.py's read_backup to read: tests/run_tests.sh)
import { pack7, unpack7, ndBackup, ndRestore, ndBackupFile, ndReadBackup, ndBackupName, ndCrc32 } from "../../web/fm1ota.js";
import { writeFileSync } from "node:fs";

let fails = 0;
const check = (what, ok) => { console.log(`${ok ? "ok  " : "FAIL"} ${what}`); if (!ok) fails++; };

function reply(cmd, body) {
  let s = 0;
  for (const b of body) s += b;
  return [0xF0, ...pack7([0x00, 0x59, cmd, body.length & 0xFF, (body.length >> 8) & 0xFF, 0, ...body, ~s & 0xFF]), 0xF7];
}

// the firmware's side: objects 0-3 stored (4 left out: an older FM-1 that has none), a write buffer, the commits; from
// 0.3 (safe) the staging (0x74) and the question (0x75), answered "sel" or "home"; spoil: a byte lost on the way
class FakeFM1 {
  constructor(objs, answers = true, safe = false, answer = "sel") {
    this.store = objs; this.buf = []; this.q = []; this.answers = answers; this.restarted = false; this.reads = 0; this.onRead = null;
    this.safe = safe; this.answer = answer; this.staged = {}; this.spoil = false;
  }
  send(pkt) {
    const u = unpack7(pkt.slice(1, -1)), cmd = u[2], b = u.slice(6, -1);
    if (!this.answers) return true;
    const obj = b[0], off = b[1] | (b[2] << 8);
    if (cmd === 0x70 && obj in this.store) {
      if (off === 0 && this.onRead) this.onRead(obj, ++this.reads);
      const d = this.store[obj], part = Array.from(d.subarray(off, off + 24));
      this.q.push(reply(0x70, [obj, b[1], b[2], d.length & 0xFF, d.length >> 8, ...part]));
    } else if (cmd === 0x71) {
      for (let i = 3; i < b.length; i++) this.buf[off + i - 3] = b[i];
      this.q.push(reply(0x71, [obj, b[1], b[2], 0]));
    } else if (cmd === 0x72) {
      this.store[obj] = Uint8Array.from(this.buf.slice(0, off));
      this.buf = [];
      this.q.push(reply(0x72, [obj, 0]));
    } else if (cmd === 0x74 && this.safe) {
      const d = Uint8Array.from(this.buf.slice(0, off)), c = (b[3] | (b[4] << 8) | (b[5] << 16) | (b[6] << 24)) >>> 0;
      if (this.spoil) d[5] ^= 1;
      const ok = ndCrc32(d) === c;
      if (ok) this.staged[obj] = d;
      this.buf = [];
      this.q.push(reply(0x74, [obj, ok ? 0 : 2]));
    } else if (cmd === 0x75 && this.safe) {
      const want = Object.keys(this.staged).join() === "0,2,3,4" ? 0b11101 : -1;   // every object of the list
      this.q.push(reply(0x75, [b[0] === want ? 0xFF : 2]));
      if (b[0] === want)                       // (the answer later: when the player presses)
        this.q.push(() => {
          if (this.answer === "sel") { Object.assign(this.store, this.staged); this.restarted = true; }
          return reply(0x75, [this.answer === "sel" ? 0 : 1]);
        });
    } else if (cmd === 0x73) {
      this.restarted = true;
    }
    return true;
  }
  async read() { const r = this.q.shift(); return typeof r === "function" ? r() : r || null; }
}

const fill = (n, k) => Uint8Array.from({ length: n }, (_, i) => (i * 7 + k) & 0xFF);
const same = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const objs = { 0: fill(1000, 1), 1: fill(40, 2), 2: fill(0, 3), 3: fill(300, 4) };

const fm = new FakeFM1({ ...objs });
const got = await ndBackup(fm);
check("a backup reads every stored object", got && same(got[0], objs[0]) && same(got[3], objs[3]));
check("... an empty one as empty, a missing one (4) left out, the panel's calibration (1) not taken",
      got && got[2].length === 0 && !(4 in got) && !(1 in got));
const busy = new FakeFM1({ ...objs });
busy.onRead = (obj, n) => { if (obj === 3 && n > 3) busy.store[3] = fill(300, 9); };   /* a save between the two reads */
let changed = false;
try { await ndBackup(busy); } catch (e) { changed = e.code === "changed"; }
check("an FM-1 that changes while it's read: refused", changed);

const text = ndBackupFile(got, "FM-1_800", new Date(2026, 9, 30, 9, 5, 7));
const j = JSON.parse(text);
check("the file is fm1_install.py's", j.niagara_drift_backup === 1 && j.identity === "FM-1_800" && j.saved === "2026-10-30 09:05:07" &&
      Object.keys(j.objects).join() === "0,2,3" && j.crc32[3] === ndCrc32(objs[3]));
check("... its CRC-32 zlib's", ndCrc32(new TextEncoder().encode("123456789")) === 0xCBF43926);
check("... named as it names it", ndBackupName(new Date(2026, 9, 30, 9, 5, 7)) === "backup-20261030-090507.json");
const back = ndReadBackup(text);
check("... and read back the same", same(back[0], objs[0]) && same(back[3], objs[3]) && back[2].length === 0);
const dam = JSON.parse(text);
dam.objects[3] = Buffer.from("bank C changed").toString("base64");
let damaged = false;
try { ndReadBackup(JSON.stringify(dam)); } catch (e) { damaged = e.code === "damaged"; }
check("a damaged file is refused", damaged);
const older = JSON.parse(text);
delete older.crc32;
older.objects[1] = Buffer.from("old panel").toString("base64");
const fromOld = ndReadBackup(JSON.stringify(older));
let refused = false;
try { ndReadBackup('{"something": 1}'); } catch (e) { refused = e.code === "badbackup"; }
check("another file is refused", refused);

const empty = () => ({ 0: new Uint8Array(0), 1: new Uint8Array(0), 2: new Uint8Array(0), 3: new Uint8Array(0) });
const safe = new FakeFM1(empty(), true, true, "sel");
let asked = false, untouched = false;
const res = await ndRestore(safe, back, () => {}, () => { asked = true; untouched = safe.store[0].length === 0 && safe.store[3].length === 0; });
check("THE SAFE RESTORE: all staged, nothing changed when the FM-1 asks", asked && untouched);
check("... SEL: every object switched to (one the backup hasn't, emptied), the FM-1 restarting itself", res === "restored" &&
      same(safe.store[0], objs[0]) && same(safe.store[3], objs[3]) && safe.store[4].length === 0 && safe.restarted);
const home = new FakeFM1(empty(), true, true, "home");
check("... HOME: cancelled, nothing changed", (await ndRestore(home, back)) === "cancelled" && home.store[0].length === 0 &&
      !home.restarted);
const spoilt = new FakeFM1(empty(), true, true, "sel");
spoilt.spoil = true;
let lost = false;
try { await ndRestore(spoilt, back); } catch (e) { lost = e.code === "lost"; }
check("... a byte lost on the way: refused before anything changed", lost && spoilt.store[0].length === 0);
const blank = new FakeFM1(empty());
const oldRes = await ndRestore(blank, back);
check("firmware before 0.3: each object written back, then restarted", oldRes === "restored" && same(blank.store[0], objs[0]) && same(blank.store[3], objs[3]) &&
      blank.restarted);
const keeps = new FakeFM1({ 0: new Uint8Array(0), 1: Uint8Array.from([7, 7]), 2: new Uint8Array(0), 3: new Uint8Array(0) });
await ndRestore(keeps, fromOld);
check("an older backup (no checksums, a calibration in it) restores, the FM-1 keeping its own calibration",
      same(keeps.store[1], [7, 7]) && same(keeps.store[0], objs[0]));

check("not Niagara Drift (no answer): nothing to back up", (await ndBackup(new FakeFM1({}, false))) === null);

if (process.argv[2]) writeFileSync(process.argv[2], text);
console.log(fails ? `backup: ${fails} FAILED` : "backup: all ok");
process.exit(fails ? 1 : 0);
