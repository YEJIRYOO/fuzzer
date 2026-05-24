# mini-fuzzer-lab

Build your own coverage-guided fuzzer.

This is a sister lab to `fuzz-lab`. Where `fuzz-lab` exercises *using* AFL++,
this one asks you to **implement** the core ideas behind it: an LLVM pass
that instruments every basic block, a coverage runtime that maintains the
bitmap, and a fuzzer driver that mutates inputs and tracks new coverage.

## 1. Build & enter the container

```bash
docker build -t mini-fuzzer-lab .
docker run --rm -it mini-fuzzer-lab
```

The image ships with `clang-14`, `llvm-14-dev`, `cmake`, `ninja-build`,
`gdb`, and the assignment scaffold under `~/`. The LLVM pass build tree
is pre-cached so the first `cmake` is fast.

## 2. What you have to write

There are exactly **three** files you'll edit. Everything else (Makefile,
crash detection, status line, seed loader, *the mutator*, build scripts)
is done. **Don't modify `mutate()` in fuzzer.c — it's a black box for
this assignment, and the grader assumes the provided one.**

| File                       | What goes there                                          |
|----------------------------|----------------------------------------------------------|
| `runtime/cov_rt.c`         | `__cov_visit`, `cov_reset`, `cov_classify_count` (3 TODOs) |
| `pass/CovPass.cpp`         | The IR-walk that inserts `__cov_visit(rand_id)` calls (1 TODO) |
| `fuzzer/fuzzer.c`          | `queue_add`, `queue_pick`, `has_new_coverage` (3 TODOs) |

Each file's TODOs are documented inline. Read them.

## 3. Architecture

```
+---------------------+       +---------------------+
|   target.c          |--+    |   fuzzer.c          |
|   (your bug-bait)   |  |    |   (your driver)     |
+---------------------+  |    +----------+----------+
                         |               |
        clang -fpass-plugin=CovPass.so   |
                         |               |
                         v               v
                   +--------------+  +--------------+
                   |  target.o    |  |  fuzzer.o    |
                   |  (per-BB     |  |              |
                   |  __cov_visit |  |              |
                   |   calls)     |  |              |
                   +------+-------+  +------+-------+
                          |                 |
                          +---->  link  <---+
                                   |
                                   v
                          +-----------------+
                          | runtime/cov_rt.o|
                          | (bitmap + helpers)
                          +-----------------+
                                   |
                                   v
                          fuzzer/build/<target>/fuzzer
                          (in-process: target is called as a function)
```

The model is `libFuzzer`-style: the target exposes `int FuzzInput(buf, len)`
and your fuzzer calls it directly inside the same process. The instrumentation
writes to a global bitmap that the fuzzer reads after each call. Crashes are
caught with `sigsetjmp`/signal handlers (already implemented).

## 4. Workflow inside the container

```bash
# 1) build the LLVM pass once
~/scripts/build_pass.sh

# 2) edit the three files (use vim or nano)
nano ~/runtime/cov_rt.c
nano ~/pass/CovPass.cpp
nano ~/fuzzer/fuzzer.c

# 3) build the fuzzer for one target
make -C ~/fuzzer TARGET=01_magic

# 4) run it
~/scripts/run.sh 01_magic

# (or all-at-once after editing)
~/scripts/build_all.sh
~/scripts/run.sh 02_branches -T 60
```

## 5. Targets

| Target          | Difficulty | Bug                                   |
|-----------------|------------|---------------------------------------|
| `01_magic`      | Easy       | "FUZZ" prefix + 16-byte stack overflow |
| `02_branches`   | Easy       | 4-deep nested byte compares + NULL deref |
| `03_state`      | Medium     | 4-state machine, deterministic crash via specific sequence |

Each `targets/<name>/README.md` explains why coverage matters and what the
target is teaching.

## 6. Exiting the container

`exit` (or Ctrl+D). Crash files inside the container are deleted with it
since we use `--rm`. To preserve them, mount a host directory:

```bash
docker run --rm -it -v "$PWD/findings:/home/student/findings" mini-fuzzer-lab
```

