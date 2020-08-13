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
 * Assert that a condition is false, or exit from the current function.
 *
 * @param[in] cond The condition to assert "falseness" of.
 * @param[in] ... The return value when the condition is not satisfied.
 */
#define AbortIfNot(cond, ...)                                           \
    do                                                                  \
    {                                                                   \
        if (!(cond))                                                    \
        {                                                               \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortIfNot(" #cond ")\n");                         \
            return __VA_ARGS__;                                         \
        }                                                               \
    }                                                                   \
    while(0);

/**
 * Assert that a condition is true, or exit from the current function.
 *
 * @param[in] cond The condition to assert.
 * @param[in] ... The return value when the condition is not satisfied.
 *
 * @note See AbortIfNot() for more details.
 */
#define AbortIf(cond, ...)                                              \
    do                                                                  \
    {                                                                   \
        if ((cond))                                                     \
        {                                                               \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortIf(" #cond ")\n");                            \
            return __VA_ARGS__;                                         \
        }                                                               \
    }                                                                   \
    while(0);

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
    do                                                                  \
    {                                                                   \
        if ((cond) < 0)                                                 \
        {                                                               \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortErrno(" #cond "): %m\n");                     \
            return __VA_ARGS__;                                         \
        }                                                               \
    }                                                                   \
    while(0);

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
    do                                                                  \
    {                                                                   \
        if (!(ptr))                                                     \
        {                                                               \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "AbortErrno(" #ptr "): %m\n");                      \
            return __VA_ARGS__;                                         \
        }                                                               \
    }                                                                   \
    while(0);

/**
 * Assert that a condition is true, or exit the application.
 *
 * @param[in] cond The condition to assert.
 */
#define Assert(cond)                                                    \
    do                                                                  \
    {                                                                   \
        if (!(cond))                                                    \
        {                                                               \
            Log_Debug(__FILE__ ":" __stringify(__LINE__) ": "           \
                    "Assert(" #cond ")\n");                             \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    }                                                                   \
    while(0);
