/*
 * SPDX-License-Identifier: MIT
 *
 * Host-side unit tests for src/lib/string.c
 *
 * Built with -m32 -ffreestanding -fno-builtin so the kernel's size_t
 * (unsigned int on i386) matches what string.c expects. The harness
 * uses host libc for printf, but string.c's symbols clash with libc's
 * — so we compile string.c with a "_kt_" prefix via macros.
 */

/* Rename kernel string symbols to avoid libc clashes during link. */
#define memcpy   _kt_memcpy
#define memmove  _kt_memmove
#define memset   _kt_memset
#define memcmp   _kt_memcmp
#define strlen   _kt_strlen
#define strcmp   _kt_strcmp
#define strncmp  _kt_strncmp
#define strcpy   _kt_strcpy
#define strncpy  _kt_strncpy
#define strchr   _kt_strchr

#include "../../src/lib/string.h"
#include "../../src/lib/string.c"

#undef memcpy
#undef memmove
#undef memset
#undef memcmp
#undef strlen
#undef strcmp
#undef strncmp
#undef strcpy
#undef strncpy
#undef strchr

#include <stdio.h>
#include <string.h>   /* host libc, for the harness */
#include <stdlib.h>

/* ── Tiny harness ───────────────────────────────────────────────────── */
static int _tests_run = 0, _tests_failed = 0;

#define FAIL(msg, ...) do {                                                    \
    fprintf(stderr, "  FAIL %s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    _tests_failed++; return;                                                   \
} while (0)

#define ASSERT_EQ(a, b) do {                                                   \
    _tests_run++;                                                              \
    long _a = (long)(a), _b = (long)(b);                                       \
    if (_a != _b) FAIL("ASSERT_EQ(%s, %s): %ld != %ld", #a, #b, _a, _b);       \
} while (0)

#define ASSERT_TRUE(c) do {                                                    \
    _tests_run++;                                                              \
    if (!(c)) FAIL("ASSERT_TRUE(%s)", #c);                                     \
} while (0)

#define RUN(fn) do { int b = _tests_failed; fn(); if (_tests_failed == b) printf("  ok   %s\n", #fn); } while (0)

/* ── Tests ──────────────────────────────────────────────────────────── */

static void test_strlen_empty(void)  { ASSERT_EQ(_kt_strlen(""), 0); }
static void test_strlen_short(void)  { ASSERT_EQ(_kt_strlen("helix"), 5); }
static void test_strlen_longer(void) { ASSERT_EQ(_kt_strlen("the quick brown fox"), 19); }

static void test_strcmp_equal(void) {
    ASSERT_EQ(_kt_strcmp("abc", "abc"), 0);
    ASSERT_EQ(_kt_strcmp("", ""), 0);
}

static void test_strcmp_ordering(void) {
    ASSERT_TRUE(_kt_strcmp("abc", "abd") < 0);
    ASSERT_TRUE(_kt_strcmp("abd", "abc") > 0);
}

static void test_strncmp(void) {
    ASSERT_EQ(_kt_strncmp("helix", "helio", 4), 0);
    ASSERT_TRUE(_kt_strncmp("helix", "helio", 5) != 0);
}

static void test_memcpy(void) {
    char src[] = "abcdef";
    char dst[8] = {0};
    _kt_memcpy(dst, src, 6);
    ASSERT_EQ(strcmp(dst, "abcdef"), 0);
}

static void test_memset(void) {
    char buf[8];
    _kt_memset(buf, 'x', 7);
    buf[7] = '\0';
    ASSERT_EQ(strcmp(buf, "xxxxxxx"), 0);
}

static void test_memcmp(void) {
    ASSERT_EQ(_kt_memcmp("abcd", "abcd", 4), 0);
    ASSERT_TRUE(_kt_memcmp("abcd", "abce", 4) != 0);
}

int main(void) {
    RUN(test_strlen_empty);
    RUN(test_strlen_short);
    RUN(test_strlen_longer);
    RUN(test_strcmp_equal);
    RUN(test_strcmp_ordering);
    RUN(test_strncmp);
    RUN(test_memcpy);
    RUN(test_memset);
    RUN(test_memcmp);
    printf("\n%d tests run, %d failed\n", _tests_run, _tests_failed);
    return _tests_failed == 0 ? 0 : 1;
}
