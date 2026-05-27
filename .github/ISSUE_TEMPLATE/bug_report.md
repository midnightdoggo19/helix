---
name: Bug report
about: Report a bug, crash, or unexpected behavior in HelixOS
title: "[BUG] "
labels: bug
assignees: ''
---

## Summary

<!-- One-line description of what's broken. -->

## Environment

- HelixOS version (commit hash or release tag):
- Build mode: native / Docker
- Host OS:
- Toolchain version (`i686-elf-gcc --version`):
- Where it runs: QEMU / VirtualBox / real hardware
- If real hardware, what kind:

## What happened

<!-- Symptoms. If there's a panic banner, paste it (or attach a screenshot). -->

## Expected behavior

<!-- What you thought would happen. -->

## Reproduction steps

1.
2.
3.

## Serial log

<!--
Boot with `-serial stdio` and paste the relevant output below.
The more context, the better.
-->

```
<paste serial output here>
```

## Additional context

<!-- Anything else: screenshots, related issues, hypotheses about the cause. -->

## Have you checked

- [ ] The bug isn't already reported (searched issues)
- [ ] It reproduces on a clean `make clean && make iso && make run`
- [ ] The same issue happens with `-display none -serial stdio` (rules out VGA driver)
