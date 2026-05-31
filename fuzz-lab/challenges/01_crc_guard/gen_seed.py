#!/usr/bin/env python3
"""Generate well-formed seed inputs for Challenge 01.

These seeds are deliberately *boring*: they take the easy code paths so the
fuzzer has something valid to mutate. Finding the bug is your job.
"""
import os
import struct
import zlib
import sys

OUT = os.path.join(os.path.dirname(__file__), "seeds")
os.makedirs(OUT, exist_ok=True)


def pack(payload: bytes) -> bytes:
    crc = zlib.crc32(payload) & 0xFFFFFFFF
    return b"FZ01" + struct.pack("<HII", 1, len(payload), crc) + payload


seeds = {
    "user.bin": b"USER" + struct.pack("<I", 1000),
    "note.bin": b"NOTE" + b"hello",
    "admin_short.bin": b"ADMN" + bytes([4]) + b"root",
}

for name, payload in seeds.items():
    with open(os.path.join(OUT, name), "wb") as f:
        f.write(pack(payload))
    print(f"wrote {name} ({len(payload)} byte payload)", file=sys.stderr)
