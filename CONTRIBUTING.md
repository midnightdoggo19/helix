# Contributing to HelixOS

Thanks for your interest. This document describes how to propose changes effectively.

## Before you start

- For **bugs**, open an issue first using the bug template
- For **small fixes** (typos, doc clarifications, missing `static`), a PR straight to `main` is fine
- For **new features** (drivers, syscalls, scheduler changes), open an issue first to align on scope and design
- For **architectural changes**, write an ADR draft as part of the PR

## Development setup

See [`docs/setup.md`](docs/setup.md). The short version:

```bash
git clone https://github.com/<you>/helix-os.git
cd helix-os
./scripts/build-toolchain.sh ~/opt/cross    # one-time, ~10 min
export PATH="$HOME/opt/cross/bin:$PATH"
make iso && make run
```

Or use Docker:

```bash
docker build -t helix-dev -f docker/Dockerfile .
docker run --rm -it -v "$PWD":/helix helix-dev make run
```

## Code style

The basics:

- C11, freestanding
- 4-space indent, no tabs (except `Makefile`)
- `snake_case` for functions/variables, `UPPER_SNAKE` for macros
- Doxygen-style comments for public functions
- Run `make format` before committing

The `.clang-format` file is authoritative. CI rejects PRs that fail `make format-check`.

## Build/lint

Every PR must pass:

```bash
make            # builds cleanly under -Werror
make format-check
make lint       # cppcheck clean
make test       # host unit tests pass
```

These run in CI on every push.

## Commit messages

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short summary>

<longer body, optional>

<footer with refs or BREAKING CHANGE notes>
```

Common types:

| Type | When to use |
|---|---|
| `feat` | New feature or public API |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Code reorganization without behavior change |
| `perf` | Performance change |
| `test` | Adding or fixing tests |
| `chore` | Build, CI, tooling |

Examples:

```
feat(scheduler): add nice-value priority hints
fix(irq): EOI before invoking handler (ADR-0005)
docs(syscalls): document SYS_MEMINFO
```

## PR checklist

Before opening a PR:

- [ ] Branch is up to date with `main`
- [ ] `make` succeeds with no warnings
- [ ] `make format-check` passes
- [ ] `make lint` is clean
- [ ] `make test` passes
- [ ] If you added an API, documented it in `docs/api/`
- [ ] If you changed behavior, updated `CHANGELOG.md` under `Unreleased`
- [ ] If you locked down a design decision, added an ADR under `docs/design/adr/`
- [ ] Commit messages follow Conventional Commits

## Scope of contributions we want

**Yes, please:**

- Bug fixes (especially with a reproducing test)
- New shell commands
- Additional unit/integration tests
- Documentation improvements
- New device drivers (open an issue first to claim the IRQ)
- Cleanups that reduce LoC without losing clarity

**Discuss first:**

- Architectural changes (scheduler swap, allocator swap, etc.)
- New subsystems (network stack, on-disk FS)
- Build-system changes
- Anything that touches the boot path

**Not in scope for v0.1:**

- User-mode work — coming in v0.2, planned separately
- SMP — v0.3
- x86_64 — v0.5
- New languages (porting parts to Rust, etc.)

## Code of Conduct

By contributing, you agree to abide by the [Code of Conduct](CODE_OF_CONDUCT.md).

## Reporting security issues

Don't open public issues for vulnerabilities. See [SECURITY.md](SECURITY.md).

## License

By contributing you agree your changes are released under the MIT License (same as the rest of the project).
