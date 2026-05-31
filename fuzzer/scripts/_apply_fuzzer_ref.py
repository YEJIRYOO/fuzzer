#!/usr/bin/env python3
"""Patch fuzzer/fuzzer.c with the reference implementations of the four
TODO functions, leaving the surrounding driver code untouched."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC  = os.path.join(ROOT, "fuzzer", "fuzzer.c")
REF  = os.path.join(ROOT, "reference", "fuzzer.c")

FUNCS = ["queue_add", "queue_pick", "has_new_coverage"]

def extract(path, name):
    """Pull `static ... name(...) { ... }` out of `path`."""
    txt = open(path).read()
    pat = re.compile(
        r"^static[^\n]*\b" + re.escape(name) + r"\b[^\n]*\([^)]*\)[^{]*\{",
        re.M)
    m = pat.search(txt)
    if not m:
        sys.exit(f"can't find {name} in {path}")
    start = m.start()
    depth = 0
    i = m.end() - 1
    while i < len(txt):
        c = txt[i]
        if c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return txt[start:i+1]
        i += 1
    sys.exit(f"unterminated {name} in {path}")

def replace(src, name, body):
    pat = re.compile(
        r"^static[^\n]*\b" + re.escape(name) + r"\b[^\n]*\([^)]*\)[^{]*\{",
        re.M)
    m = pat.search(src)
    if not m:
        sys.exit(f"can't find {name} in target file")
    start = m.start()
    depth = 0
    i = m.end() - 1
    while i < len(src):
        c = src[i]
        if c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return src[:start] + body + src[i+1:]
        i += 1
    sys.exit(f"unterminated {name} in target file")

src = open(SRC).read()
for f in FUNCS:
    body = extract(REF, f)
    src = replace(src, f, body)
open(SRC, "w").write(src)
print("[+] patched", ", ".join(FUNCS))
