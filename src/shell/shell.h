/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/shell/shell.h
 *  Module:   shell
 *  Brief:    Interactive built-in shell (reads stdin, parses, runs).
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_SHELL_H
#define HELIX_SHELL_H

/** Shell entry point. Suitable for use as a task_entry_t. */
void shell_main(void);

#endif /* HELIX_SHELL_H */
