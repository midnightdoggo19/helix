/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/lib/string.h
 *  Module:   lib/string
 *  Brief:    Freestanding subset of <string.h>.
 *
 *  The compiler may emit calls to memcpy/memset/memcmp implicitly
 *  (e.g. for struct assignment), so these must be available even if
 *  the kernel never calls them directly.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_LIB_STRING_H
#define HELIX_LIB_STRING_H

#include <helix/types.h>

void  *memcpy (void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
void  *memset (void *dst, int value, size_t n);
int    memcmp (const void *a, const void *b, size_t n);

size_t strlen (const char *s);
int    strcmp (const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy (char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strchr (const char *s, int c);

#endif /* HELIX_LIB_STRING_H */
