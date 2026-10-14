# Building Niagara Drift

The file names below are still FoMni's (`omni.*`, `omni_host`); the engine inside is the drone's.
Renaming them is a separate change.

## Notes for macOS 26/27 on Apple silicon

Found while setting up Niagara Drift (Apple M3, macOS 27, Xcode 26.5):

- **Fonts need Pillow with libraqm.** Apple's Python 3.9 Pillow can't shape text, so
  `tools/gen_font.py` fails with "setting text direction ... not supported without libraqm". Use a
  venv with a current Pillow wheel (it bundles libraqm) and put it first on `PATH`:

  ```
  /opt/homebrew/bin/python3.12 -m venv .venv && .venv/bin/pip install pillow fonttools
  export PATH="$PWD/.venv/bin:$PATH"
  ```

- **The host simulator fails to link** ("unknown architecture arm64e.x1-macos" in `libSystem.B.tbd`)
  when the Command Line Tools SDK is newer than Xcode's linker. Point the compiler at Xcode's SDK:

  ```
  export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
  ```

- **The browser emulator needs Emscripten:** `brew install emscripten`.

With those three, `tests/run_tests.sh` and `web/emu/build.sh` pass. `./build.sh` then works as
documented below, with Docker Desktop running; under Rosetta it takes about 15 seconds.

## The build

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `omni.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `omni.fwsc` | the installable package (app + loader) |

## Prerequisites (macOS)

- Python 3 with Pillow: `pip3 install Pillow`
- Docker Desktop. The JieLi toolchain is Linux x86-64 only; the build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder Docker can share, e.g. under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).

On Linux x86-64 the toolchain runs natively and Docker is not needed.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations
(`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

`./build.sh --release 0.1` makes a release build; the package is `build/niagara-drift-0.1.fwsc`.

On macOS with podman instead of Docker, put a `docker` script that runs `exec podman "$@"` first
on your PATH. `OM_JOBS` (default 4) limits parallel compiles: a podman machine drops
connections when many containers start at once.

Build option: `OM_CDC=1` adds Felucca's USB serial function (off by default: one plain
MIDI interface).

The build also generates `build/gen/` (the font). It fails if a soft-double routine is linked
(a `double` crept in) and checks the image, RAM and pool sizes.

## Tests

```
tests/run_tests.sh
```

Runs, on the build machine: the maths library against libm; the drone engine (`tests/host/drone_test.c`:
silence, fades, pitch, glide, voices, the filter at its corners), its CPU cost (`drone_cost.c`) and its
listening renders (`build/drone/*.wav`); flash storage; Felucca's update-path tests against
`build/omni.fwsc`; the browser build; and the whole app in the simulator (`tests/scenarios/*.omni`),
with its screenshots and audio in `build/scenarios/`.

`host/build_host.sh` builds the simulator alone; `build/host/omni_host SCRIPT OUTDIR` runs one
script (the command list is at the top of `host/omni_host.c`).

## Install

From the command line (needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/omni.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

Installing firmware is at your own risk. Hold OCT− and OCT+ for 5 seconds for Felucca's update
mode. If the FM-1 no longer starts but reaches the chip's update mode (4C4A:8057 on USB),
`tools/fm1_rescue.sh` puts stock firmware back from a Mac; otherwise recovery needs
[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Publishing a release

1. `./build.sh --release X.Y-beta` (for example 0.1-beta): `build/niagara-drift-X.Y-beta.fwsc`, identity
   FM-1_8XXYYZZ (what the installer checks, and what the FM-1 reports after an install).
2. `tools/publish_pages.sh X.Y-beta`: builds the emulator with that version, then the site with
   `tools/make_pages.py` (the emulator page as the front page, with Install / Download / Full manual / Source;
   the web installer at `install/`; the firmware at `firmware/`) and pushes it to the `gh-pages` branch,
   which GitHub Pages serves at <https://evanyerburgh.github.io/niagara-drift/>.
3. `gh release create vX.Y-beta build/niagara-drift-X.Y-beta.fwsc --prerelease` for the download on GitHub.
