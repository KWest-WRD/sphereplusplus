/**
 * @file abort.hh
 * @author Matthieu Bucchianeri
 * @brief Generic abort macros.
 *
 * These macros are intended to print a simple backtrace on irrecoverable
 * errors, for example:
 *
 * $ cat test.cc
 * #include "abort.hh"
 *
 * bool very_very_nested()
 * {
 *   AbortIfNot(1 == 0, false);
 *   return true;
 * }
 *
 * bool very_nested()
 * {
 *   AbortIfNot(very_very_nested(), false);
 *   return true;
 * }
 *
 * bool nested()
 * {
 *   AbortIfNot(very_nested(), false);
 *   return true;
 * }
 *
 * int main()
 * {
 *   AbortIfNot(nested(), 1);
 *   return 0;
 * }
 *
 * $ ./test
 * test.cc:5: AbortIfNot(1 == 0)
 * test.cc:11: AbortIfNot(very_very_nested())
 * test.cc:17: AbortIfNot(very_nested())
 * test.cc:23: AbortIfNot(nested())
 */

#pragma once

#include <stdlib.h>

#include <applibs/log.h>

namespace SpherePlusPlus {

/**
 * Internal macro needed for stringification.
 *
 * @param[in] val The value to stringify.
 */
#define __xstringify(val)   #val

/**
 * Stringify its parameter.
 *
 * @param[in] val The value to stringify.
 */
#define __stringify(val)    __xstringify(val)

/**
 * Internal functions to print typed values.
 *
 * @param[in] a The first value.
 * @param[in] b The second value.
 * @{
 */
static inline void __print_values(const int a, const int b)
{
    Log_Debug("%d, %d", a, b);
}

static inline void __print_values(const unsigned int a, const unsigned int b)
{
    Log_Debug("%u, %u", a, b);
}

static inline void __print_values(const float a, const float b)
{
    Log_Debug("%f, %f", a, b);
}
/**
 * @}
 */

/**
 * Assert that a condition is false, or exit from the current function.
 *
 * @param[in] cond The condition to assert "falseness" of.
 * @param[in] ... The return value when the condition is not satisfied.
 */
#define AbortIfNot(cond, ...)                                           \
    do {                                                                \
        if (__builtin_expect(!(cond), 0)) {                             \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortIfNot(" #cond ")\n");                         \
            return __VA_ARGS__;                                         \
        }                                                               \
    } while(0);

/**
 * Assert that a condition is true, or exit from the current function.
 *
 * @param[in] cond The condition to assert.
 * @param[in] ... The return value when the condition is not satisfied.
 *
 * @note See AbortIfNot() for more details.
 */
#define AbortIf(cond, ...)                                              \
    do {                                                                \
        if (__builtin_expect(!!(cond), 0)) {                            \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortIf(" #cond ")\n");                            \
            return __VA_ARGS__;                                         \
        }                                                               \
    } while(0);

/**
 * Assert that two values are equal, or exit from the current function.
 *
 * @param[in] a The first value.
 * @param[in] b The second value.
 * @param[in] ... The return value when the condition is not satisfied.
 *
 * @note See AbortIfNot() for more details.
 */
#define AbortIfNeq(a, b, ...) \
    do {                                                                \
        if (!__builtin_expect((a) == (b), 1)) {                         \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortIfNeq(" #a ", " #b " (values: ");             \
            __print_values(a, b);                                       \
            Log_Debug("))\n");                                          \
            return __VA_ARGS__;                                         \
        }                                                               \
    } while(0);

/**
 * Assert that a value is positive, or exit from the current function. Print the
 * errno string on error.
 *
 * @param[in] cond The condition to assert.
 * @param[in] ... The return value when the condition is not satisfied.
 *
 * @note See AbortIfNot() for more details.
 */
#define AbortErrno(cond, ...)                                           \
    do {                                                                \
        if (__builtin_expect((cond) < 0, 0)) {                          \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortErrno(" #cond "): %m\n");                     \
            return __VA_ARGS__;                                         \
        }                                                               \
    } while(0);

/**
 * Assert that a pointer is non-null, or exit from the current function. Print
 * the errno string on error.
 *
 * @param[in] cond The condition to assert.
 * @param[in] ... The return value when the condition is not satisfied.
 *
 * @note See AbortIfNot() for more details.
 */
#define AbortErrnoPtr(ptr, ...)                                         \
    do {                                                                \
        if (__builtin_expect((ptr) == nullptr, 0)) {                    \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortErrno(" #ptr "): %m\n");                      \
            return __VA_ARGS__;                                         \
        }                                                               \
    } while(0);

/**
 * Assert that a condition is true, or exit the application.
 *
 * @param[in] cond The condition to assert.
 */
#define Assert(cond)                                                    \
    do {                                                                \
        if (__builtin_expect(!(cond), 0)) {                             \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "Assert(" #cond ")\n");                             \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while(0);

} /* namespace SpherePlusPlus */
