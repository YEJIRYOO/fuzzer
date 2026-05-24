#!/usr/bin/env bash
# Build the LLVM pass and all three target fuzzers.
#
# Note: this requires the student's fuzzer.c TODOs to be implemented.
# Until they are, expect the build to succeed but the fuzzer to terminate
# immediately on launch (queue empty / no new coverage).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
"$ROOT/scripts/build_pass.sh"

for t in 01_magic 02_branches 03_state; do
    echo
    echo "=========================================================="
    echo " building target: $t"
    echo "=========================================================="
    make -C "$ROOT/fuzzer" TARGET="$t" -j"$(nproc)"
done

echo
echo "[+] done.  Run e.g.:  ~/scripts/run.sh 01_magic"
