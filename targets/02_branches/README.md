# Target 02 - nested branches (easy)

## Bug class

NULL pointer dereference, reachable only after passing four nested byte
comparisons (`A`, `B`, `C`, `D` at offsets 0..3).

## Why coverage helps here

Each correct character lights up a new edge in the bitmap (the `if (...)`
true-branch). Blind random mutation takes ~2^32 attempts; coverage-guided
fuzzing follows the breadcrumbs and arrives in seconds.

## Expected behaviour

This is the litmus test: if your fuzzer cannot crash this target, the
"new coverage means save the input" loop is wrong. Re-check
`has_new_coverage()` and your mutator's ability to flip individual
bytes.
