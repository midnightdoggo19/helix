/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/kernel/kversion.c
 *  Module:   kernel/version
 *  Brief:    Compile-time version string.
 * ─────────────────────────────────────────────────────────────────────
 */
#include <helix/version.h>

static const char banner[] =
    "HelixOS " HELIX_VERSION_STRING " \"" HELIX_CODENAME "\""
    "  (built " HELIX_BUILD_DATE " " HELIX_BUILD_TIME ")";

const char *helix_banner(void)
{
    return banner;
}
