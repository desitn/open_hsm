/*
 * Unity - Minimal Test Framework for C
 * Simplified version for OSA testing
 * Multi-platform support with output abstraction
 */

#ifndef UNITY_H
#define UNITY_H

#include <string.h>
#include <math.h>

/* Platform abstraction for output */
#ifdef OSA_PLATFORM
    /* Use OSA platform logging if available */
    #include "osa_platform.h"
    #define UNITY_PRINT(msg)    osa_log msg
#else
    /* Standard output for Linux/Unix platforms */
    #include <stdio.h>
    #define UNITY_PRINT(msg)    printf msg
#endif

/* Unity output macros for multi-platform support */
#define UNITY_PRINT_CHAR(c)     UNITY_PRINT(("%c", c))
#define UNITY_PRINT_STR(s)      UNITY_PRINT(("%s", s))
#define UNITY_PRINT_INT(i)      UNITY_PRINT(("%d", i))
#define UNITY_PRINT_UINT(u)     UNITY_PRINT(("%u", u))
#define UNITY_PRINT_HEX(h)      UNITY_PRINT(("0x%X", h))
#define UNITY_PRINT_FLOAT(f)    UNITY_PRINT(("%f", f))
#define UNITY_PRINT_NEWLINE()   UNITY_PRINT(("\n"))

/* Test result tracking */
extern int unity_tests_run;
extern int unity_tests_failed;
extern int unity_current_test_failed;

/* Unity Begin/End */
#define UNITY_BEGIN() do { \
    unity_tests_run = 0; \
    unity_tests_failed = 0; \
} while(0)

#define UNITY_END() unity_end()

static inline int unity_end(void) {
    UNITY_PRINT(("\n-----------------------\n"));
    UNITY_PRINT(("%d Tests %d Failures 0 Ignored\n",
           unity_tests_run, unity_tests_failed));
    UNITY_PRINT(("-----------------------\n"));
    return unity_tests_failed;
}

/* Test runner */
#define RUN_TEST(func) do { \
    unity_current_test_failed = 0; \
    UNITY_PRINT(("  RUN: " #func " ... ")); \
    func(); \
    unity_tests_run++; \
    if (unity_current_test_failed) { \
        unity_tests_failed++; \
        UNITY_PRINT(("FAIL\n")); \
    } else { \
        UNITY_PRINT(("PASS\n")); \
    } \
} while(0)

/* Assertions */
#define TEST_ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        unity_current_test_failed = 1; \
        UNITY_PRINT(("\n    ASSERTION FAILED: " #condition " is FALSE ")); \
    } \
} while(0)

#define TEST_ASSERT_FALSE(condition) do { \
    if (condition) { \
        unity_current_test_failed = 1; \
        UNITY_PRINT(("\n    ASSERTION FAILED: " #condition " is TRUE ")); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL(expected, actual) do { \
    if ((expected) != (actual)) { \
        unity_current_test_failed = 1; \
        UNITY_PRINT(("\n    ASSERTION FAILED: Expected %d, got %d ", \
               (int)(expected), (int)(actual))); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        unity_current_test_failed = 1; \
        UNITY_PRINT(("\n    ASSERTION FAILED: Expected \"%s\", got \"%s\" ", \
               (expected), (actual))); \
    } \
} while(0)

#define TEST_ASSERT_NULL(pointer) do { \
    if ((pointer) != NULL) { \
        unity_current_test_failed = 1; \
        UNITY_PRINT(("\n    ASSERTION FAILED: Pointer is not NULL ")); \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(pointer) do { \
    if ((pointer) == NULL) { \
        unity_current_test_failed = 1; \
        UNITY_PRINT(("\n    ASSERTION FAILED: Pointer is NULL ")); \
    } \
} while(0)

/* Function prototypes */
void setUp(void);
void tearDown(void);

#endif /* UNITY_H */
