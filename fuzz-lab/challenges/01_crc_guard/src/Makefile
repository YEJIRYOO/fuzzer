# Challenge 01 - CRC-guarded container parser
#
# Targets:
#   parser            : reference build (gcc, no instrumentation) - sanity tests
#   parser.afl        : AFL++ instrumented build for plain afl-fuzz
#   parser.cmplog     : AFL++ CmpLog target for `-c ./parser.cmplog`
#   parser.asan       : AFL++ instrumented + AddressSanitizer
#   parser.nocrc      : AFL++ instrumented, CRC check compiled out
#                       (intentionally easy - so students see the
#                        difference the guard makes)

CC      ?= cc
AFLCC   ?= afl-clang-fast
CFLAGS  ?= -O2 -g -fno-omit-frame-pointer
WARN    := -Wall -Wextra

.PHONY: all clean
all: parser parser.afl parser.cmplog parser.asan parser.nocrc

parser: parser.c
	$(CC) $(CFLAGS) $(WARN) -o $@ $<

parser.afl: parser.c
	$(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

parser.cmplog: parser.c
	AFL_LLVM_CMPLOG=1 $(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

parser.asan: parser.c
	AFL_USE_ASAN=1 $(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

parser.nocrc: parser.c
	$(AFLCC) $(CFLAGS) $(WARN) -DDISABLE_CRC -o $@ $<

clean:
	rm -f parser parser.afl parser.cmplog parser.asan parser.nocrc
