#!/usr/bin/env bash
# Light-weight entrypoint that prints a banner and drops to a shell.
set -e
cat <<'BANNER'

  ----------------------------------------------------------
   fuzz-lab : System Security fuzzing playground (AFL++)
  ----------------------------------------------------------
   Challenges    : ~/challenges/{01,02,03}_*
   Build all     : ~/scripts/build_all.sh
   Start fuzzing : ~/scripts/run_fuzz.sh ~/challenges/01_crc_guard
   Read first    : ~/README.md

BANNER
exec "$@"
