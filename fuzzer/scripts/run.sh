#!/usr/bin/env bash
# Convenience launcher.
#
#   run.sh <target_name> [extra fuzzer args...]
#
#   run.sh 01_magic
#   run.sh 02_branches -T 60       # stop after 60 seconds
#   run.sh 03_state    -m 256      # cap input size at 256 bytes
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "usage: $(basename "$0") <target_name> [extra fuzzer args...]"
    echo "available targets:"
    ls -1 ~/targets/ | sed 's/^/  /'
    exit 64
fi

TARGET="$1"; shift
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

BIN="$ROOT/fuzzer/build/$TARGET/fuzzer"
SEEDS="$ROOT/targets/$TARGET/seeds"
OUT="$ROOT/findings/$TARGET/crashes"

if [ ! -x "$BIN" ]; then
    echo "[-] $BIN not built.  Run scripts/build_all.sh first." >&2
    exit 1
fi

mkdir -p "$OUT"
echo "[*] target  : $TARGET"
echo "[*] binary  : $BIN"
echo "[*] seeds   : $SEEDS"
echo "[*] crashes : $OUT"
echo

exec "$BIN" -s "$SEEDS" -o "$OUT" "$@"
