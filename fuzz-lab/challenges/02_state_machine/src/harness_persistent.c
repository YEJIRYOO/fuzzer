/*
 * Reference persistent-mode harness for Challenge 02.
 *
 * This file is a *starting point* for students. It strips the file I/O off
 * vm.c and exposes the inner record loop so AFL++ can drive it via shared
 * memory. The behaviour is deliberately conservative - feel free to rewrite
 * it (e.g. fix the per-record XOR checksum on the fly, slice the record
 * stream from a flat byte buffer, etc.).
 *
 * Build with the Makefile target `vm.persistent`. Run with:
 *
 *      afl-fuzz -i seeds -o out -- ./vm.persistent
 *
 * Persistent mode reuses the process across many testcases - if you do
 * stateful work inside the loop, RESET it explicitly each iteration.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 64
#define MAX_RECORD_LEN 4096

/* AFL++ persistent-mode shim */
#ifdef __AFL_HAVE_MANUAL_CONTROL
#  include <unistd.h>
__AFL_FUZZ_INIT();
#endif

struct ctx { uint8_t *buf; size_t used; };

static const uint8_t *cursor;
static size_t         remaining;

static int take(void *dst, size_t n) {
    if (remaining < n) return -1;
    memcpy(dst, cursor, n);
    cursor    += n;
    remaining -= n;
    return 0;
}

static int handle(struct ctx *c)
{
    uint8_t  type, sum;
    uint16_t len;
    if (take(&type, 1)) return 1;
    if (take(&len,  2)) return -1;
    if (len > MAX_RECORD_LEN) return -1;

    const uint8_t *data = cursor;
    if (remaining < (size_t)len + 1) return -1;
    cursor    += len;
    remaining -= len;

    if (take(&sum, 1)) return -1;

    uint8_t s = type ^ (uint8_t)(len & 0xFF) ^ (uint8_t)(len >> 8);
    for (uint16_t i = 0; i < len; i++) s ^= data[i];
    if (s != sum) return -2;

    switch (type) {
    case 0x02:
        if (len > BUF_SIZE) return -1;
        memcpy(c->buf, data, len);
        c->used = len;
        break;
    case 0x03:
        memcpy(c->buf + c->used, data, len);  /* same bug as vm.c */
        c->used += len;
        break;
    case 0x04:
        memset(c->buf, 0, BUF_SIZE);
        c->used = 0;
        break;
    case 0x05:
        if (c->used < BUF_SIZE) c->buf[c->used] = '\0';
        else                    c->buf[BUF_SIZE - 1] = '\0';
        printf("%s\n", c->buf);
        break;
    default:
        break;
    }
    return 0;
}

static void run_once(const uint8_t *input, size_t n)
{
    if (n < 8) return;
    if (memcmp(input, "VM01", 4) != 0) return;

    uint32_t rc;
    memcpy(&rc, input + 4, 4);
    if (rc > 4096) return;

    cursor    = input + 8;
    remaining = n - 8;

    struct ctx c;
    c.buf  = (uint8_t *)calloc(1, BUF_SIZE);
    c.used = 0;
    if (!c.buf) return;

    for (uint32_t i = 0; i < rc; i++) {
        int r = handle(&c);
        if (r != 0) break;
    }
    free(c.buf);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    unsigned char *afl_buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
        run_once(afl_buf, (size_t)len);
    }
    return 0;
#else
    /* Non-AFL build: read all of stdin once. */
    uint8_t buf[1 << 20];
    size_t  n = fread(buf, 1, sizeof(buf), stdin);
    run_once(buf, n);
    return 0;
#endif
}
