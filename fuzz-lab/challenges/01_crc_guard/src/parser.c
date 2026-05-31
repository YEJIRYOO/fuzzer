/*
 * Challenge 01 - CRC-Guarded Container Parser
 * --------------------------------------------------------------
 *
 * File format ("FZ01"):
 *
 *      offset 0  : magic[4]      = "FZ01"
 *      offset 4  : version[2]    = 0x0001 little-endian
 *      offset 6  : payload_len[4]= uint32 LE
 *      offset 10 : crc32[4]      = uint32 LE, IEEE 802.3 over payload
 *      offset 14 : payload[payload_len]
 *
 *  - The payload is rejected unless the CRC32 matches.
 *  - At least one memory-corruption bug lurks somewhere in this file.
 *  - The binary segfaults (or ASAN-aborts) on a "good" crashing input.
 *
 * NOTE TO STUDENTS
 *  Your first naive `afl-fuzz` run will burn cycles on the magic and CRC.
 *  Read the README and HINTS in this directory before deciding on a strategy.
 *
 * (c) 2026 - System Security course material
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---- IEEE 802.3 CRC32 (poly 0xEDB88320) -------------------------------- */
static uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
    }
    return ~crc;
}

/* ---- Header ------------------------------------------------------------ */
struct __attribute__((packed)) header {
    char     magic[4];   /* "FZ01" */
    uint16_t version;    /* 0x0001 */
    uint32_t payload_len;
    uint32_t crc;
};

#define MAX_PAYLOAD (1u << 20)   /* 1 MiB hard cap so AFL doesn't OOM */

/* ---- Inner parser ------------------------------------------------------ *
 *
 * Sub-records inside the payload look like:
 *      tag[4]  : "ADMN" | "USER" | "NOTE"
 *      data ...
 *
 * The "ADMN" branch reads a 1-byte name length and copies that many bytes
 * into a stack-resident 32-byte buffer.  *That* is the bug we want our
 * fuzzer to find.
 */
static void handle_admin(const uint8_t *p, uint32_t len)
{
    if (len < 5) return;

    uint8_t name_len = p[4];

    /* Length is bounded against the *outer* payload, but NOT against the
     * destination buffer size. Classic off-by-spec stack overflow. */
    if ((uint32_t)5 + name_len > len) return;

    char name[32];
    /* BUG: name_len can be up to 0xFF, name[] is only 32 bytes wide. */
    memcpy(name, p + 5, name_len);
    name[name_len < sizeof(name) ? name_len : (sizeof(name) - 1)] = '\0';

    printf("[admin] name=\"%s\"\n", name);
}

static void handle_user(const uint8_t *p, uint32_t len)
{
    /* Boring branch on purpose - only here so coverage isn't trivial. */
    if (len < 8) return;
    uint32_t uid = (uint32_t)p[4] | ((uint32_t)p[5] << 8)
                 | ((uint32_t)p[6] << 16) | ((uint32_t)p[7] << 24);
    printf("[user]  uid=%u\n", uid);
}

static void handle_note(const uint8_t *p, uint32_t len)
{
    if (len <= 4) return;
    /* Just print the trailing bytes as a (possibly non-terminated) note. */
    printf("[note]  %.*s\n", (int)(len - 4), (const char *)(p + 4));
}

static int dispatch(const uint8_t *payload, uint32_t len)
{
    if (len < 4) return -1;
    if (!memcmp(payload, "ADMN", 4)) { handle_admin(payload, len); return 0; }
    if (!memcmp(payload, "USER", 4)) { handle_user (payload, len); return 0; }
    if (!memcmp(payload, "NOTE", 4)) { handle_note (payload, len); return 0; }
    return -1;
}

/* ---- Driver ------------------------------------------------------------ */
static int read_all(FILE *f, uint8_t **out, size_t *out_len)
{
    size_t cap = 4096, len = 0;
    uint8_t *buf = (uint8_t *)malloc(cap);
    if (!buf) return -1;
    for (;;) {
        if (len == cap) {
            cap *= 2;
            uint8_t *nb = (uint8_t *)realloc(buf, cap);
            if (!nb) { free(buf); return -1; }
            buf = nb;
        }
        size_t n = fread(buf + len, 1, cap - len, f);
        len += n;
        if (n == 0) break;
    }
    *out = buf;
    *out_len = len;
    return 0;
}

int main(int argc, char **argv)
{
    FILE *f = stdin;
    if (argc > 1) {
        f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return 1; }
    }

    uint8_t *blob = NULL; size_t blob_len = 0;
    if (read_all(f, &blob, &blob_len) != 0 || blob_len < sizeof(struct header)) {
        fprintf(stderr, "input too small\n");
        free(blob);
        if (f != stdin) fclose(f);
        return 1;
    }

    struct header h;
    memcpy(&h, blob, sizeof(h));

    if (memcmp(h.magic, "FZ01", 4) != 0) {
        fprintf(stderr, "bad magic\n");
        free(blob);
        if (f != stdin) fclose(f);
        return 1;
    }
    if (h.version != 0x0001) {
        fprintf(stderr, "unsupported version 0x%04x\n", h.version);
        free(blob);
        if (f != stdin) fclose(f);
        return 1;
    }
    if (h.payload_len > MAX_PAYLOAD) {
        fprintf(stderr, "payload too large\n");
        free(blob);
        if (f != stdin) fclose(f);
        return 1;
    }
    if (sizeof(h) + (size_t)h.payload_len > blob_len) {
        fprintf(stderr, "truncated payload\n");
        free(blob);
        if (f != stdin) fclose(f);
        return 1;
    }

    const uint8_t *payload = blob + sizeof(h);

#ifndef DISABLE_CRC
    uint32_t got = crc32_ieee(payload, h.payload_len);
    if (got != h.crc) {
        fprintf(stderr, "bad crc (have 0x%08x, want 0x%08x)\n", got, h.crc);
        free(blob);
        if (f != stdin) fclose(f);
        return 2;
    }
#endif

    int rc = dispatch(payload, h.payload_len);
    free(blob);
    if (f != stdin) fclose(f);
    return rc;
}
