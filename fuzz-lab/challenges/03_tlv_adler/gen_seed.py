#!/usr/bin/env python3
"""Seed generator for Challenge 03 - Adler32-guarded TLV."""
import os
import struct
import sys

OUT = os.path.join(os.path.dirname(__file__), "seeds")
os.makedirs(OUT, exist_ok=True)


def adler32(data: bytes) -> int:
    a, b = 1, 0
    for byte in data:
        a = (a + byte) % 65521
        b = (b + a)    % 65521
    return ((b << 16) | a) & 0xFFFFFFFF


def tlv(tag: int, value: bytes) -> bytes:
    return bytes([tag]) + struct.pack("<H", len(value)) + value


def file(records):
    body = b"".join(records)
    return b"TLV1" + struct.pack("<II", len(body), adler32(body)) + body


seeds = {
    "name.bin":      file([tlv(0x01, b"hello")]),
    "load.bin":      file([tlv(0x02, bytes([0xAA] * 32))]),
    "load_dup.bin":  file([tlv(0x02, bytes([0xAA] * 32)),
                           tlv(0x20, b"")]),
    "show_basic.bin":file([tlv(0x02, bytes([0xAA] * 16)),
                           tlv(0x20, b""),
                           tlv(0x22, b"")]),
    "drop_only.bin": file([tlv(0x02, bytes([0xAA] * 16)),
                           tlv(0x20, b""),
                           tlv(0x21, b"")]),
}

for name, blob in seeds.items():
    with open(os.path.join(OUT, name), "wb") as f:
        f.write(blob)
    print(f"wrote {name} ({len(blob)} bytes)", file=sys.stderr)
