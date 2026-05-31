# Challenge 03 - Adler32 + TLV

A small TLV record format wrapped in an Adler32-protected envelope.
Internally the program maintains a 1024-byte working buffer, a 64-byte name
slot, and a four-entry function-pointer table for the EXEC tag. The bug is
*not* a buffer overflow.

## File layout (little-endian)

```
+---------+---------+---------+----------------------------+
| "TLV1"  | body_len| adler32 | body (body_len)            |
+---------+---------+---------+----------------------------+
```

`adler32` is computed over `body[]` only.

Records inside `body[]`:

| tag  | meaning                                                          |
|------|------------------------------------------------------------------|
| 0x01 | NAME : value (<=64) copied into a name slot                      |
| 0x02 | LOAD : value (<=1024) copied into the working buffer             |
| 0x20 | DUP  : `malloc()` a private copy of the working buffer           |
| 0x21 | DROP : `free()` the private copy                                 |
| 0x22 | SHOW : touch byte 0 of the private copy and print it             |

## Build

```bash
cd src
make
```

Produces `tlv`, `tlv.afl`, `tlv.cmplog`, `tlv.asan`, and `tlv.noadler`.

## Why Adler32 is harder than CRC32 for AFL

A CRC32 mismatch flips 32 bits of a state-machine output and AFL's CmpLog
can sometimes infer the comparison directly. Adler32 is computed
incrementally and has weaker locality, so even with `-c` you usually need
either a wrapper that recomputes the checksum on the fly or a custom
mutator that keeps it valid.

## What you should hand in

The deliverables from C01/C02, plus:

- A short comparison: how many executions/sec did you get
  - against `tlv.afl`,
  - against `tlv.afl -c tlv.cmplog`,
  - against a custom harness that fixes Adler32 inside the testcase loop?
- The crashing input *and* its 64-bit equivalent if you ran ASAN
  (the ASAN report goes in your write-up).

## Things to think about (no full spoilers)

- The crash is not a buffer overflow. ASAN will name the bug class for you
  the moment you hit it - run `tlv.asan` against any input that gets past
  the Adler32 check.
- Reaching the bug needs a particular *sequence* of records, not a
  particular byte pattern. Coverage-driven mutation will find the right
  records eventually; the question is how to keep the file checksummed.
- Don't waste real fuzzing budget on Adler32. Patch it out (`tlv.noadler`)
  to *develop* a strategy, then prove that your final approach works on
  the guarded `tlv.afl` binary too.
- AFL++'s post-process callback (`AFL_POST_LIBRARY`) is the cleanest way
  to keep the checksum valid; LibAFL's `BytesInput` with a wrapping stage
  also works.
