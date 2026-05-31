#!/usr/bin/env bash
#
# Convenience launcher.
#
#   run_fuzz.sh <challenge_dir> [target_binary] [extra afl-fuzz args...]
#
# Example:
#   run_fuzz.sh ~/challenges/01_crc_guard parser.afl
#   run_fuzz.sh ~/challenges/02_state_machine vm.persistent -- ./vm.persistent
#
# Output goes into ~/findings/<challenge_name>/ which docker-compose mounts
# back to the host so crashes survive container restarts.
#
set -euo pipefail

if [ $# -lt 1 ]; then
    cat <<EOF
Usage: $(basename "$0") <challenge_dir> [target_binary] [-- afl args...]

Examples:
  $(basename "$0") ~/challenges/01_crc_guard
  $(basename "$0") ~/challenges/01_crc_guard parser.cmplog -- -c ./parser.cmplog
  $(basename "$0") ~/challenges/02_state_machine vm.persistent
EOF
    exit 64
fi

CHAL="$(cd "$1" && pwd)"
shift
TARGET="${1:-}"
[ $# -gt 0 ] && shift

NAME="$(basename "$CHAL")"
SEEDS="$CHAL/seeds"
OUT="$HOME/findings/$NAME"
SRC="$CHAL/src"

if [ -z "$TARGET" ]; then
    # Pick the first .afl binary in src/
    TARGET="$(basename "$(ls "$SRC"/*.afl 2>/dev/null | head -n1 || true)")"
fi
if [ -z "$TARGET" ]; then
    echo "No instrumented binary found in $SRC"; exit 1
fi
if [ ! -x "$SRC/$TARGET" ]; then
    echo "$SRC/$TARGET is not executable. Did you run build_all.sh?"; exit 1
fi

mkdir -p "$OUT"

echo "[*] challenge : $NAME"
echo "[*] target    : $SRC/$TARGET"
echo "[*] seeds     : $SEEDS"
echo "[*] findings  : $OUT"
echo

cd "$SRC"
exec afl-fuzz -i "$SEEDS" -o "$OUT" "$@" -- "./$TARGET"
