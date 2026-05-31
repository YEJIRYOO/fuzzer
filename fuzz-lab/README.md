# fuzz-lab

System Security course - AFL++ fuzzing playground.

The container ships with `build-essential`, `clang/llvm`, `gdb`, and a
source build of AFL++ (`afl-fuzz`, `afl-clang-fast`, `afl-cmin`, `afl-tmin`,
CmpLog, persistent-mode runtime, etc.). Three medium-difficulty challenges
under `~/challenges` are pre-built and ready to fuzz.

## 1. Build & enter the container

You build the image once on your own machine. Apple Silicon Macs get a
`linux/arm64` image; Intel/AMD PCs get `linux/amd64`. AFL++ is compiled
from source inside the build, so it always matches your host arch (no
QEMU emulation, full speed).

From the directory that contains this `README.md` and the `Dockerfile`:

```bash
# 1) build the image (one-off, takes a few minutes the first time)
docker build -t fuzz-lab .

# 2) start an interactive shell inside the container
docker run --rm -it fuzz-lab
```

Option B - if you also want crash inputs to be saved on the host (so they
survive `docker rm` and you can open them from your IDE):

```bash
docker compose up -d --build      # builds and starts in the background
docker compose exec lab bash      # drop into the running container
```

When the shell starts you should see the `fuzz-lab` banner. From here you
are inside the container - everything below assumes you stay inside.

The first run may print AFL++ warnings about
`/sys/devices/system/cpu/.../scaling_governor` or
`/proc/sys/kernel/core_pattern`. They are silenced for you in the image
(`AFL_SKIP_CPUFREQ=1`, `AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1`) and can
be ignored on Docker for Mac.

## 2. Layout inside the container

```
~/challenges/
  01_crc_guard/        # CRC32-protected container parser
  02_state_machine/    # Per-record XOR-checksummed record stream
  03_tlv_adler/        # Adler32-protected TLV file
~/scripts/
  build_all.sh         # rebuild every challenge + regenerate seeds
  run_fuzz.sh          # convenience wrapper around afl-fuzz
```

Each challenge directory has:

- `src/*.c`           - the buggy program (single C file)
- `src/Makefile`      - builds five flavours: native, AFL, CmpLog, ASAN,
                        and a "guard-removed" easy mode for development
- `seeds/`            - pre-generated seed corpus
- `gen_seed.py`       - regenerates the seed corpus
- `README.md`         - hints and rules of engagement (no spoilers)

## 3. Quick sanity check (inside the container)

Run each line below and confirm you see the indicated output. All three
binaries are pre-built when the image is created, so this should "just
work" the first time you start the container.

```bash
$ ./challenges/01_crc_guard/src/parser ./challenges/01_crc_guard/seeds/admin_short.bin
[admin] name="root"

$ ./challenges/02_state_machine/src/vm < ./challenges/02_state_machine/seeds/store.bin
hello

$ ./challenges/03_tlv_adler/src/tlv ./challenges/03_tlv_adler/seeds/show_basic.bin
dup[0] = 0xab
```

All three should exit with status 0. If any line errors or prints
something different, run `~/scripts/build_all.sh` to regenerate seeds and
rebuild every challenge.

## 4. Run the fuzzer (inside the container)

The convenience wrapper is the recommended starting point:

```bash
~/scripts/run_fuzz.sh ~/challenges/01_crc_guard
```

That command:

1. picks the first instrumented binary in `src/` (`parser.afl`),
2. uses `seeds/` as the input corpus,
3. writes findings into `~/findings/01_crc_guard/`,
4. and starts `afl-fuzz` in the foreground.

You will see the live AFL++ status screen update in your terminal. Leave
it running. Crashes show up under `~/findings/01_crc_guard/default/crashes/`
as soon as the fuzzer finds them.

To start CmpLog (often dramatically faster on the guarded binaries):

```bash
~/scripts/run_fuzz.sh ~/challenges/01_crc_guard parser.afl -c ./parser.cmplog
```

To run a different challenge, change the directory argument:

```bash
~/scripts/run_fuzz.sh ~/challenges/02_state_machine
~/scripts/run_fuzz.sh ~/challenges/03_tlv_adler
```

You can also call `afl-fuzz` directly if you want full control:

```bash
cd ~/challenges/01_crc_guard/src
afl-fuzz -i ../seeds -o ~/findings/01 -- ./parser.afl
```

### Stopping the fuzzer

While the AFL++ status screen is up, press **Ctrl+C** once. AFL writes
out the queue/crashes/hangs and exits cleanly.

### Inspecting crashes

```bash
ls ~/findings/01_crc_guard/default/crashes/
# pick one
xxd ~/findings/01_crc_guard/default/crashes/id:000000,*
# replay it
~/challenges/01_crc_guard/src/parser ~/findings/01_crc_guard/default/crashes/id:000000,*
```

For a clean ASan-decoded backtrace, replay the crash against the ASAN
binary:

```bash
~/challenges/01_crc_guard/src/parser.asan \
    ~/findings/01_crc_guard/default/crashes/id:000000,*
```

## 5. Exiting the container

You have two ways out depending on how you started it.

**If you used `docker run --rm -it fuzz-lab`** (Option A above):

- Type `exit` (or press Ctrl+D) at the shell prompt. The container stops
  and is automatically removed because of `--rm`. Your fuzz findings
  inside the container are gone with it - copy anything you want to keep
  out first with `docker cp` from another terminal.

**If you used `docker compose up -d`** (Option B):

- Type `exit` to leave the shell; the container keeps running in the
  background.
- To stop and remove it: `docker compose down` (run on the host, not
  inside the container). Files under `./findings/` on your host are
  preserved.

To force-stop a runaway fuzzer container from the host:

```bash
docker ps                      # find the container ID or name
docker stop <id-or-name>
```

## 6. Difficulty knob

All three challenges ship with both a *guarded* binary (`*.afl`) and a
*guard-removed* binary (`parser.nocrc`, `tlv.noadler`, etc.). Use the
guard-removed builds to develop and validate your fuzzing strategy
quickly, then prove it again on the guarded binary - that is the
artifact graded.
