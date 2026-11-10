#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The screen reader's audit (Charles Vestal's fm1-reader, next to this repository: ../fm1-reader): every page, knob
# value and screen (the host's "speakdump"), every banner the scenarios show (OMNI_SAID_LOG), put in words with this
# build's words file and checked against fm1-reader's rules.
#   sh tools/reader_audit.sh [--spoken | --all]     (--spoken: as it will be heard)
set -e
cd "$(dirname "$0")/.."
R="${FM1_READER:-../fm1-reader}"
[ -f "$R/tools/audit.mjs" ] || { echo "reader_audit: no fm1-reader at $R (git clone https://github.com/charlesvestal/fm1-reader $R)"; exit 2; }
sh host/build_host.sh >/dev/null
mkdir -p build/sr build/sr/scen
printf 'notips\nspeakdump\n' > build/sr/dump.omni
build/host/omni_host build/sr/dump.omni build/sr | grep -v '^host' | sort -u > build/sr/dump.tsv
rm -f build/sr/said.tsv
for f in tests/scenarios/*.omni tests/golden/*.omni; do
    OMNI_SAID_LOG="$PWD/build/sr/said.tsv" build/host/omni_host "$f" build/sr/scen >/dev/null 2>&1 || true
done
sort -u build/sr/said.tsv | cat build/sr/dump.tsv - > build/sr/all.tsv
W=$(sed -n 's/.*\/\([0-9a-f]*\.json\)".*/\1/p' build/gen/ann_words.h)
node "$R/tools/audit.mjs" build/sr/all.tsv --words "web/words/$W" --allow tools/reader-allow.txt "$@"
