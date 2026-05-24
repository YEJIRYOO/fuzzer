/*
 * Target 02 - nested specific branches.  EASY.
 *
 * Demonstrates how coverage feedback dramatically beats blind random
 * mutation. The crash requires *four* specific bytes in *order*
 * ('A', 'B', 'C', 'D') at offsets 0..3.  Random mutation alone needs
 * ~2^32 trials; a coverage-guided fuzzer finds it in seconds because
 * each correct byte unlocks a new edge.
 */
#include <stdint.h>
#include <stddef.h>

int FuzzInput(const uint8_t *data, size_t size)
{
    if (size < 4) return 0;
    if (data[0] == 'A')
      if (data[1] == 'B')
        if (data[2] == 'C')
          if (data[3] == 'D') {
              /* NULL deref - reliable SIGSEGV */
              volatile int *p = (volatile int *)0;
              *p = 0xdeadbeef;
          }
    return 0;
}
