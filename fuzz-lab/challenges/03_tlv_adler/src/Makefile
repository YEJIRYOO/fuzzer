# Challenge 03 - Adler32-guarded TLV file
CC      ?= cc
AFLCC   ?= afl-clang-fast
CFLAGS  ?= -O2 -g -fno-omit-frame-pointer
WARN    := -Wall -Wextra

.PHONY: all clean
all: tlv tlv.afl tlv.cmplog tlv.asan tlv.noadler

tlv: tlv.c
	$(CC) $(CFLAGS) $(WARN) -o $@ $<

tlv.afl: tlv.c
	$(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

tlv.cmplog: tlv.c
	AFL_LLVM_CMPLOG=1 $(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

tlv.asan: tlv.c
	AFL_USE_ASAN=1 $(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

tlv.noadler: tlv.c
	$(AFLCC) $(CFLAGS) $(WARN) -DDISABLE_ADLER -o $@ $<

clean:
	rm -f tlv tlv.afl tlv.cmplog tlv.asan tlv.noadler
