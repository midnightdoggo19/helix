/*
 * SPDX-License-Identifier: MIT
 *
 * Placeholder for printf format-specifier tests.
 *
 * Full implementation requires extracting the printf format machinery
 * into a standalone unit (currently coupled to kputc → VGA + serial).
 * Stubbed out here so the unit-test pipeline has at least two binaries
 * to demonstrate the harness works.
 */
#include "../harness.h"

static void test_placeholder(void) {
    /* When printf is decoupled, this becomes:
     *   char buf[64];
     *   ksnprintf(buf, sizeof(buf), "%d", 42);
     *   ASSERT_STREQ(buf, "42");
     */
    ASSERT_EQ(1 + 1, 2);
}

int main(void) {
    RUN_TEST(test_placeholder);
    TEST_SUMMARY();
}
