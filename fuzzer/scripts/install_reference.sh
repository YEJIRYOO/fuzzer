#!/usr/bin/env bash
# Instructor helper: drop the reference solutions on top of the student
# skeleton.  Useful for verifying the lab end-to-end before distribution.
#
#   scripts/install_reference.sh        # install
#   scripts/install_reference.sh undo   # restore skeletons from git
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
case "${1:-install}" in
    install)
        cp "$ROOT/reference/cov_rt.c"  "$ROOT/runtime/cov_rt.c"
        cp "$ROOT/reference/CovPass.cpp" "$ROOT/pass/CovPass.cpp"
        # fuzzer.c is replaced wholesale - drop the reference TODOs into
        # the student copy by patching only the four functions.
        python3 "$ROOT/scripts/_apply_fuzzer_ref.py"
        echo "[+] reference solutions installed"
        ;;
    undo)
        if command -v git >/dev/null 2>&1 && [ -d "$ROOT/.git" ]; then
            git -C "$ROOT" checkout -- runtime/cov_rt.c pass/CovPass.cpp fuzzer/fuzzer.c
            echo "[+] reverted via git"
        else
            echo "[-] no git checkout available; restore manually" >&2
            exit 1
        fi
        ;;
    *)
        echo "usage: $0 [install|undo]"; exit 64;;
esac
