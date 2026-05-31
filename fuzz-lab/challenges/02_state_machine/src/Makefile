# Challenge 02 - Stream of checksummed records
#
# Targets mirror Challenge 01 plus a `vm.persistent` build that uses
# AFL_LOOP() so students can learn persistent-mode harnessing.
CC      ?= cc
AFLCC   ?= afl-clang-fast
CFLAGS  ?= -O2 -g -fno-omit-frame-pointer
WARN    := -Wall -Wextra

.PHONY: all clean
all: vm vm.afl vm.cmplog vm.asan vm.persistent

vm: vm.c
	$(CC) $(CFLAGS) $(WARN) -o $@ $<

vm.afl: vm.c
	$(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

vm.cmplog: vm.c
	AFL_LLVM_CMPLOG=1 $(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

vm.asan: vm.c
	AFL_USE_ASAN=1 $(AFLCC) $(CFLAGS) $(WARN) -o $@ $<

# Persistent-mode harness lives in harness_persistent.c and #includes vm.c
# logic by linking against a slightly refactored version. Students will
# typically *write* this themselves; we ship a stub for reference.
vm.persistent: vm.c harness_persistent.c
	$(AFLCC) $(CFLAGS) $(WARN) -DPERSISTENT_HARNESS -o $@ harness_persistent.c

clean:
	rm -f vm vm.afl vm.cmplog vm.asan vm.persistent
