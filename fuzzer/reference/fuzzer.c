/*
 * REFERENCE solution for fuzzer/fuzzer.c (the four TODO functions).
 * Instructor-only.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>

#include "../runtime/cov_rt.h"

extern int FuzzInput(const uint8_t *data, size_t size);

#define DEFAULT_MAX_INPUT  4096

typedef struct {
    uint8_t *buf;
    size_t   size;
    int      from_seed;
} Input;

static Input    *queue          = NULL;
static size_t    queue_count    = 0;
static size_t    queue_capacity = 0;

static uint8_t   virgin_map[COV_MAP_SIZE];
static size_t    g_max_input    = DEFAULT_MAX_INPUT;
static unsigned long n_execs    = 0;
static unsigned long n_crashes  = 0;
static unsigned long n_uniq     = 0;
static time_t        start_time = 0;
static char          g_crash_dir[256] = "crashes";

static uint32_t rng_state = 0x12345678u;
static uint32_t rng_next(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x ? x : 0x12345678u;
    return rng_state;
}

static sigjmp_buf crash_jmp;
static volatile sig_atomic_t crash_signo = 0;
static void crash_handler(int signo)
{
    crash_signo = (sig_atomic_t)signo;
    siglongjmp(crash_jmp, 1);
}
static void install_crash_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
}

/* === reference TODOs === */

static void queue_add(const uint8_t *data, size_t size, int from_seed)
{
    if (queue_count == queue_capacity) {
        queue_capacity = queue_capacity ? queue_capacity * 2 : 64;
        queue = (Input *)realloc(queue, queue_capacity * sizeof(Input));
    }
    Input *e = &queue[queue_count++];
    e->buf       = (uint8_t *)malloc(size ? size : 1);
    e->size      = size;
    e->from_seed = from_seed;
    if (size) memcpy(e->buf, data, size);
}

static Input *queue_pick(void)
{
    if (queue_count == 0) return NULL;
    return &queue[rng_next() % queue_count];
}

static void mutate(uint8_t *out, size_t *out_size,
                   const uint8_t *in, size_t in_size)
{
    if (in_size > g_max_input) in_size = g_max_input;
    memcpy(out, in, in_size);
    size_t sz = in_size;

    /* Pick a strategy with weighted distribution. */
    uint32_t r = rng_next() % 100;
    int havoc_steps = (r >= 80) ? (4 + (rng_next() % 12)) : 1;

    for (int step = 0; step < havoc_steps; step++) {
        if (sz == 0 && (rng_next() & 1)) {
            /* must insert if empty */
            out[0] = (uint8_t)rng_next();
            sz = 1;
            continue;
        }
        switch (rng_next() % 6) {
        case 0: { /* bit flip */
            if (sz == 0) break;
            size_t pos = rng_next() % sz;
            out[pos] ^= (uint8_t)(1u << (rng_next() % 8));
        } break;
        case 1: { /* random byte */
            if (sz == 0) break;
            size_t pos = rng_next() % sz;
            out[pos] = (uint8_t)rng_next();
        } break;
        case 2: { /* arith add */
            if (sz == 0) break;
            size_t pos = rng_next() % sz;
            out[pos] += (int8_t)(rng_next() & 0x1f) - 16;
        } break;
        case 3: { /* insert */
            if (sz < g_max_input) {
                size_t pos = rng_next() % (sz + 1);
                memmove(out + pos + 1, out + pos, sz - pos);
                out[pos] = (uint8_t)rng_next();
                sz++;
            }
        } break;
        case 4: { /* delete */
            if (sz > 0) {
                size_t pos = rng_next() % sz;
                memmove(out + pos, out + pos + 1, sz - pos - 1);
                sz--;
            }
        } break;
        case 5: { /* duplicate chunk */
            if (sz > 0 && sz + 4 < g_max_input) {
                size_t pos = rng_next() % sz;
                size_t len = 1 + (rng_next() % 4);
                if (pos + len > sz) len = sz - pos;
                size_t dst = rng_next() % (sz + 1);
                memmove(out + dst + len, out + dst, sz - dst);
                memcpy(out + dst, out + pos + (dst <= pos ? len : 0), len);
                sz += len;
            }
        } break;
        }
    }

    if (sz > g_max_input) sz = g_max_input;
    *out_size = sz;
}

static int has_new_coverage(void)
{
    int found = 0;
    for (size_t i = 0; i < COV_MAP_SIZE; i++) {
        uint8_t b = cov_classify_count(cov_map[i]);
        if (b > virgin_map[i]) {
            virgin_map[i] = b;
            found = 1;
        }
    }
    return found;
}

/* (Driver code is identical to fuzzer.c; omitted in this reference file.) */
