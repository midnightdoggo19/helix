# ─────────────────────────────────────────────────────────────────────
#  HelixOS development container
#  Build:  docker build -t helix-dev -f docker/Dockerfile .
#  Run:    docker run --rm -it -v $(pwd):/helix helix-dev
# ─────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV CROSS_PREFIX=/opt/cross
ENV PATH="${CROSS_PREFIX}/bin:${PATH}"

# Base toolchain + QEMU + GRUB + lint/format tooling
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential nasm \
        qemu-system-x86 \
        grub-pc-bin grub-common xorriso mtools \
        gcc-multilib make wget ca-certificates \
        bison flex \
        libgmp-dev libmpc-dev libmpfr-dev texinfo \
        clang-format cppcheck gdb \
        git python3 \
    && rm -rf /var/lib/apt/lists/*

# Build the i686-elf cross-compiler. Versions pinned for reproducibility.
ARG BINUTILS_VERSION=2.42
ARG GCC_VERSION=13.2.0

RUN mkdir -p /tmp/src && cd /tmp/src \
    && wget -q "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz" \
    && wget -q "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz" \
    && tar xf "binutils-${BINUTILS_VERSION}.tar.gz" \
    && tar xf "gcc-${GCC_VERSION}.tar.gz" \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-${BINUTILS_VERSION}/configure \
          --target=i686-elf --prefix="${CROSS_PREFIX}" \
          --disable-nls --disable-werror --with-sysroot \
    && make -j"$(nproc)" && make install \
    && cd /tmp/src \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-${GCC_VERSION}/configure \
          --target=i686-elf --prefix="${CROSS_PREFIX}" \
          --disable-nls --enable-languages=c \
          --without-headers \
    && make -j"$(nproc)" all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc \
    && cd / && rm -rf /tmp/src

WORKDIR /helix

CMD ["/bin/bash"]
