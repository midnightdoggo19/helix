/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     include/helix/version.h
 *  Module:   core/version
 *  Brief:    Kernel version identifiers.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_VERSION_H
#define HELIX_VERSION_H

#define HELIX_VERSION_MAJOR   0
#define HELIX_VERSION_MINOR   1
#define HELIX_VERSION_PATCH   0

#define HELIX_VERSION_STRING  "0.1.0"
#define HELIX_CODENAME        "Genesis"
#define HELIX_BUILD_DATE      __DATE__
#define HELIX_BUILD_TIME      __TIME__

/* Returns a static, null-terminated banner string. */
const char *helix_banner(void);

#endif /* HELIX_VERSION_H */
