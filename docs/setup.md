# Setup Guide

Two paths: native build (Linux/macOS) or Docker.

## Path A — Docker (recommended)

The repo ships a Dockerfile that bootstraps the entire toolchain. Easiest for first-time runs.

```bash
# Build the dev image (~10 min the first time; ~10 s thereafter, cached)
docker build -t helix-dev -f docker/Dockerfile .

# Drop into a shell with the source mounted
docker run --rm -it -v "$PWD":/helix helix-dev

# Inside the container
make iso
make run
```

QEMU runs inside the container with `-display none`. Serial output goes to your host terminal. Press Ctrl-A then X (or Ctrl-C) to kill QEMU.

To get a graphical window, run on the host instead (Path B).

## Path B — Native

### Linux (Ubuntu/Debian)

```bash
sudo apt update && sudo apt install -y \
    build-essential nasm qemu-system-x86 \
    grub-pc-bin grub-common xorriso mtools \
    gcc-multilib make wget ca-certificates \
    bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo \
    clang-format cppcheck gdb git

# Build the i686-elf cross compiler (~10 min, once)
./scripts/build-toolchain.sh "$HOME/opt/cross"
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Verify
i686-elf-gcc --version

# Build & run
make iso
make run
```

### macOS (Homebrew)

```bash
brew install nasm qemu xorriso x86_64-elf-gcc i686-elf-gcc \
             clang-format cppcheck

# Optional: install GRUB tools via the `grub2-tools` cask
brew install grub
```

`x86_64-elf-gcc` works as a stand-in for `i686-elf-gcc` if you pass `-m32` consistently, but the dedicated cross-compiler avoids that complication.

### Windows

Use WSL2 with Ubuntu and follow the Linux instructions.

## Verifying the toolchain

```bash
which i686-elf-gcc nasm qemu-system-i386 grub-mkrescue xorriso
# All should print non-empty paths
```

If `grub-mkrescue` fails with "no platform installed," install `grub-pc-bin` (Linux). On macOS this is fiddly — Docker is easier.

## First successful boot

After `make run` you should see:

1. GRUB menu (auto-dismisses in 0 ms)
2. HelixOS ASCII banner in cyan
3. `[ok]` lines for each subsystem
4. `[ READY ] HelixOS Phase 3f -- spawning shell`
5. `helix:/$` prompt

If you don't get there, run `make debug` and connect GDB:

```bash
# In a second terminal:
i686-elf-gdb build/helix.bin -ex "target remote :1234"
(gdb) break kmain
(gdb) continue
```

Step through `kmain` line by line to find where it diverges.

## Common issues

| Symptom | Cause | Fix |
|---|---|---|
| `i686-elf-gcc: command not found` | PATH not updated | `export PATH="$HOME/opt/cross/bin:$PATH"` |
| `grub-mkrescue: no platform` | `grub-pc-bin` missing | `sudo apt install grub-pc-bin` |
| QEMU window stays blank | Boot crashed early | Check `-serial stdio` output |
| `multiboot: invalid magic` | GRUB used Multiboot2 path | Check `boot.asm` header magic = `0x1BADB002` |
| Build error: redefinition of `int_no` | Header included twice | All headers have include guards; if you added one, add yours too |

## Where things live

| Thing | Path |
|---|---|
| Kernel binary | `build/helix.bin` |
| ISO | `build/helix.iso` |
| Toolchain | `$PREFIX/bin/i686-elf-*` (default `~/opt/cross/`) |
| QEMU sample command | `scripts/run-qemu.sh` |
| GDB sample command | `scripts/run-qemu-debug.sh` |
