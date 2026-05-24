# Fuzzer driver

`fuzzer.c` is the skeleton you'll fill in. Read the comment block at the top
of the file, then look for the four `TODO` sections and implement them.

## Build

The Makefile builds *one target at a time* because the target source is
linked statically into the fuzzer binary (in-process model).

```bash
# from this directory
make TARGET=01_magic
make TARGET=02_branches
make TARGET=03_state
```

Outputs live under `fuzzer/build/<TARGET>/fuzzer`.

## Run

```bash
./build/01_magic/fuzzer -s ../targets/01_magic/seeds \
                       -o ../findings/01_magic/crashes
```

Optional flags:

| Flag | Meaning                           |
|------|-----------------------------------|
| `-s` | seed directory (required)         |
| `-o` | crash output directory            |
| `-T` | run for N seconds then exit       |
| `-m` | max input size in bytes (default 4096) |

A status line refreshes every ~1k executions on stderr; press Ctrl+C to
stop.
