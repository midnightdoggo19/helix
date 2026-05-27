/*
 * SPDX-License-Identifier: MIT
 *
 * Tiny test harness — compile-time assertions for host-side unit tests.
 * No frameworks; if something fails, we print and abort. Goals: zero
 * dependencies, runs anywhere with a C compiler.
 */
#ifndef HELIX_TEST_HARNESS_H
#define HELIX_TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _tests_run    = 0;
static int _tests_failed = 0;

#define TEST_FAIL(msg, ...) do {                                          \
    fprintf(stderr, "  FAIL %s:%d: " msg "\n",                            \
            __FILE__, __LINE__, ##__VA_ARGS__);                           \
    _tests_failed++;                                                      \
    return;                                                               \
} while (0)

#define ASSERT(cond) do {                                                 \
    _tests_run++;                                                         \
    if (!(cond)) TEST_FAIL("ASSERT(%s)", #cond);                          \
} while (0)

#define ASSERT_EQ(a, b) do {                                              \
    _tests_run++;                                                         \
    long _a = (long)(a), _b = (long)(b);                                  \
    if (_a != _b) TEST_FAIL("ASSERT_EQ(%s, %s): %ld != %ld",              \
                            #a, #b, _a, _b);                              \
} while (0)

#define ASSERT_STREQ(a, b) do {                                           \
    _tests_run++;                                                         \
    if (strcmp((a), (b)) != 0)                                            \
        TEST_FAIL("ASSERT_STREQ(\"%s\", \"%s\")", (a), (b));              \
} while (0)

#define RUN_TEST(fn) do {                                                 \
    int before = _tests_failed;                                           \
    fn();                                                                 \
    if (_tests_failed == before)                                          \
        printf("  ok   %s\n", #fn);                                       \
} while (0)

#define TEST_SUMMARY() do {                                               \
    printf("\n%d tests run, %d failed\n", _tests_run, _tests_failed);     \
    return _tests_failed == 0 ? 0 : 1;                                    \
} while (0)

#endif /* HELIX_TEST_HARNESS_H */
