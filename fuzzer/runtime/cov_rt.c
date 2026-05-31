/*
 * Coverage runtime - student skeleton.
 *
 * You must fill in the three functions marked TODO. Read cov_rt.h first;
 * the symbol names and contracts are fixed (the LLVM pass and the fuzzer
 * call into these directly).
 *
 * Once finished, this file should compile cleanly with:
 *
 *      cc -O2 -fPIC -c runtime/cov_rt.c -o runtime/cov_rt.o
 *
 * The fuzzer driver and every instrumented target are linked against
 * cov_rt.o.
 */
#include "cov_rt.h"
#include <string.h>

/* Storage for the global bitmap and the previous-location register.
 * These two definitions are correct as-is; do not change them. */
uint8_t  cov_map[COV_MAP_SIZE];
uint16_t cov_prev_loc;

/*
 * TODO 1.
 *
 * Update the bitmap to record an "edge" (prev_loc -> cur_loc) transition.
 *
 * Hints (read carefully):
 *  - The edge id is computed as (cur_loc XOR cov_prev_loc) modulo
 *    COV_MAP_SIZE.  Use COV_MAP_MASK so the result lands inside the
 *    bitmap.
 *  - Increment cov_map[edge_id] by one. Saturate at 0xFF (do not let the
 *    counter wrap around, otherwise you lose information about loops).
 *  - After updating the map, update cov_prev_loc so the *next* call sees
 *    this block as its predecessor. To keep edge ids asymmetric (i.e.
 *    A->B different from B->A) shift cur_loc right by one bit before
 *    storing.
 */
void __cov_visit(uint32_t cur_loc)
{
    int32_t idx = (cur_loc ^ (uint32_t)cov_prev_loc) & COV_MAP_MASK;
    if (cov_map[idx] != 0xFFu) cov_map[idx]++; // 8 비트 saturate
    cov_prev_loc = (uint16_t)(cur_loc >> 1);
}

/*
 * TODO 2.
 *
 * Zero the bitmap and reset cov_prev_loc to 0. The fuzzer calls this
 * between test cases so each execution starts from a clean coverage
 * state.
 */
void cov_reset(void)
{
    memset(cov_map, 0, sizeof(cov_map));
    cov_prev_loc = 0;
}

/*
 * TODO 3.
 *
 * Bucket an 8-bit hit count into a power-of-two-style category. The
 * exact buckets you choose are up to you, but a reasonable starting
 * scheme (matching AFL) is:
 *
 *      hits == 0  ->   0
 *      hits == 1  ->   1
 *      hits == 2  ->   2
 *      hits == 3  ->   4
 *      4..7       ->   8
 *      8..15      ->   16
 *      16..31     ->   32
 *      32..127    ->   64
 *      128..255   ->   128
 *
 * Returning 0 for hits==0 is REQUIRED (the fuzzer relies on it). The
 * other buckets just need to be monotonic and stable.
 */
uint8_t cov_classify_count(uint8_t hits)
{
    if (hits == 0) return 0;
    if (hits == 1) return 1;
    if (hits == 2) return 2;
    if (hits == 3) return 4;
    if (hits <= 7) return 8;
    if (hits <= 15) return 16;
    if (hits <= 31) return 32;
    if (hits <= 127) return 64;
    return 128;
}
