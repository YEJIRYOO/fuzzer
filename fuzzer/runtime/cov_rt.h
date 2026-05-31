/*
 * Coverage runtime - shared between the instrumented target and the fuzzer.
 *
 * This header is part of the assignment scaffold. You should NOT modify it;
 * the LLVM pass and the fuzzer rely on the symbol names defined here.
 */
#ifndef COV_RT_H
#define COV_RT_H

#include <stdint.h>
#include <stddef.h>

/* 64 KiB bitmap (16-bit edge IDs, classic AFL sizing). */
#define COV_MAP_SIZE_POW2 16
#define COV_MAP_SIZE      (1u << COV_MAP_SIZE_POW2)
#define COV_MAP_MASK      (COV_MAP_SIZE - 1u)

/* The shared bitmap. Each entry is a saturating 8-bit hit counter for
 * one edge (i.e. one (prev_block -> cur_block) transition). */
extern uint8_t  cov_map[COV_MAP_SIZE];

/* The thread-local "previous location" used to compute edge IDs. The
 * LLVM pass updates this on every basic-block entry. */
extern uint16_t cov_prev_loc;

/* Inserted by the LLVM pass at the start of every basic block.
 *
 *  cur_loc: a 16-bit constant assigned to the current basic block at
 *           compile time (random per BB).
 *
 * Implementation lives in cov_rt.c (your job, see TODOs).
 */
void __cov_visit(uint32_t cur_loc);

/* Zero out the bitmap and prev_loc. Called by the fuzzer between
 * test cases. */
void cov_reset(void);

/* Map an 8-bit hit count into a "bucket" so that, e.g., going from 1 hit
 * to 2 hits counts as new coverage but going from 100 to 101 does not.
 * The fuzzer uses this when comparing the live map against the virgin
 * map. (You implement the bucketing logic.)
 */
uint8_t cov_classify_count(uint8_t hits);

#endif /* COV_RT_H */
