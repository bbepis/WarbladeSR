#pragma once
// test.h: A minimal test framework for the game's tests (tests/README.md).
//
//   TEST(StrHash_matches_java_hash) { CHECK_EQ_INT(StrHash("abc"), 96354); }
//
// Tests register themselves; runner.c runs each in its own forked process, so every test
// starts from the program's initial globals and a crash or hang fails only that test.
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef void (*TestFn)(void);
void TestRegister(const char *name, const char *file, TestFn fn);
void TestFail(const char *file, int line, const char *fmt, ...);

#define TEST(name)                                                                     \
    static void name(void);                                                            \
    __attribute__((constructor)) static void name##_register(void)                     \
    {                                                                                  \
        TestRegister(#name, __FILE__, name);                                           \
    }                                                                                  \
    static void name(void)

// Fails the test (and stops it) unless `cond` holds.
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond))                                                                   \
            TestFail(__FILE__, __LINE__, "CHECK(%s)", #cond);                          \
    } while (0)

#define CHECK_MSG(cond, ...)                                                           \
    do {                                                                               \
        if (!(cond))                                                                   \
            TestFail(__FILE__, __LINE__, __VA_ARGS__);                                 \
    } while (0)

#define CHECK_EQ_INT(actual, expected)                                                 \
    do {                                                                               \
        long long a_ = (long long)(actual), e_ = (long long)(expected);                \
        if (a_ != e_)                                                                  \
            TestFail(__FILE__, __LINE__, "%s == %lld, expected %s == %lld", #actual,  \
                     a_, #expected, e_);                                               \
    } while (0)

#define CHECK_NE_INT(actual, unexpected)                                               \
    do {                                                                               \
        long long a_ = (long long)(actual), e_ = (long long)(unexpected);              \
        if (a_ == e_)                                                                  \
            TestFail(__FILE__, __LINE__, "%s == %lld, expected anything else", #actual, \
                     a_);                                                              \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                        \
    do {                                                                               \
        double a_ = (double)(actual), e_ = (double)(expected);                         \
        if (!(fabs(a_ - e_) <= (tolerance)))                                           \
            TestFail(__FILE__, __LINE__, "%s == %g, expected %g (+-%g)", #actual, a_,  \
                     e_, (double)(tolerance));                                         \
    } while (0)

#define CHECK_STR(actual, expected)                                                    \
    do {                                                                               \
        const char *a_ = (actual), *e_ = (expected);                                   \
        if (!a_ || strcmp(a_, e_) != 0)                                                \
            TestFail(__FILE__, __LINE__, "%s == \"%s\", expected \"%s\"", #actual,     \
                     a_ ? a_ : "(null)", e_);                                          \
    } while (0)

#define CHECK_MEM(actual, expected, size)                                              \
    do {                                                                               \
        if (memcmp((actual), (expected), (size)) != 0)                                 \
            TestFail(__FILE__, __LINE__, "%s differs from %s", #actual, #expected);   \
    } while (0)
