# LLVM Coverage Pass

This directory holds the LLVM pass that injects `__cov_visit(id)` at the start
of every basic block.

## Build

```bash
mkdir -p build && cd build
cmake .. -G Ninja
ninja
ls -la CovPass.so
```

This produces `build/CovPass.so`. Use it with clang:

```bash
clang -fpass-plugin=./build/CovPass.so -O2 -c target.c -o target.o
```

The pass is also registered as `cov-pass` for `opt` if you want to inspect IR:

```bash
clang -O0 -emit-llvm -c target.c -o target.bc
opt -load-pass-plugin=./build/CovPass.so -passes=cov-pass target.bc -o target.cov.bc
llvm-dis target.cov.bc -o -
```

## What you must implement

`CovPass.cpp` is a working plugin scaffold. The `run()` method is where you
write the actual instrumentation - read the `TODO 1` comment and follow the
hints. Everything below `// Plugin registration boilerplate` is done.

## Sanity check

After building, verify your pass is being invoked by clang:

```bash
echo 'int main(){return 0;}' | clang -fpass-plugin=./build/CovPass.so \
    -O2 -x c - -c -o /tmp/t.o 2>&1 | head
```

You should see the `[CovPass] instrumented N basic blocks ...` message your
`run()` method prints.
