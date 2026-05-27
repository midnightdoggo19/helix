#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
#  build-toolchain.sh — Build i686-elf-gcc cross compiler from source.
#
#  Usage:  ./scripts/build-toolchain.sh [PREFIX]
#  Default PREFIX: $HOME/opt/cross
#  Takes ~10 minutes on a modern machine. Idempotent — re-runs are quick
#  if the toolchain is already built.
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

PREFIX="${1:-$HOME/opt/cross}"
BINUTILS_VERSION="2.42"
GCC_VERSION="13.2.0"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu)"

if [ -x "${PREFIX}/bin/i686-elf-gcc" ]; then
    echo "i686-elf-gcc already exists at ${PREFIX}/bin/. Nothing to do."
    exit 0
fi

echo "Building cross-compiler at ${PREFIX} (${JOBS} parallel jobs)..."

WORK="$(mktemp -d)"
trap "rm -rf ${WORK}" EXIT

cd "${WORK}"
mkdir -p "${PREFIX}"
export PATH="${PREFIX}/bin:${PATH}"

echo "── Fetching binutils ${BINUTILS_VERSION} ──"
wget -q "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz"
tar xf "binutils-${BINUTILS_VERSION}.tar.gz"

echo "── Building binutils ──"
mkdir build-binutils && cd build-binutils
../binutils-${BINUTILS_VERSION}/configure \
    --target=i686-elf \
    --prefix="${PREFIX}" \
    --disable-nls --disable-werror --with-sysroot
make -j"${JOBS}"
make install
cd ..

echo "── Fetching gcc ${GCC_VERSION} ──"
wget -q "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz"
tar xf "gcc-${GCC_VERSION}.tar.gz"

echo "── Building gcc ──"
mkdir build-gcc && cd build-gcc
../gcc-${GCC_VERSION}/configure \
    --target=i686-elf \
    --prefix="${PREFIX}" \
    --disable-nls --enable-languages=c \
    --without-headers
make -j"${JOBS}" all-gcc all-target-libgcc
make install-gcc install-target-libgcc

echo
echo "── Done. Toolchain installed to ${PREFIX} ──"
echo "Add to your shell profile:"
echo "    export PATH=\"${PREFIX}/bin:\$PATH\""
