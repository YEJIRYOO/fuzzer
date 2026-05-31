# Challenge 01 - CRC-Guarded Container Parser

The binary `parser` reads a small "FZ01" container off stdin (or argv[1]) and
dispatches to one of three handlers depending on the leading 4-byte tag inside
the payload. Somewhere in that pipeline there is at least one memory-safety
bug that triggers a crash on a carefully crafted input.

## File format (little-endian)

```
+---------+---------+---------+---------+--------------------+
| magic   | version | pay_len | crc32   | payload (pay_len)  |
| 4 bytes | 2 bytes | 4 bytes | 4 bytes | ...                |
+---------+---------+---------+---------+--------------------+
   "FZ01"   0x0001    LE u32    LE u32
```

`crc32` is the standard IEEE 802.3 (zlib) CRC over `payload`.

## Build

```bash
cd src
make            # builds parser, parser.afl, parser.cmplog, parser.asan, parser.nocrc
```

Outputs live next to the source. The `parser.nocrc` target compiles with
`-DDISABLE_CRC` and is provided so you can compare guarded vs unguarded
behaviour during your write-up.

## Sanity-check the seeds

```bash
./parser ../seeds/admin_short.bin    # prints [admin] name="root"
./parser ../seeds/user.bin           # prints [user]  uid=1000
./parser ../seeds/note.bin           # prints [note]  hello
```

## What you should hand in

1. A short technical write-up (max 2 pages) covering:
   - What the bug is, in C-source terms (file + line + class of bug).
   - Why naive `afl-fuzz -i seeds -o out -- ./parser.afl` is or is not
     enough to find it within a few minutes of fuzzing.
   - The technique you actually used to find it. Describe the tradeoffs.
2. The crashing input (`crash.bin`) and the AFL++ crash dir layout.
3. A patch that removes the bug *without* loosening the format.

## Things to think about (no full spoilers)

- AFL++ has a built-in mechanism for solving multi-byte comparisons that
  block coverage. Look at `afl-cc --help` and `afl-fuzz --help` for the flag
  whose name starts with `-c`. Pair that with the `*.cmplog` binary the
  Makefile produces.
- A dictionary (`-x`) covering the magic and the inner tags ("ADMN",
  "USER", "NOTE") is almost free coverage.
- If you write a small custom harness (`__AFL_FUZZ_INIT`, persistent mode),
  you can recompute the CRC inside the harness and feed only the *payload*
  to the fuzzer. This is the most realistic technique and the one the
  course rubric rewards the most.
- The "easy mode" `parser.nocrc` binary is for comparison only; submitting
  a crash found purely against `parser.nocrc` will not earn full credit.

Good luck.
