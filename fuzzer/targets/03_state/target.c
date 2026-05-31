/*
 * Target 03 - small state machine + bounded out-of-bounds write.  MEDIUM.
 *
 * The input is parsed byte-by-byte through a four-state machine. Reaching
 * the buggy state requires a specific *sequence* of transitions, not just
 * specific bytes; this exercises the fuzzer's coverage-tracking on indirect
 * branches and loop iteration buckets.
 *
 * State transitions:
 *
 *      S0 (start)
 *         '<'  -> S1
 *         '!'  -> S2
 *      S1 (after '<')
 *         '>'  -> S0
 *         'A'  -> S3
 *      S2 (collecting bytes into accum[])
 *         '!'  -> S0
 *         else -> append byte to accum, stay in S2
 *               (BUG: no bound check on accum_len)
 *      S3 (after '<A')
 *         'C'  -> S4
 *      S4 (winning state)
 *         'X' && accum_len > 32  -> *bad_ptr  (deterministic crash)
 *
 * The accum overflow itself is mostly a smoke-screen; the deterministic
 * crash trigger is the wild-pointer write at the end. We use it instead of
 * relying on stack-canary detection so the crash is loud regardless of
 * compiler hardening.
 */
#include <stdint.h>
#include <stddef.h>

int FuzzInput(const uint8_t *data, size_t size)
{
    int state = 0;
    char accum[32];
    int  accum_len = 0;

    for (size_t i = 0; i < size; i++) {
        unsigned char c = data[i];
        switch (state) {
        case 0:
            if (c == '<')      state = 1;
            else if (c == '!') state = 2;
            break;

        case 1:
            if (c == '>')      state = 0;
            else if (c == 'A') state = 3;
            break;

        case 2:
            /* BUG: append without checking accum_len < sizeof(accum) */
            accum[accum_len++] = (char)c;
            if (c == '!') state = 0;
            break;

        case 3:
            if (c == 'C') state = 4;
            break;

        case 4:
            if (c == 'X' && accum_len > 32) {
                volatile int *p = (volatile int *)0xdeadbeef;
                *p = 1;     /* deterministic crash */
            }
            break;
        }
    }
    return 0;
}
