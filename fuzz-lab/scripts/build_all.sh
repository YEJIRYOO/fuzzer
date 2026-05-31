#!/usr/bin/env bash
#
# Build every challenge target and regenerate the seed corpora.
# Idempotent - safe to re-run.
set -euo pipefail

ROOT="${1:-$HOME/challenges}"

echo "[*] Building challenges under: $ROOT"

for d in "$ROOT"/0*_*/; do
    [ -d "$d" ] || continue
    name=$(basename "$d")
    echo
    echo "============================================================"
    echo " $name"
    echo "============================================================"

    if [ -f "$d/gen_seed.py" ]; then
        echo "[*] Generating seeds..."
        # Wipe stale seeds first, then regenerate the canonical set
        rm -f "$d"/seeds/*.bin 2>/dev/null || true
        python3 "$d/gen_seed.py"
    fi

    if [ -f "$d/src/Makefile" ]; then
        echo "[*] make -C $d/src"
        make -C "$d/src" -j"$(nproc)"
    fi
done

echo
echo "[+] Done."
