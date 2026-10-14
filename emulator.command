#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The browser emulator, started by hand: double-click this file in Finder (or run ./emulator.command).
# On main it first pulls what's been merged on GitHub; then it rebuilds the emulator from the code checked
# out, serves it at http://localhost:8765 and opens it.
# Close this window (or press Ctrl-C) to stop it.
cd "$(dirname "$0")" || exit 1
export PATH="$PWD/.venv/bin:$PATH"
# On main, bring in what's been merged on GitHub first (quietly; offline or on a branch, build as it is)
if [ "$(git rev-parse --abbrev-ref HEAD 2>/dev/null)" = main ]; then
    if git pull -q --ff-only 2>/dev/null; then
        echo "Up to date with GitHub: $(git log --oneline -1)"
    else
        echo "Couldn't update from GitHub (offline, or local changes); building the code as it is."
    fi
else
    echo "On branch $(git rev-parse --abbrev-ref HEAD), not main: building it as it is, without updating."
fi
if ! sh web/emu/build.sh; then
    echo "The emulator didn't build (see above). Press Return to close."
    read -r _
    exit 1
fi
(sleep 1; open "http://localhost:8765/") &
echo
echo "Niagara Drift emulator: http://localhost:8765  (close this window to stop it)"
exec python3 -m http.server 8765 -d build/emu
