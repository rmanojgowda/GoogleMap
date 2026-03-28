#pragma once
// ============================================================
//  TestFramework.h  —  Minimal Google Test-compatible harness
// ============================================================
//
//  WHY THIS EXISTS:
//  Google Test (GTest) is the industry standard at Google.
//  This header mimics the exact same macros:
//    EXPECT_EQ, EXPECT_NEAR, EXPECT_TRUE, EXPECT_FALSE,
//    EXPECT_THROW, TEST(Suite, Name)
//
//  On your Windows machine you can swap this out for real GTest
//  by installing it and changing the include — zero test code
//  changes needed.
//
//  HOW TO SWITCH TO REAL GTEST (on your machine):
//    1. Install: vcpkg install gtest
//    2. In CMakeLists.txt add:
//         find_package(GTest REQUIRED)
//         target_link_libraries(gmap_tests GTest::gtest_main)
//    3. Delete this file — the macros below already match GTest.
// ============================================================

#include <iostream>
#include <string>
#include <cmath>
#include <functional>
#include <vector>
#include <stdexcept>

// ── Colour codes ──────────────────────────────────────────────
#define T_GREEN "\033[32m"
#define T_RED "\033[31m"
#define T_YELLOW "\033[33m"
#define T_CYAN "\033[36m"
#define T_BOLD "\033[1m"
#define T_RESET "\033[0m"

// ── Global test registry ──────────────────────────────────────
struct TestCase
{
    std::string suite;
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase> &testRegistry()
{
    static std::vector<TestCase> reg;
    return reg;
}

inline int &failCount()
{
    static int n = 0;
    return n;
}
inline int &passCount()
{
    static int n = 0;
    return n;
}
inline std::string &currentTest()
{
    static std::string s;
    return s;
}

// ── TEST macro — registers a test function ────────────────────
#define TEST(Suite, Name)                                    \
    void Suite##_##Name##_fn();                              \
    struct Suite##_##Name##_Reg                              \
    {                                                        \
        Suite##_##Name##_Reg()                               \
        {                                                    \
            testRegistry().push_back({#Suite, #Name,         \
                                      Suite##_##Name##_fn}); \
        }                                                    \
    } Suite##_##Name##_reg_instance;                         \
    void Suite##_##Name##_fn()

// ── Assertion macros ──────────────────────────────────────────
#define EXPECT_EQ(a, b)                                      \
    do                                                       \
    {                                                        \
        if ((a) == (b))                                      \
        {                                                    \
            ++passCount();                                   \
        }                                                    \
        else                                                 \
        {                                                    \
            ++failCount();                                   \
            std::cout << T_RED "    FAIL " T_RESET           \
                      << __FILE__ << ":" << __LINE__ << "\n" \
                      << "      Expected: " << (b) << "\n"   \
                      << "      Actual  : " << (a) << "\n";  \
        }                                                    \
    } while (0)

#define EXPECT_NE(a, b)                                          \
    do                                                           \
    {                                                            \
        if ((a) != (b))                                          \
        {                                                        \
            ++passCount();                                       \
        }                                                        \
        else                                                     \
        {                                                        \
            ++failCount();                                       \
            std::cout << T_RED "    FAIL " T_RESET               \
                      << __FILE__ << ":" << __LINE__             \
                      << " — expected values to differ, both = " \
                      << (a) << "\n";                            \
        }                                                        \
    } while (0)

#define EXPECT_TRUE(expr)                                 \
    do                                                    \
    {                                                     \
        if (expr)                                         \
        {                                                 \
            ++passCount();                                \
        }                                                 \
        else                                              \
        {                                                 \
            ++failCount();                                \
            std::cout << T_RED "    FAIL " T_RESET        \
                      << __FILE__ << ":" << __LINE__      \
                      << " — expected TRUE: " #expr "\n"; \
        }                                                 \
    } while (0)

#define EXPECT_FALSE(expr)                                 \
    do                                                     \
    {                                                      \
        if (!(expr))                                       \
        {                                                  \
            ++passCount();                                 \
        }                                                  \
        else                                               \
        {                                                  \
            ++failCount();                                 \
            std::cout << T_RED "    FAIL " T_RESET         \
                      << __FILE__ << ":" << __LINE__       \
                      << " — expected FALSE: " #expr "\n"; \
        }                                                  \
    } while (0)

// EXPECT_NEAR: for floating point — within epsilon
#define EXPECT_NEAR(a, b, eps)                               \
    do                                                       \
    {                                                        \
        if (std::fabs((double)(a) - (double)(b)) <= (eps))   \
        {                                                    \
            ++passCount();                                   \
        }                                                    \
        else                                                 \
        {                                                    \
            ++failCount();                                   \
            std::cout << T_RED "    FAIL " T_RESET           \
                      << __FILE__ << ":" << __LINE__ << "\n" \
                      << "      Expected: " << (b)           \
                      << " ± " << (eps) << "\n"              \
                      << "      Actual  : " << (a) << "\n";  \
        }                                                    \
    } while (0)

// EXPECT_LT / EXPECT_GT
#define EXPECT_LT(a, b)                                        \
    do                                                         \
    {                                                          \
        if ((a) < (b))                                         \
        {                                                      \
            ++passCount();                                     \
        }                                                      \
        else                                                   \
        {                                                      \
            ++failCount();                                     \
            std::cout << T_RED "    FAIL " T_RESET             \
                      << __FILE__ << ":" << __LINE__           \
                      << " — expected " << (a) << " < " << (b) \
                      << "\n";                                 \
        }                                                      \
    } while (0)

#define EXPECT_GT(a, b)                                        \
    do                                                         \
    {                                                          \
        if ((a) > (b))                                         \
        {                                                      \
            ++passCount();                                     \
        }                                                      \
        else                                                   \
        {                                                      \
            ++failCount();                                     \
            std::cout << T_RED "    FAIL " T_RESET             \
                      << __FILE__ << ":" << __LINE__           \
                      << " — expected " << (a) << " > " << (b) \
                      << "\n";                                 \
        }                                                      \
    } while (0)

// EXPECT_THROW: verify exception is thrown
#define EXPECT_THROW(stmt, ExcType)                                 \
    do                                                              \
    {                                                               \
        bool threw = false;                                         \
        try                                                         \
        {                                                           \
            stmt;                                                   \
        }                                                           \
        catch (const ExcType &)                                     \
        {                                                           \
            threw = true;                                           \
        }                                                           \
        catch (...)                                                 \
        {                                                           \
        }                                                           \
        if (threw)                                                  \
        {                                                           \
            ++passCount();                                          \
        }                                                           \
        else                                                        \
        {                                                           \
            ++failCount();                                          \
            std::cout << T_RED "    FAIL " T_RESET                  \
                      << __FILE__ << ":" << __LINE__                \
                      << " — expected " #ExcType " to be thrown\n"; \
        }                                                           \
    } while (0)

// ── Test runner ───────────────────────────────────────────────
inline int RUN_ALL_TESTS()
{
    std::string lastSuite;
    for (auto &tc : testRegistry())
    {
        if (tc.suite != lastSuite)
        {
            std::cout << "\n"
                      << T_BOLD << T_CYAN
                      << "  [ " << tc.suite << " ]\n"
                      << T_RESET;
            lastSuite = tc.suite;
        }
        std::cout << T_YELLOW << "  ▶ " << T_RESET << tc.name << " ... ";
        int failsBefore = failCount();
        try
        {
            tc.fn();
        }
        catch (const std::exception &e)
        {
            ++failCount();
            std::cout << T_RED << "\n    EXCEPTION: " << e.what() << T_RESET;
        }
        if (failCount() == failsBefore)
            std::cout << T_GREEN << "PASS" << T_RESET << "\n";
        else
            std::cout << T_RED << "FAIL" << T_RESET << "\n";
    }

    int total = passCount() + failCount();
    std::cout << "\n"
              << T_BOLD;
    if (failCount() == 0)
        std::cout << T_GREEN << "  ✅  ALL " << total
                  << " assertions passed.\n"
                  << T_RESET;
    else
        std::cout << T_RED << "  ❌  " << failCount()
                  << " FAILED / " << total << " total\n"
                  << T_RESET;
    std::cout << "\n";
    return failCount() == 0 ? 0 : 1;
}