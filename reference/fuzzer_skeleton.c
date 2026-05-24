/*
 * Pristine student skeleton for fuzzer/fuzzer.c.
 *
 * Restore command:
 *      cp reference/fuzzer_skeleton.c fuzzer/fuzzer.c
 *
 * NOTE: this skeleton tracks the current contract — three student TODOs
 * (queue_add, queue_pick, has_new_coverage) and a *provided*, working
 * mutate(). When you change fuzzer/fuzzer.c, mirror the change here so
 * the backup stays meaningful.
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
#define QUEUE_GROW_STEP    64

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

/* ============================================================ *
 *  TODOs                                                       *
 * ============================================================ */

static void queue_add(const uint8_t *data, size_t size, int from_seed)
{
    /* TODO: implement */
    (void)data; (void)size; (void)from_seed;
}

static Input *queue_pick(void)
{
    /* TODO: implement */
    return NULL;
}

/* PROVIDED mutator - do not modify. */
static void mutate(uint8_t *out, size_t *out_size,
                   const uint8_t *in, size_t in_size)
{
    if (in_size > g_max_input) in_size = g_max_input;
    memcpy(out, in, in_size);
    size_t sz = in_size;

    uint32_t r = rng_next() % 100;
    int havoc_steps = (r >= 80) ? (4 + (rng_next() % 12)) : 1;

    for (int step = 0; step < havoc_steps; step++) {
        if (sz == 0) {
            out[0] = (uint8_t)rng_next();
            sz = 1;
            continue;
        }
        switch (rng_next() % 6) {
        case 0: { size_t pos = rng_next() % sz;
                  out[pos] ^= (uint8_t)(1u << (rng_next() % 8)); break; }
        case 1: { size_t pos = rng_next() % sz;
                  out[pos] = (uint8_t)rng_next(); break; }
        case 2: { size_t pos = rng_next() % sz;
                  out[pos] = (uint8_t)((int)out[pos] +
                       ((int)(rng_next() & 0x1f) - 16)); break; }
        case 3: { if (sz < g_max_input) {
                  size_t pos = rng_next() % (sz + 1);
                  memmove(out + pos + 1, out + pos, sz - pos);
                  out[pos] = (uint8_t)rng_next(); sz++; } break; }
        case 4: { if (sz > 1) { size_t pos = rng_next() % sz;
                  memmove(out + pos, out + pos + 1, sz - pos - 1);
                  sz--; } break; }
        case 5: { if (sz + 4 < g_max_input) {
                  size_t srclen = 1 + (rng_next() % 4);
                  if (srclen > sz) srclen = sz;
                  size_t srcpos = rng_next() % (sz - srclen + 1);
                  size_t dstpos = rng_next() % (sz + 1);
                  uint8_t tmp[8];
                  memcpy(tmp, out + srcpos, srclen);
                  memmove(out + dstpos + srclen, out + dstpos,
                          sz - dstpos);
                  memcpy(out + dstpos, tmp, srclen); sz += srclen; }
                  break; }
        }
    }
    if (sz > g_max_input) sz = g_max_input;
    *out_size = sz;
}

static int has_new_coverage(void)
{
    /* TODO: implement */
    return 0;
}

/* (driver code identical to fuzzer/fuzzer.c — kept short for restore) */
static void save_crash(const uint8_t *data, size_t size, int signo)
{
    char path[512];
    snprintf(path, sizeof path, "%s/crash_%06lu_sig%d.bin",
             g_crash_dir, n_uniq, signo);
    FILE *f = fopen(path, "wb"); if (!f) return;
    fwrite(data, 1, size, f); fclose(f); n_uniq++;
}
static int load_seeds(const char *dir_path)
{
    DIR *d = opendir(dir_path);
    if (!d) { perror(dir_path); return -1; }
    struct dirent *de; int loaded = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof path, "%s/%s", dir_path, de->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        FILE *f = fopen(path, "rb"); if (!f) continue;
        uint8_t *buf = (uint8_t *)malloc(g_max_input);
        size_t n = fread(buf, 1, g_max_input, f);
        fclose(f);
        queue_add(buf, n, 1);
        free(buf); loaded++;
    }
    closedir(d); return loaded;
}
static int run_once(const uint8_t *data, size_t size)
{
    cov_reset();
    if (sigsetjmp(crash_jmp, 1) == 0) { FuzzInput(data, size); return 0; }
    return (int)crash_signo;
}
static void print_status(void)
{
    time_t elapsed = time(NULL) - start_time; if (elapsed <= 0) elapsed = 1;
    fprintf(stderr,
        "\r[+] execs=%lu  speed=%lu/s  queue=%zu  crashes=%lu (uniq=%lu)  elapsed=%lds   ",
        n_execs, n_execs / (unsigned long)elapsed,
        queue_count, n_crashes, n_uniq, (long)elapsed);
    fflush(stderr);
}
static void usage(const char *p) {
    fprintf(stderr, "usage: %s -s SEEDS -o CRASHES [-T SECONDS] [-m MAX]\n", p);
    exit(64);
}
int main(int argc, char **argv)
{
    const char *seeds_dir = NULL;
    long duration_sec = 0;
    int opt;
    while ((opt = getopt(argc, argv, "s:o:T:m:h")) != -1) {
        switch (opt) {
        case 's': seeds_dir = optarg; break;
        case 'o': strncpy(g_crash_dir, optarg, sizeof g_crash_dir - 1); break;
        case 'T': duration_sec = atol(optarg); break;
        case 'm': g_max_input = (size_t)atol(optarg); break;
        case 'h': default: usage(argv[0]);
        }
    }
    if (!seeds_dir) usage(argv[0]);
    if (g_max_input == 0) g_max_input = DEFAULT_MAX_INPUT;
    install_crash_handlers();
    rng_state = (uint32_t)time(NULL) ^ (uint32_t)getpid();
    start_time = time(NULL);
    mkdir(g_crash_dir, 0755);
    int loaded = load_seeds(seeds_dir);
    if (loaded <= 0) { fprintf(stderr, "[-] no seeds\n"); return 1; }
    fprintf(stderr, "[+] loaded %d seed(s)\n", loaded);
    for (size_t i = 0; i < queue_count; i++) {
        run_once(queue[i].buf, queue[i].size);
        has_new_coverage();
    }
    uint8_t *child = (uint8_t *)malloc(g_max_input);
    if (!child) return 1;
    while (1) {
        if (duration_sec > 0 && (time(NULL) - start_time) >= duration_sec) break;
        Input *parent = queue_pick();
        if (!parent) break;
        size_t child_size = 0;
        mutate(child, &child_size, parent->buf, parent->size);
        int sig = run_once(child, child_size);
        n_execs++;
        if (sig != 0) { n_crashes++; save_crash(child, child_size, sig); }
        if (has_new_coverage()) queue_add(child, child_size, 0);
        if ((n_execs & 0x3FFu) == 0) print_status();
    }
    print_status(); fprintf(stderr, "\n");
    free(child); return 0;
}
