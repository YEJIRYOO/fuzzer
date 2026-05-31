#!/usr/bin/env python3
"""Seed generator for Challenge 02 - stream of XOR-checksummed records."""
import os
import struct
import sys

OUT = os.path.join(os.path.dirname(__file__), "seeds")
os.makedirs(OUT, exist_ok=True)


def record(rtype: int, data: bytes) -> bytes:
    if len(data) > 0xFFFF:
        raise ValueError("record too long")
    header = bytes([rtype]) + struct.pack("<H", len(data))
    body = header + data
    sum_byte = 0
    for b in body:
        sum_byte ^= b
    return body + bytes([sum_byte])


def stream(records):
    body = b"".join(records)
    return b"VM01" + struct.pack("<I", len(records)) + body


seeds = {
    "noop.bin":   stream([record(0x01, b"")]),
    "store.bin":  stream([record(0x02, b"hello"),
                          record(0x05, b"")]),
    "store_concat.bin": stream([record(0x02, b"hi"),
                                record(0x03, b" there"),
                                record(0x05, b"")]),
    "reset.bin":  stream([record(0x02, b"foo"),
                          record(0x04, b""),
                          record(0x05, b"")]),
}

for name, blob in seeds.items():
    with open(os.path.join(OUT, name), "wb") as f:
        f.write(blob)
    print(f"wrote {name} ({len(blob)} bytes)", file=sys.stderr)
