# Build the student fuzzer for one target at a time.
#
#   make TARGET=01_magic     # or 02_branches, 03_state
#
# Outputs:
#   build/$(TARGET)/fuzzer       - the fuzzer binary, statically linked
#                                  against the instrumented target
#
# Layers:
#   1) compile cov_rt.c with regular cc                  -> cov_rt.o
#   2) compile target.c with clang + the LLVM pass plugin -> target.o
#   3) compile fuzzer.c with regular cc                  -> fuzzer.o
#   4) link them all together                             -> fuzzer

TARGET   ?= 01_magic

ROOT     := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/..
RUNTIME  := $(ROOT)/runtime
PASS     := $(ROOT)/pass/build/CovPass.so
TARGETSRC:= $(ROOT)/targets/$(TARGET)/target.c
BUILD    := $(ROOT)/fuzzer/build/$(TARGET)

CC       ?= cc
CLANG    ?= clang

CFLAGS   ?= -O1 -g -fno-omit-frame-pointer -Wall -Wextra
COVCFLAGS:= $(CFLAGS) -fpass-plugin=$(PASS) -Xclang -fno-discard-value-names

.PHONY: all clean
all: $(BUILD)/fuzzer

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/cov_rt.o: $(RUNTIME)/cov_rt.c $(RUNTIME)/cov_rt.h | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(BUILD)/target.o: $(TARGETSRC) $(PASS) | $(BUILD)
	$(CLANG) $(COVCFLAGS) -c $< -o $@

$(BUILD)/fuzzer.o: $(ROOT)/fuzzer/fuzzer.c $(RUNTIME)/cov_rt.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/fuzzer: $(BUILD)/fuzzer.o $(BUILD)/target.o $(BUILD)/cov_rt.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD)
