#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Niagara Drift host tests: the maths, the platform pieces kept from Felucca / X0X, the instrument, and the
# whole firmware app in the simulator (tests/scenarios/*.omni: UI, the drone, persistence), with
# screenshots and audio in build/scenarios/.
#   tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
FAIL=0
OUT=build/host
mkdir -p "$OUT" build/scenarios
run() {
    name="$1"; shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        echo "  ok   $name"
    else
        echo "  FAIL $name (see $OUT/$name.log)"
        tail -15 "$OUT/$name.log" | sed 's/^/       /'
        FAIL=1
    fi
}
run fastmath sh -c "$CC -O2 -ffp-contract=off -Wall -Wextra -Werror -o $OUT/fastmath_test tests/host/fastmath_test.c -lm && $OUT/fastmath_test"
run encoder sh -c "$CC -O2 -w -Ifirmware/hal -o $OUT/encoder_test tests/host/encoder_test.c && $OUT/encoder_test"
run uac sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/uac_test tests/host/uac_test.c -lm && $OUT/uac_test"
run trs sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/trs_test tests/host/trs_test.c && $OUT/trs_test"
# the project's format change from 0.1: a 0.1 project loads, its scenes kept, the Colour converted
run project sh -c "$CC -O2 -std=c99 -Wall -Wextra -Werror -DOM_HOST -o $OUT/project_test tests/host/project_test.c \
    firmware/src/dsp/drone.c -lm && $OUT/project_test"
# the key pressure sensing (MONK-1's sense.c, from Velo), on synthetic press curves
run sense sh -c "$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wdouble-promotion -Werror \
    -o $OUT/sense_test tests/host/sense_test.c -lm && $OUT/sense_test"
run drone sh -c "$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wdouble-promotion -Werror -DOM_HOST -Ifirmware/src/dsp \
    -o $OUT/drone_test tests/host/drone_test.c firmware/src/dsp/drone.c && $OUT/drone_test"
# cross-FM keeps both drones' pitch (built with the engine: it counts the oscillators' phases)
run xfm sh -c "$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Werror -Wno-unused-function -DOM_HOST -Ifirmware/src/dsp \
    -o $OUT/xfm_test tests/host/xfm_test.c -lm && $OUT/xfm_test"
# what the drone engine costs (host instructions a sample, macOS; a device estimate): fails over its budget
run drone-cost sh -c "$CC -O2 -ffp-contract=off -std=c99 -D_DARWIN_C_SOURCE -Wall -Wextra -Werror -DOM_HOST -Ifirmware/src/dsp \
    -o $OUT/drone_cost tests/host/drone_cost.c firmware/src/dsp/drone.c && $OUT/drone_cost"
# the drone engine's scenes, to listen to: build/drone/*.wav
mkdir -p build/drone
run drone-render sh -c "$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-missing-field-initializers -Werror -DOM_HOST \
    -Ifirmware/src/dsp -o $OUT/drone_render host/drone_render.c firmware/src/dsp/drone.c && $OUT/drone_render build/drone"
run storage sh -c "$CC -O2 -o $OUT/storage_test tests/storage_test.c && $OUT/storage_test"
# the update path, against the firmware package (Felucca's tests; needs ./build.sh)
if [ -f build/omni.fwsc ]; then
    run ota-entry sh -c "$CC -o $OUT/ota_test tests/ota_test.c && $OUT/ota_test build/omni.fwsc"
    if [ -z "${AC79_SDK:-}" ]; then
        echo "  skip update-loader (needs AC79_SDK, as the build)"
    else
    run update-loader sh -c "head -c 100000 build/omni.bin > $OUT/old_app.bin && \
        python3 tools/fm1pkg_make.py $OUT/old_app.bin build/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -o $OUT/ldr_test tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc build/omni.fwsc"
    fi
    run installer python3 tests/install_test.py
else
    echo "  skip update-path tests (no build/omni.fwsc: run ./build.sh)"
fi
run host-build sh host/build_host.sh
if command -v emcc >/dev/null 2>&1; then
    run emu sh -c "sh web/emu/build.sh >/dev/null && node tests/host/emu_test.mjs build/emu/omni.wasm"
fi
for s in tests/scenarios/*.omni; do
    n=$(basename "$s" .omni)
    mkdir -p "build/scenarios/$n"
    run "scenario-$n" build/host/omni_host "$s" "build/scenarios/$n"
done
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
