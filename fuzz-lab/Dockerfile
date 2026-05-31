# syntax=docker/dockerfile:1.6
#
# fuzz-lab - System Security class AFL++ playground
#
# Ubuntu 22.04 base with:
#   - build-essential, clang/llvm, gdb, etc.
#   - AFL++ (built from source for the host arch)
#   - Three medium-difficulty fuzzing challenges (CRC32, XOR sum, Adler32)
#
# Build:  docker build -t fuzz-lab .
# Run:    docker run --rm -it fuzz-lab
#
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG AFLPP_REF=v4.21c

LABEL org.opencontainers.image.title="fuzz-lab" \
      org.opencontainers.image.description="System Security class - AFL++ fuzzing lab" \
      org.opencontainers.image.source="local"

# ---------------------------------------------------------------------------
# Base toolchain
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        clang-14 \
        clang-tools-14 \
        libclang-14-dev \
        libclang-common-14-dev \
        llvm-14 \
        llvm-14-dev \
        llvm-14-tools \
        lld-14 \
        cmake \
        ninja-build \
        git \
        python3 \
        python3-dev \
        python3-pip \
        python3-setuptools \
        automake \
        autoconf \
        libtool \
        pkg-config \
        bison \
        flex \
        gdb \
        strace \
        ltrace \
        vim \
        nano \
        less \
        file \
        bsdmainutils \
        xxd \
        binutils \
        wget \
        curl \
        ca-certificates \
        sudo \
        tmux \
        htop \
        man-db \
        gnuplot-nox \
        libglib2.0-dev \
        libpixman-1-dev \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Symlink versionless tool names (clang/llvm-config/etc) so AFL++'s build can
# find them without a version suffix.
RUN for t in clang clang++ llvm-config llvm-ar llvm-ranlib lld ld.lld; do \
        if [ -x "/usr/bin/$t-14" ] && [ ! -e "/usr/bin/$t" ]; then \
            ln -s "/usr/bin/$t-14" "/usr/bin/$t"; \
        fi; \
    done

# ---------------------------------------------------------------------------
# AFL++ (compile-time instrumentation; works natively on amd64 + arm64)
# Force LLVM_CONFIG so the LLVM mode (afl-clang-fast) really gets built.
# Verify after install: if afl-clang-fast didn't get built, the image is
# broken and we want the docker build itself to fail loudly here.
# ---------------------------------------------------------------------------
RUN git clone --branch "${AFLPP_REF}" --depth=1 \
        https://github.com/AFLplusplus/AFLplusplus.git /opt/AFLplusplus \
 && cd /opt/AFLplusplus \
 && LLVM_CONFIG=llvm-config-14 make all \
 && make install \
 && command -v afl-clang-fast \
 && command -v afl-fuzz \
 && rm -rf /opt/AFLplusplus/.git /opt/AFLplusplus/build_*.log

# ---------------------------------------------------------------------------
# Non-root user
# ---------------------------------------------------------------------------
RUN useradd -m -s /bin/bash -u 1000 student \
 && echo 'student ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \
 && mkdir -p /home/student/work \
 && chown -R student:student /home/student

# ---------------------------------------------------------------------------
# Project files
# ---------------------------------------------------------------------------
COPY --chown=student:student challenges /home/student/challenges
COPY --chown=student:student scripts    /home/student/scripts
COPY --chown=student:student README.md  /home/student/README.md

# Pre-build all challenges as the student user so the image is ready to fuzz.
USER student
WORKDIR /home/student
RUN bash /home/student/scripts/build_all.sh

# AFL++ runtime hints (Docker on macOS has no scaling_governor / core_pattern)
ENV AFL_SKIP_CPUFREQ=1 \
    AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_AUTORESUME=1 \
    AFL_NO_AFFINITY=1

CMD ["/bin/bash", "-l"]
