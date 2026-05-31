# Challenge 02 - Checksummed Record Stream

A 64-byte working buffer is mutated by a sequence of checksummed records.
The bug is somewhere in the record dispatcher and is *not* the same flavour
as Challenge 01.

## Stream layout

```
"VM01"  : 4 bytes              magic
nrec    : uint32 LE            record count (<= 4096)
records : nrec * record
```

Each record:

```
type:1   len:2 LE   data:len   sum:1
                                 ^
                                 sum = type ^ lo(len) ^ hi(len) ^ XOR(data...)
```

| type | name   | semantics                                     |
|------|--------|-----------------------------------------------|
| 0x01 | NOP    | does nothing                                  |
| 0x02 | STORE  | copy data into 64-byte buffer (len <= 64)     |
| 0x03 | CONCAT | append data to the 64-byte buffer             |
| 0x04 | RESET  | zero the buffer                               |
| 0x05 | PRINT  | NUL-terminate and `printf("%s\n", buf)`       |

## Build

```bash
cd src
make
```

Produces `vm`, `vm.afl`, `vm.cmplog`, `vm.asan`, and `vm.persistent`.

## Why this one is harder than C01

There is no single, monolithic checksum to defeat - **every record carries
its own**. Plain `afl-fuzz` will keep mutating away from valid records and
losing them. The intended path involves writing a harness, or at minimum a
custom mutator, that re-stamps each record's XOR byte after mutation. The
provided `harness_persistent.c` is a head-start.

## What you should hand in

The same artifacts as Challenge 01, plus:

- The harness or custom mutator you wrote, with a brief description of how
  it preserves record framing under mutation.
- A back-of-envelope estimate (with numbers) of how many test executions
  per second you sustained, with and without persistent mode.

## Things to think about (no full spoilers)

- Persistent mode (`__AFL_LOOP`) is essentially mandatory if you want
  realistic throughput on a 64-byte working buffer.
- The "right" abstraction to fuzz here is *records*, not bytes. Look up
  AFL++'s "custom mutator" API (`AFL_CUSTOM_MUTATOR_LIBRARY`) and consider
  whether a structured mutator pays for itself.
- The bug is reachable from a perfectly legal byte stream - no need to
  break the framing to trigger it.
- ASAN drastically lowers the bar for *detecting* the corruption; you'll
  still need a strategy for *reaching* it.
