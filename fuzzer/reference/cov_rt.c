/*
 * REFERENCE solution for runtime/cov_rt.c.  Instructor-only.
 *
 * Drop this on top of runtime/cov_rt.c to verify the rest of the lab
 * is wired up correctly.
 */
#include "../runtime/cov_rt.h"
#include <string.h>

uint8_t  cov_map[COV_MAP_SIZE];
uint16_t cov_prev_loc;

void __cov_visit(uint32_t cur_loc)
{
    uint32_t idx = (cur_loc ^ (uint32_t)cov_prev_loc) & COV_MAP_MASK;
    if (cov_map[idx] != 0xFFu) cov_map[idx]++;
    cov_prev_loc = (uint16_t)(cur_loc >> 1);
}

void cov_reset(void)
{
    memset(cov_map, 0, sizeof(cov_map));
    cov_prev_loc = 0;
}

uint8_t cov_classify_count(uint8_t hits)
{
    if (hits == 0)   return 0;
    if (hits == 1)   return 1;
    if (hits == 2)   return 2;
    if (hits == 3)   return 4;
    if (hits <= 7)   return 8;
    if (hits <= 15)  return 16;
    if (hits <= 31)  return 32;
    if (hits <= 127) return 64;
    return 128;
}
