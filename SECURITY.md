# Security Policy

## Scope

HelixOS is an educational kernel. It runs in ring 0 with no user/kernel separation and is **not** intended for production use. With that caveat, we still care about correctness — memory-safety bugs, IRQ races, and similar issues are bugs worth fixing.

## Supported versions

Only the `main` branch is supported. Tagged releases follow [SemVer](https://semver.org); only the latest minor version of the current major receives fixes.

| Version | Supported |
|---|---|
| 0.1.x | ✅ |
| < 0.1 | ❌ |

## Reporting a vulnerability

Please **do not open a public GitHub issue** for security-sensitive bugs.

Instead:

1. Email the maintainers at the address listed in the repo metadata, OR
2. Use GitHub's "Report a vulnerability" feature in the **Security** tab (private disclosure)

Include:

- A clear description of the issue
- Steps to reproduce (a minimal test case if possible)
- Impact assessment (what does this let an attacker do?)
- Any suggested fix

## Response timeline

| Step | Target time |
|---|---|
| Acknowledge receipt | within 3 days |
| Initial triage + severity assessment | within 7 days |
| Status update | weekly thereafter |
| Coordinated disclosure | within 90 days, or sooner if a fix is ready |

We don't currently have a bug bounty. Acknowledgement in the changelog and (with your permission) in `AUTHORS.md` is offered to first reporters of valid issues.

## What counts as a security issue

In an educational kernel, "security" mostly means:

- Memory corruption (heap overflow, use-after-free, double-free)
- Page-table corruption that breaks isolation guarantees we *do* make
- IRQ-handler races that can be triggered remotely (we don't have a network stack, so this is mostly moot)
- Build supply-chain issues (the Dockerfile, the cross-compiler bootstrap script)

What is **not** in scope:

- Running arbitrary code as ring 0 — the entire kernel runs at ring 0 by design in v0.1
- Reading kernel memory from "user" code — there's no user code; everyone's a kernel thread
- Denial of service from a malicious kernel thread — kernel threads are trusted
- Anything in the `tests/` directory (not shipped)

When user mode lands in v0.2, the scope expands considerably.

## Disclosure policy

When a fix is ready, we:

1. Land the fix in `main`
2. Release a new patch version
3. Update `CHANGELOG.md` with a `Security` entry
4. Credit the reporter (with their permission)
5. Open a public GitHub Security Advisory if the issue is significant

## Past advisories

None yet.
