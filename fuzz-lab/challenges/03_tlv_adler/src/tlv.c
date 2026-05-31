/*
 * Challenge 03 - TLV records under an Adler32 file checksum
 * --------------------------------------------------------------
 *
 *      +---------+---------+---------+--------------------+
 *      | magic   | body_len| adler32 | body (body_len)    |
 *      | "TLV1"  | u32 LE  | u32 LE  | TLV records        |
 *      +---------+---------+---------+--------------------+
 *
 *      adler32 = standard Adler-32 over body[].
 *
 * TLV record:  tag:1  len:2 LE  value:len
 *
 *      tag 0x01 NAME : copy value into g_name (max 64)
 *      tag 0x02 LOAD : copy value into g_buf  (max 1024)
 *      tag 0x20 DUP  : malloc a private copy of g_buf into g_dup
 *      tag 0x21 DROP : free(g_dup)
 *      tag 0x22 SHOW : touch g_dup[0] and print it
 *
 * The bug here is *not* a buffer overflow.  AFL needs to discover a
 * particular *sequence* of records, then pass the Adler32 guard for the
 * body that contains it. Build with `tlv.asan` to see the bug class.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BODY  (1u << 20)
#define MAX_NAME  64
#define MAX_BUF   1024

/* ---- Adler32 ----------------------------------------------------------- */
static uint32_t adler32(const uint8_t *data, size_t len)
{
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % 65521u;
        b = (b + a)       % 65521u;
    }
    return (b << 16) | a;
}

/* ---- Working state ----------------------------------------------------- */
static char     g_name[MAX_NAME];
static uint8_t  g_buf[MAX_BUF];
static size_t   g_buf_len;

static uint8_t *g_dup;        /* heap-allocated private copy */
static size_t   g_dup_len;

/* ---- TLV dispatcher ---------------------------------------------------- */
static int handle_record(uint8_t tag, const uint8_t *value, uint16_t len)
{
    switch (tag) {
    case 0x01: /* NAME */
        if (len > MAX_NAME) return -1;
        memcpy(g_name, value, len);
        if (len < MAX_NAME) g_name[len] = '\0';
        return 0;

    case 0x02: /* LOAD */
        if (len > MAX_BUF) return -1;
        memcpy(g_buf, value, len);
        g_buf_len = len;
        return 0;

    case 0x20: /* DUP - allocate a private copy of g_buf */
        if (len != 0) return -1;
        free(g_dup);
        g_dup = (uint8_t *)malloc(g_buf_len ? g_buf_len : 1);
        if (!g_dup) return -1;
        memcpy(g_dup, g_buf, g_buf_len);
        g_dup_len = g_buf_len;
        return 0;

    case 0x21: /* DROP - free the copy */
        if (len != 0) return -1;
        free(g_dup);
        /* BUG: g_dup and g_dup_len are not reset.  Future references to
         * g_dup operate on a freed heap region. */
        return 0;

    case 0x22: /* SHOW - inspect/touch the copy */
        if (len != 0) return -1;
        if (!g_dup) return -1;
        if (g_dup_len > 0)
            g_dup[0] = (uint8_t)(g_dup[0] + 1);  /* UAF write */
        printf("dup[0] = 0x%02x\n", g_dup ? g_dup[0] : 0);
        return 0;

    default:
        return -1;
    }
}

/* ---- File reader ------------------------------------------------------- */
int main(int argc, char **argv)
{
    FILE *f = stdin;
    if (argc > 1) {
        f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return 1; }
    }

    char     magic[4];
    uint32_t body_len;
    uint32_t adler;

    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "TLV1", 4) != 0) {
        fprintf(stderr, "bad magic\n");
        if (f != stdin) fclose(f);
        return 1;
    }
    if (fread(&body_len, 4, 1, f) != 1 ||
        fread(&adler,    4, 1, f) != 1) {
        if (f != stdin) fclose(f);
        return 1;
    }
    if (body_len > MAX_BODY) {
        fprintf(stderr, "body too large\n");
        if (f != stdin) fclose(f);
        return 1;
    }

    uint8_t *body = (uint8_t *)malloc(body_len ? body_len : 1);
    if (!body) { if (f != stdin) fclose(f); return 1; }
    if (fread(body, 1, body_len, f) != body_len) {
        free(body); if (f != stdin) fclose(f); return 1;
    }

#ifndef DISABLE_ADLER
    if (adler32(body, body_len) != adler) {
        fprintf(stderr, "bad adler32\n");
        free(body); if (f != stdin) fclose(f); return 2;
    }
#endif

    /* Reset state */
    memset(g_name, 0, sizeof(g_name));
    memset(g_buf,  0, sizeof(g_buf));
    g_buf_len = 0;
    g_dup     = NULL;
    g_dup_len = 0;

    /* Walk records */
    size_t off = 0;
    while (off + 3 <= body_len) {
        uint8_t  tag = body[off];
        uint16_t len = (uint16_t)body[off + 1] |
                      ((uint16_t)body[off + 2] << 8);
        off += 3;
        if (off + len > body_len) break;
        int rc = handle_record(tag, body + off, len);
        off += len;
        if (rc != 0) {
            fprintf(stderr, "record rejected (tag=0x%02x)\n", tag);
            free(body); if (f != stdin) fclose(f); return 3;
        }
    }

    free(body);
    if (f != stdin) fclose(f);
    return 0;
}
