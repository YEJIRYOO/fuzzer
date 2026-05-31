#!/usr/bin/env bash
# Build the LLVM coverage pass (out-of-tree, LLVM 14 new-PM plugin).
#
# Produces ./pass/build/CovPass.so, which the per-target Makefile loads
# via clang -fpass-plugin=...
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/pass"

mkdir -p build
cd build

if [ ! -f build.ninja ] && [ ! -f Makefile ]; then
    cmake .. -G Ninja >/dev/null
fi

if [ -f build.ninja ]; then
    ninja
else
    make -j"$(nproc)"
fi

ls -la CovPass.so
