# Target 01 - magic + overflow (easy)

## Bug class

Stack buffer overflow. After the 4-byte magic `FUZZ`, the rest of the input
is `memcpy`'d into a 16-byte stack array with no length check.

## Why coverage helps here

Each correct magic byte changes the branch taken at one of the four
`if (data[i] != ...)` checks. A coverage-guided fuzzer sees a new bitmap
bit each time, so it keeps inputs that got one byte further. Without
coverage, finding `FUZZ` by random mutation would take ~2^32 tries.

## Expected behaviour

A working fuzzer (with a sane mutator) should crash this target in well
under a minute. If yours can't, your bitmap diff or your mutator is
broken.

## Sanity-check from the shell

```bash
printf 'FUZZX' | ./your_fuzzer ./targets/01_magic/build/target_01
```
