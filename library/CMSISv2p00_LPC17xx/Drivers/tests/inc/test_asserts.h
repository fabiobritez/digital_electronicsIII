#ifndef TEST_ASSERTS_H
#define TEST_ASSERTS_H

#ifdef UNIT_TESTING_ENABLED

#include <stdbool.h>
#include <stdio.h>

#define TEST_INIT() int __test_passed = 1

#define EXPECT_TRUE(expr)                                                               \
    do {                                                                                \
        if (!(expr)) {                                                                  \
            __test_passed = 0;                                                          \
            printf("EXPECT_TRUE failed in: %s. %s:%d\n", __func__, __FILE__, __LINE__); \
        }                                                                               \
    } while (0)

#define EXPECT_FALSE(expr)                                                               \
    do {                                                                                 \
        if (expr) {                                                                      \
            __test_passed = 0;                                                           \
            printf("EXPECT_FALSE failed in: %s. %s:%d\n", __func__, __FILE__, __LINE__); \
        }                                                                                \
    } while (0)

#define EXPECT_EQUAL(expr, expected)                                                     \
    do {                                                                                 \
        uint32_t __val = (expr);                                                         \
        uint32_t __exp = (expected);                                                     \
        if (__val != __exp) {                                                            \
            __test_passed = 0;                                                           \
            printf("EXPECT_EQUAL failed in: %s. %s:%d\n", __func__, __FILE__, __LINE__); \
            printf("Got: %d Expected: %d\n", __val, __exp);                              \
        }                                                                                \
    } while (0)

#define ASSERT_TEST()                             \
    do {                                          \
        if (__test_passed)                        \
            printf("Test %s passed\n", __func__); \
        else                                      \
            printf("Test %s failed\n", __func__); \
        return __test_passed;                     \
    } while (0)

#endif  // UNIT_TESTING_ENABLED
#endif  // TEST_ASSERTS_H
