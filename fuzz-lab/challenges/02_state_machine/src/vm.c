/*
 * Challenge 02 - Stream of Checksummed Records (mini "VM")
 * --------------------------------------------------------------
 *
 * The program reads a sequence of records off stdin (or argv[1]) and feeds
 * them to a tiny stateful interpreter that maintains one 64-byte working
 * buffer.  Each record carries its own one-byte XOR checksum, so the file
 * format looks like a stream and not a single self-contained blob.
 *
 *      +--------+----------+--------+--------+
 *      | type:1 | len:2 LE | data   | sum:1  |
 *      +--------+----------+--------+--------+
 *
 *      sum = type ^ len_lo ^ len_hi ^ data[0] ^ data[1] ^ ...
 *
 * Stream prefix (8 bytes): "VM01" + uint32 LE record_count.
 *
 * Record types:
 *      0x01 NOP    - data ignored
 *      0x02 STORE  - copy data into the working buffer (rejected if len>64)
 *      0x03 CONCAT - APPEND data to the working buffer
 *      0x04 RESET  - zero out the working buffer
 *      0x05 PRINT  - printf("%s\n", ...) the buffer (NUL-terminated)
 *
 * There is at least one memory-corruption bug.  Find it.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 64
#define MAX_RECORDS 4096
#define MAX_RECORD_LEN 4096

struct ctx {
    uint8_t *buf;       /* heap-allocated, exactly BUF_SIZE bytes */
    size_t   used;      /* current logical length */
};

static int read_exact(FILE *f, void *dst, size_t n)
{
    return fread(dst, 1, n, f) == n ? 0 : -1;
}

static int handle_record(struct ctx *c, FILE *f)
{
    uint8_t  type;
    uint16_t len;
    uint8_t  expected_sum;

    if (read_exact(f, &type, 1)        != 0) return 1;   /* clean EOF */
    if (read_exact(f, &len,  2)        != 0) return -1;

    if (len > MAX_RECORD_LEN) return -1;

    uint8_t *data = (uint8_t *)malloc(len ? len : 1);
    if (!data) return -1;

    if (len && read_exact(f, data, len) != 0) { free(data); return -1; }
    if (read_exact(f, &expected_sum, 1) != 0) { free(data); return -1; }

    /* XOR checksum verification */
    uint8_t sum = type ^ (uint8_t)(len & 0xFF) ^ (uint8_t)(len >> 8);
    for (uint16_t i = 0; i < len; i++) sum ^= data[i];
    if (sum != expected_sum) { free(data); return -2; }

    switch (type) {
    case 0x01: /* NOP */
        break;

    case 0x02: /* STORE - guarded */
        if (len > BUF_SIZE) { free(data); return -1; }
        memcpy(c->buf, data, len);
        c->used = len;
        break;

    case 0x03: /* CONCAT */
        /* BUG: forgot to check (c->used + len) <= BUF_SIZE.
         * Result: heap buffer overflow on the working buffer. */
        memcpy(c->buf + c->used, data, len);
        c->used += len;
        break;

    case 0x04: /* RESET */
        memset(c->buf, 0, BUF_SIZE);
        c->used = 0;
        break;

    case 0x05: /* PRINT */
        if (c->used < BUF_SIZE) c->buf[c->used] = '\0';
        else                    c->buf[BUF_SIZE - 1] = '\0';
        printf("%s\n", (const char *)c->buf);
        break;

    default:
        /* Unknown record: ignored, but we still consumed it. */
        break;
    }

    free(data);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *f = stdin;
    if (argc > 1) {
        f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return 1; }
    }

    char     magic[4];
    uint32_t record_count;

    if (read_exact(f, magic, 4) != 0 ||
        memcmp(magic, "VM01", 4) != 0) {
        fprintf(stderr, "bad magic\n");
        if (f != stdin) fclose(f);
        return 1;
    }
    if (read_exact(f, &record_count, 4) != 0) {
        if (f != stdin) fclose(f);
        return 1;
    }
    if (record_count > MAX_RECORDS) {
        fprintf(stderr, "too many records\n");
        if (f != stdin) fclose(f);
        return 1;
    }

    struct ctx c;
    c.buf  = (uint8_t *)calloc(1, BUF_SIZE);
    c.used = 0;
    if (!c.buf) {
        if (f != stdin) fclose(f);
        return 1;
    }

    for (uint32_t i = 0; i < record_count; i++) {
        int rc = handle_record(&c, f);
        if (rc == 1) break;            /* clean EOF mid-stream */
        if (rc < 0) {
            fprintf(stderr, "record %u rejected (%d)\n", i, rc);
            free(c.buf);
            if (f != stdin) fclose(f);
            return 2;
        }
    }

    free(c.buf);
    if (f != stdin) fclose(f);
    return 0;
}
