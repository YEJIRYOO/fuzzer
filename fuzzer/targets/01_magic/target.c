/*
 * Target 01 - magic bytes + small overflow.  EASY.
 *
 * The fuzzer should be able to:
 *   1) discover the "FUZZ" prefix by following coverage on the byte
 *      compares (each correct byte unlocks new coverage),
 *   2) then drive the payload size large enough to overflow the
 *      16-byte stack buffer.
 *
 * Expected outcome: a SIGSEGV (or fortify-detected abort) reasonably
 * quickly even with a very simple bit-flip + havoc mutator.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

int FuzzInput(const uint8_t *data, size_t size)
{
    if (size < 4) return 0;
    if (data[0] != 'F') return 0;
    if (data[1] != 'U') return 0;
    if (data[2] != 'Z') return 0;
    if (data[3] != 'Z') return 0;

    /* "Authenticated" path: copy the rest of the input into a tiny
     * stack buffer.  The bug: no size check. */
    char buf[16];
    size_t payload_len = size - 4;
    memcpy(buf, data + 4, payload_len);   /* BUG */
    return buf[0];
}
