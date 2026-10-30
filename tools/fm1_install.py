#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Install a Felucca package (.fwsc) on an FM-1 over USB-MIDI.

The same update as the web installer (web/fm1ota.js): step 1, the
running firmware reads parts of the package and starts the update loader;
step 2, the loader reads the whole package and writes it. In both steps the
device asks (SysEx read requests on the logical image) and we answer. Then
the FM-1 restarts and the installed identity is checked.

  fm1_install.py PACKAGE.fwsc [--port NAME] [--yes] [--force] [--no-backup]
  fm1_install.py --info [--port NAME]
  fm1_install.py --backup [FILE]       Niagara Drift: save the sound, the scenes and the settings to a file
  fm1_install.py --restore FILE        ... and put them back (the FM-1 restarts)

Before an install over Niagara Drift, a backup is saved first, in ~/niagara-drift-backups/ (--no-backup:
not). Restore it after coming back from another firmware.

If the FM-1 is still in update mode (an earlier install was cut off), the
install finishes the write. Needs mido with python-rtmidi.

Exit codes: 0 done, 1 cancelled or other error, 2 bad arguments or package,
3 FM-1 not found, 4 connection lost or the device stopped, 5 timeout (no
loader / no restart), 6 wrong model, or another identity after the install.
"""
import argparse
import base64
import json
import os
import queue
import re
import sys
import time
import zlib

HS_QUERY = bytes([0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7])
UPGRADE = bytes([0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7])
FINISH_CHECK, FINISH_WRITE = 0xE0000000, 0xF0000000
MAXDATA = 512
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
LOADER_MARK = b"FELUCCA-LOADER-1"
PORT_RE = re.compile(r"fm-1|felucca|ota|composite|sinco|usb-midi", re.I)   # never probe other gear

# seconds; the tests shorten them
DELAY = {"open": 0.3, "start": 2.0, "reply": 0.01, "loader": 3.0, "reboot": 3.0, "retry": 1.0,
         "poll": 1.0, "hs": 1.0, "idle_check": 8.0, "idle_write": 180.0,
         "wait_loader": 30.0, "wait_reboot": 40.0, "backup": 1.0}

EXIT = {"usage": 2, "badpkg": 2, "notfound": 3, "model": 6, "lost": 4, "stopped": 4, "badreq": 4,
        "noloader": 5, "noreturn": 5, "mismatch": 6}


class InstallError(Exception):
    def __init__(self, code, msg):
        super().__init__(msg)
        self.code = code


# ------------------------------------------------------------ wire format ---

def pack7(data):
    out, acc, nb = [], 0, 0
    for b in data:
        acc |= b << nb
        nb += 8
        while nb >= 7:
            out.append(acc & 0x7F)
            acc >>= 7
            nb -= 7
    if nb:
        out.append(acc & 0x7F)
    return bytes(out)


def unpack7(s):
    out, acc, nb = [], 0, 0
    for b in s:
        acc |= (b & 0x7F) << nb
        nb += 7
        while nb >= 8:
            out.append(acc & 0xFF)
            acc >>= 8
            nb -= 8
    return bytes(out)


def response(addr, data, fl=0):
    n = len(data)
    body = bytearray([0x00, 0x59, 0x30, (n + 8) & 0xFF, ((n + 8) >> 8) & 0xFF, 0, fl])
    body += addr.to_bytes(4, "little") + bytes([n & 0xFF, (n >> 8) & 0xFF, 0]) + bytes(data)
    body.append(~sum(body[6:]) & 0xFF)
    return b"\xF0" + pack7(body) + b"\xF7"


def parse_request(pkt):
    """-> (flags, addr, len) or None"""
    if len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    u = unpack7(pkt[1:-1])
    if len(u) != 15 or u[:3] != b"\x00\x59\x30" or ~sum(u[6:14]) & 0xFF != u[14]:
        return None
    return u[6], int.from_bytes(u[7:11], "little"), int.from_bytes(u[11:14], "little")


class Identity:
    def __init__(self, text, model, version):
        self.text, self.model, self.version = text, model, version

    @property
    def loader(self):
        return self.model.lower().startswith("ota-")


def parse_identity(pkt):
    if len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    d = unpack7(pkt[1:-1])
    if len(d) != 34 or d[:3] != b"\x00\x59\x11":
        return None
    txt = d[6:33].rstrip(b"\0").decode("latin-1")
    m = re.fullmatch(r"([^_]+)_(\d+)", txt)
    return Identity(txt, m[1], int(m[2])) if m else None


# --------------------------------------------------------------- package ---

def product_of(raw):
    """the package identity ("FM-1_906"): one marker byte after each of the first 20 blocks"""
    if len(raw) < BLOCKS * BLK:
        raise InstallError("badpkg", "not an FM-1 package (too short)")
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def logical_image(raw):
    """the image the device reads during an update: the package without the 20 marker bytes"""
    out = bytearray()
    for i in range(BLOCKS):
        out += raw[i * BLK:i * BLK + KEEP]
    return bytes(out + raw[BLOCKS * BLK:])


def model_of(text):
    return text.split("_")[0].removeprefix("ota-")


# ------------------------------------------------------------------ MIDI ---

class MidoLink:
    """one MIDI in/out pair with a SysEx queue"""

    def __init__(self, backend, in_name, out_name):
        import mido
        self.backend, self.name, self.mido = backend, in_name, mido
        self.q = queue.Queue()
        self.inp = mido.open_input(in_name, callback=self._rx)
        try:
            self.out = mido.open_output(out_name)
        except Exception:
            self.inp.close()
            raise
        time.sleep(DELAY["open"])   # CoreMIDI may drop a message sent just after opening

    def _rx(self, msg):
        if msg.type == "sysex":
            self.q.put(bytes([0xF0, *msg.data, 0xF7]))

    @property
    def lost(self):
        return self.name not in self.backend.input_names()

    def send(self, pkt):
        try:
            self.out.send(self.mido.Message("sysex", data=pkt[1:-1]))
            return True
        except Exception:
            return False

    def read(self, timeout):
        try:
            return self.q.get(timeout=max(timeout, 0))
        except queue.Empty:
            return None

    def drain(self):
        while not self.q.empty():
            self.q.get_nowait()

    def close(self):
        for p in (self.inp, self.out):
            try:
                p.close()
            except Exception:
                pass


class MidoBackend:
    def __init__(self):
        import mido
        self.mido = mido

    def input_names(self):
        return list(dict.fromkeys(self.mido.get_input_names()))

    def output_names(self):
        return list(dict.fromkeys(self.mido.get_output_names()))

    def open(self, in_name, out_name):
        return MidoLink(self, in_name, out_name)


def pair_output(name, outs):
    if name in outs:
        return name
    base = re.sub(r"\s+\d+$", "", name)     # Windows numbers each port
    same = [o for o in outs if re.sub(r"\s+\d+$", "", o) == base]
    if len(same) == 1:
        return same[0]
    return outs[0] if len(outs) == 1 else None


# -------------------------------------------------------- backup / restore ---
# Niagara Drift's stored objects (firmware/src/app/main_fm1.c, backup_cmd): the sound and bank A, the panel
# calibration, banks B and C, read and written as they lie on the flash.
OBJECTS = {0: "sound and bank A", 1: "panel", 2: "bank B", 3: "bank C", 4: "presets saved over"}
OPTIONAL = {4}                     # firmware older than the Presets bank (v0.2) has no such object: none to keep
UNIT = {1}                         # the panel's calibration belongs to the unit: never in a backup, never restored
                                   # from one (Evan, 2026-10-30: as fm1-base's; a backup goes onto any FM-1)
BACKUP_DIR = os.path.join(os.path.expanduser("~"), "niagara-drift-backups")


def nd_msg(cmd, body=b""):
    body = bytes(body)
    u = bytes([0x00, 0x59, cmd, len(body) & 0xFF, (len(body) >> 8) & 0xFF, 0]) + body + bytes([~sum(body) & 0xFF])
    return b"\xF0" + pack7(u) + b"\xF7"


def nd_reply(pkt, cmd):
    if not pkt or len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    u = unpack7(pkt[1:-1])
    if len(u) < 7 or u[:3] != bytes([0, 0x59, cmd]) or int.from_bytes(u[3:6], "little") + 7 != len(u):
        return None
    if ~sum(u[6:-1]) & 0xFF != u[-1]:
        return None
    return u[6:-1]


def nd_transact(link, cmd, body, want, tries=3):
    """send, and the reply to it: want(reply) says which one it is"""
    for _ in range(tries):
        if not link.send(nd_msg(cmd, body)):
            raise InstallError("lost", "FM-1 disconnected")
        end = time.monotonic() + DELAY["backup"]
        while (left := end - time.monotonic()) > 0:
            r = nd_reply(link.read(left), cmd)
            if r is not None and want(r):
                return r
    return None


def nd_backup(link):
    """{obj: bytes} (b"" when none is stored), or None: this firmware can't make a backup. Each object is read twice:
    one that changed meanwhile (a save while it was read) is refused, as Melodee's are"""
    first = nd_read_all(link)
    if first is None:
        return None
    if nd_read_all(link) != first:
        raise InstallError("lost", "backup: the FM-1 changed while it was read: stop it (PLAY) and try again")
    return first


def nd_read_all(link):
    objs = {}
    for obj in OBJECTS:
        if obj in UNIT:
            continue
        data, off, total = b"", 0, None
        while total is None or off < total:
            r = nd_transact(link, 0x70, [obj, off & 0xFF, off >> 8],
                            lambda r: len(r) >= 5 and r[0] == obj and r[1] | r[2] << 8 == off)
            if r is None:
                if obj == 0 and off == 0:
                    return None
                if obj in OPTIONAL and off == 0:
                    data = None               # this firmware hasn't got it: nothing to keep
                    break
                raise InstallError("lost", f"backup: no answer reading the {OBJECTS[obj]}")
            total, chunk = r[3] | r[4] << 8, r[5:]
            if total and not chunk:
                raise InstallError("lost", f"backup: the {OBJECTS[obj]} came back short")
            data += chunk
            off += len(chunk)
            if not total:
                break
        if data is not None:
            objs[obj] = data
    return objs


def nd_restore(link, objs, out, on_ask=lambda: None):
    """THE SAFE RESTORE (v0.3; main_fm1.c 0x74 / 0x75): each object into the FM-1's buffer, then staged there, checked
    against its CRC-32 (nothing changes yet); then the FM-1 asks on its screen, SEL yes, HOME no. "restored" (it
    restarts itself) or "cancelled". Firmware before it (no answer to the first stage): each written at once, then a
    restart, as before"""
    mask, old = 0, False
    for obj in sorted(OBJECTS):              # every object (fm1-base's journal holds them all): one the backup hasn't,
        data = objs.get(obj) or b""          # empty (the FM-1 then matches the backup); never the calibration
        if obj in UNIT or (old and not data):
            continue
        for off in range(0, len(data), 128):
            part = data[off:off + 128]
            r = nd_transact(link, 0x71, bytes([obj, off & 0xFF, off >> 8]) + part,
                            lambda r: len(r) >= 4 and r[0] == obj and r[1] | r[2] << 8 == off)
            if r is None or r[3]:
                raise InstallError("lost", f"restore: the {OBJECTS[obj]} wasn't accepted")
        n, c = len(data), zlib.crc32(data)
        if old and not n:
            continue
        if not old:
            r = nd_transact(link, 0x74, bytes([obj, n & 0xFF, n >> 8]) + c.to_bytes(4, "little"),
                            lambda r: len(r) >= 2 and r[0] == obj)
            if r is None and not mask:
                old = True
                print("(this firmware writes them one at a time)", file=out)
                if not n:
                    continue
            elif r is None or r[1]:
                why = "no answer" if r is None else {1: "refused", 2: "damaged on the way", 3: "flash"}.get(r[1], "?")
                raise InstallError("lost", f"restore: the {OBJECTS[obj]} couldn't be staged ({why}): nothing changed")
            else:
                mask |= 1 << obj
                print(f"staged: {OBJECTS[obj]} ({n} bytes)", file=out)
        if old:
            r = nd_transact(link, 0x72, [obj, n & 0xFF, n >> 8], lambda r: len(r) >= 2 and r[0] == obj)
            if r is None or r[1]:
                raise InstallError("lost", f"restore: the {OBJECTS[obj]} couldn't be written")
            print(f"restored: {OBJECTS[obj]} ({n} bytes)", file=out)
    if old:
        link.send(nd_msg(0x73))
        return "restored"
    r = nd_transact(link, 0x75, [mask], lambda r: len(r) >= 1)
    if r is None or r[0] != 0xFF:
        raise InstallError("lost", "restore: the FM-1 didn't ask: nothing changed")
    on_ask()
    end = time.monotonic() + 70.0            # (it asks for a minute)
    while (left := end - time.monotonic()) > 0:
        r = nd_reply(link.read(left), 0x75)
        if not r or r[0] == 0xFF:
            continue
        if r[0] == 0:
            return "restored"
        if r[0] == 1:
            return "cancelled"
        raise InstallError("lost", "restore: the FM-1 couldn't switch to the backup: nothing changed")
    raise InstallError("lost", "restore: no answer from the FM-1")


def backup_file(objs, ident, path):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w") as f:
        json.dump({"niagara_drift_backup": 1, "identity": ident.text, "saved": time.strftime("%Y-%m-%d %H:%M:%S"),
                   "objects": {str(k): base64.b64encode(v).decode() for k, v in objs.items()},
                   "crc32": {str(k): zlib.crc32(v) for k, v in objs.items()}}, f, indent=1)


def read_backup(path):
    try:
        with open(path) as f:
            j = json.load(f)
        objs = {int(k): base64.b64decode(v) for k, v in j["objects"].items()}
        crcs = {int(k): int(v) for k, v in j.get("crc32", {}).items()}   # (none before 2026-10-30)
    except (OSError, ValueError, KeyError, TypeError) as e:
        raise InstallError("usage", f"{path}: not a Niagara Drift backup ({e})")
    if any(k not in OBJECTS for k in objs):
        raise InstallError("usage", f"{path}: unknown objects in the backup")
    if crcs and (set(crcs) != set(objs) or any(zlib.crc32(v) != crcs[k] for k, v in objs.items())):
        raise InstallError("usage", f"{path}: the backup is damaged (a checksum doesn't match): nothing was written")
    return objs


def default_backup_path():
    return os.path.join(BACKUP_DIR, "backup-" + time.strftime("%Y%m%d-%H%M%S") + ".json")


# --------------------------------------------------------------- updater ---

class Device:
    def __init__(self, link, ident, name):
        self.link, self.id, self.name = link, ident, name


def handshake(link, tries=3):
    link.drain()
    for _ in range(tries):
        if not link.send(HS_QUERY):
            return None
        end = time.monotonic() + DELAY["hs"]
        while (left := end - time.monotonic()) > 0:
            p = link.read(left)
            if p is None:
                break
            ident = parse_identity(p)
            if ident:
                return ident
    return None


class Updater:
    def __init__(self, backend, port=None):
        self.backend = backend
        self.port = port

    def candidates(self, only_port):
        names = self.backend.input_names()
        if self.port:
            mine = [n for n in names if self.port.lower() in n.lower()]
            if only_port:
                return mine
            return mine + [n for n in names if n not in mine and PORT_RE.search(n)]
        return [n for n in names if PORT_RE.search(n)]

    def find(self, want=None, only_port=True):
        """the first matching port that answers the handshake (and want(identity))"""
        outs = self.backend.output_names()
        for name in self.candidates(only_port):
            out = pair_output(name, outs)
            if not out:
                continue
            try:
                link = self.backend.open(name, out)
            except Exception:
                continue
            ident = handshake(link, 2)
            if ident and (want is None or want(ident)):
                return Device(link, ident, name)
            link.close()
        return None

    def wait_for(self, want, secs):
        end = time.monotonic() + secs
        while time.monotonic() < end:
            dev = self.find(want, only_port=False)
            if dev:
                return dev
            time.sleep(DELAY["retry"])
        return None

    def serve(self, link, image, finish, idle, progress):
        """answer read requests until the device asks for `finish`; -> (served, finished, lost)"""
        served, last = 0, time.monotonic()
        while True:
            pkt = link.read(DELAY["poll"])
            if pkt is None:
                if link.lost:
                    return served, False, True
                if time.monotonic() - last > idle:
                    return served, False, False
                continue
            r = parse_request(pkt)
            if not r:
                continue
            fl, addr, n = r
            last = time.monotonic()
            if addr in (FINISH_CHECK, FINISH_WRITE):
                link.send(response(addr, b"success\0"))
                if addr == finish:
                    return served, True, False
                continue
            if n > MAXDATA or addr + n > len(image):
                raise InstallError("badreq", f"the device asked for {addr:#x}+{n}, outside the package")
            time.sleep(DELAY["reply"])
            if not link.send(response(addr, image[addr:addr + n], fl)):
                return served, False, True
            served += 1
            progress(served, addr + n)

    def check(self, dev, image, step):
        """step 1: the running firmware checks the package and starts the loader"""
        step("start", dev.id.text)
        dev.link.send(UPGRADE)
        time.sleep(DELAY["start"])
        served, finished, lost = self.serve(dev.link, image, FINISH_CHECK, DELAY["idle_check"],
                                            lambda k, end: step("check", k))
        dev.link.close()
        if not finished:
            raise InstallError("lost" if lost else "stopped",
                               f"the FM-1 {'was disconnected' if lost else 'stopped answering'} after "
                               f"{served} requests; nothing was written")
        step("loader")
        time.sleep(DELAY["loader"])
        ota = self.wait_for(lambda i: i.loader, DELAY["wait_loader"])
        if not ota:
            raise InstallError("noloader", "the update loader did not appear. Replug the USB cable and "
                                           "run the install again: the FM-1 stays in update mode until it is done")
        return ota

    def write(self, ota, image, step):
        """step 2: the loader reads and writes the whole package, then restarts"""
        step("write", 0)
        ota.link.send(UPGRADE)
        time.sleep(DELAY["start"])
        served, finished, lost = self.serve(ota.link, image, FINISH_WRITE, DELAY["idle_write"],
                                            lambda k, end: step("write", min(99, end * 100 // len(image))))
        ota.link.close()
        if not finished:
            raise InstallError("lost" if lost else "stopped",
                               f"the loader {'was disconnected' if lost else 'stopped answering'} after "
                               f"{served} requests. Replug the USB cable and run the install again to finish")
        step("write", 100)

    def verify(self, product, step):
        step("reboot")
        time.sleep(DELAY["reboot"])
        back = self.wait_for(lambda i: not i.loader, DELAY["wait_reboot"])
        if not back:
            raise InstallError("noreturn", "the FM-1 did not come back: power-cycle it")
        back.link.close()
        if back.id.text != product:
            raise InstallError("mismatch", f"written, but the FM-1 reports {back.id.text} (expected {product})")
        step("done", back.id.text)
        return back.id.text

    def install(self, dev, image, product, step=lambda *a: None):
        """running firmware -> loader -> new firmware; dev from find(). A device already in
        update mode only needs the write."""
        if model_of(dev.id.text) != model_of(product):
            dev.link.close()
            raise InstallError("model", f"the device is {dev.id.text}, the package is for {product}")
        ota = dev if dev.id.loader else self.check(dev, image, step)
        self.write(ota, image, step)
        return self.verify(product, step)


# ------------------------------------------------------------------- CLI ---

def load_package(path, force):
    try:
        raw = open(path, "rb").read()
    except OSError as e:
        raise InstallError("badpkg", f"cannot read {path}: {e.strerror}")
    product = product_of(raw)
    if not re.fullmatch(r"[^_]+_\d+", product):
        raise InstallError("badpkg", f"{path}: not an FM-1 package (identity {product!r})")
    if LOADER_MARK not in raw and not force:
        raise InstallError("badpkg", f"{path}: no Felucca update loader in this package; "
                                     "only Felucca's own packages are installed (--force overrides)")
    return product, logical_image(raw)


def not_found(up):
    names = up.backend.input_names()
    where = f"no MIDI port matching {up.port!r} answered" if up.port else "FM-1 not found"
    return InstallError("notfound", f"{where} (USB data cable? another app using it?)"
                        f"\n  MIDI inputs: {', '.join(names) if names else '(none)'}")


class Progress:
    def __init__(self, out):
        self.out, self.line = out, False

    def __call__(self, k, a=None):
        if k == "check":
            self.put(f"checking the package ({a} reads)")
        elif k == "write":
            self.put(f"writing {a:3d}%")
        else:
            self.end()
            msg = {"start": f"starting the update on {a}", "loader": "switching to update mode",
                   "reboot": "restarting", "done": f"done: the FM-1 runs {a}"}[k]
            print(msg, file=self.out, flush=True)

    def put(self, s):
        print(f"\r{s}\033[K", end="", file=self.out, flush=True)
        self.line = True

    def end(self):
        if self.line:
            print(file=self.out, flush=True)
            self.line = False


def run(a, backend, out, ask):
    up = Updater(backend, a.port)
    if a.info:
        dev = up.find()
        if not dev:
            raise not_found(up)
        dev.link.close()
        mode = "update loader (update not finished)" if dev.id.loader else "running"
        print(f"{dev.id.text}  [{mode}]  port: {dev.name}", file=out)
        return 0
    if a.backup is not None or a.restore:
        dev = up.find()
        if not dev:
            raise not_found(up)
        if dev.id.loader:
            raise InstallError("usage", "the FM-1 is in update mode: finish the install first")
        try:
            if a.restore:
                objs = read_backup(a.restore)
                if not a.yes and not ask(f"Restore {a.restore} onto the FM-1 ({dev.id.text})? It replaces its "
                                         "sound and scenes, and it restarts. [y/N] "):
                    print("cancelled", file=out)
                    return 1
                r = nd_restore(dev.link, objs, out,
                               lambda: print("On the FM-1: press SEL to put the backup back (HOME keeps what it has)",
                                             file=out, flush=True))
                if r == "cancelled":
                    print("not put back: the FM-1 kept its sound and scenes", file=out)
                    return 1
                print("done: the FM-1 restarts with the restored sound and scenes", file=out)
                return 0
            objs = nd_backup(dev.link)
            if objs is None:
                raise InstallError("model", "this firmware can't make a backup (Niagara Drift v0.2 or later can)")
            path = a.backup or default_backup_path()
            backup_file(objs, dev.id, path)
            print(f"backup: {path}", file=out)
            return 0
        finally:
            dev.link.close()
    product, image = load_package(a.package, a.force)
    print(f"package: {product}  ({a.package})", file=out)
    dev = up.find()
    if not dev:
        raise not_found(up)
    print(f"device:  {dev.id.text}  ({dev.name})" + ("  in update mode: the write will be finished" if dev.id.loader else ""),
          file=out)
    if not a.yes and not ask("Install? Do not unplug the FM-1 while writing. [y/N] "):
        dev.link.close()
        print("cancelled", file=out)
        return 1
    if not dev.id.loader and not a.no_backup:  # Niagara Drift: its sound and scenes saved first
        objs = nd_backup(dev.link)
        if objs is not None:
            path = default_backup_path()
            backup_file(objs, dev.id, path)
            print(f"backup:  {path}  (put back with --restore)", file=out)
    prog = Progress(out)
    try:
        up.install(dev, image, product, prog)
    finally:
        prog.end()
    return 0


def ask_tty(prompt):
    try:
        return input(prompt).strip().lower() in ("y", "yes")
    except EOFError:
        return False


def main(argv=None, backend=None, out=sys.stdout, ask=ask_tty):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("package", nargs="?", help="Felucca package (.fwsc)")
    ap.add_argument("--info", action="store_true", help="print the identity of the connected FM-1")
    ap.add_argument("--port", metavar="NAME", help="MIDI port to use (part of its name)")
    ap.add_argument("--yes", action="store_true", help="do not ask for confirmation")
    ap.add_argument("--force", action="store_true", help="install a package without the Felucca loader marker")
    ap.add_argument("--backup", nargs="?", const="", metavar="FILE",
                    help="save the sound, scenes and settings (Niagara Drift) to FILE (default ~/niagara-drift-backups/)")
    ap.add_argument("--restore", metavar="FILE", help="put a backup back onto the FM-1 (it restarts)")
    ap.add_argument("--no-backup", action="store_true", help="install without saving a backup first")
    a = ap.parse_args(argv)
    if sum([bool(a.info), bool(a.package), a.backup is not None, bool(a.restore)]) != 1:
        ap.error("give a PACKAGE.fwsc, --info, --backup or --restore")
    try:
        if backend is None:
            try:
                backend = MidoBackend()
            except ImportError:
                raise InstallError("usage", "needs mido and python-rtmidi (pip install mido python-rtmidi)")
        return run(a, backend, out, ask)
    except InstallError as e:
        print(f"error: {e}", file=sys.stderr)
        return EXIT.get(e.code, 1)
    except KeyboardInterrupt:
        print("\ninterrupted: if the FM-1 is in update mode, run the install again to finish", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
