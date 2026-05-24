# Target 03 - state machine (medium)

## Bug class

A wild-pointer write reachable only by driving a small state machine
through state 4 with a specific sequence: bytes that have been collected
in state S2 (`!...!`) so the accumulator length exceeds 32, *and then*
an `<A C` sequence that pushes the machine into state S4 with `X`.

## Why this one is harder

Coverage on byte-equality branches alone is not enough. Reaching state 4
needs a multi-character word (`<AC`), and triggering the crash requires
the previous S2 visit to have left `accum_len > 32`. Your fuzzer must:

1. record different "edge" coverage for each state transition,
2. preserve "interesting" inputs that reach S4 even before they crash,
3. mutate by *splicing* or *insertion* often enough to grow the accum
   buffer past 32 bytes.

If your mutator only does bit-flips you may struggle; add havoc-style
random byte insertion / chunk duplication.

## Expected behaviour

A reasonable student fuzzer with a havoc stage finds this in a few
minutes. Without havoc-style insertion you may not find it at all -
that's the lesson.
