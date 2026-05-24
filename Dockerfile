# syntax=docker/dockerfile:1.6
#
# mini-fuzzer-lab - "Build your own AFL"
#
# A teaching container for the System Security course where students
# implement a tiny coverage-guided fuzzer themselves: an LLVM pass that
# instruments every basic block, a coverage runtime that maintains the
# bitmap, and a fuzzer driver (mutation, queue, crash detection).
#
# Build:  docker build -t mini-fuzzer-lab .
# Run:    docker run --rm -it mini-fuzzer-lab
#
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

LABEL org.opencontainers.image.title="mini-fuzzer-lab" \
      org.opencontainers.image.description="Build-your-own coverage-guided fuzzer (LLVM 14)"

# ---------------------------------------------------------------------------
# Toolchain - clang/llvm 14 with the development headers needed for an
# out-of-tree LLVM pass.
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        clang-14 \
        clang-tools-14 \
        clang-format-14 \
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
        gdb \
        strace \
        vim \
        nano \
        less \
        file \
        xxd \
        binutils \
        man-db \
        sudo \
        ca-certificates \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Versionless aliases so course commands match the textbook
RUN for t in clang clang++ llvm-config llvm-ar llvm-ranlib lld ld.lld opt; do \
        if [ -x "/usr/bin/$t-14" ] && [ ! -e "/usr/bin/$t" ]; then \
            ln -s "/usr/bin/$t-14" "/usr/bin/$t"; \
        fi; \
    done

# ---------------------------------------------------------------------------
# Non-root user
# ---------------------------------------------------------------------------
RUN useradd -m -s /bin/bash -u 1000 student \
 && echo 'student ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers \
 && chown -R student:student /home/student

# ---------------------------------------------------------------------------
# Project files
# ---------------------------------------------------------------------------
COPY --chown=student:student pass      /home/student/pass
COPY --chown=student:student runtime   /home/student/runtime
COPY --chown=student:student fuzzer    /home/student/fuzzer
COPY --chown=student:student targets   /home/student/targets
COPY --chown=student:student reference /home/student/reference
COPY --chown=student:student scripts   /home/student/scripts
COPY --chown=student:student README.md /home/student/README.md

USER student
WORKDIR /home/student

# Pre-build the LLVM pass so the cmake config is cached. The student's
# fuzzer source is intentionally NOT pre-compiled - it's full of TODOs
# they need to fill in first.
RUN bash /home/student/scripts/build_pass.sh

CMD ["/bin/bash", "-l"]
