/**
 * @file application.hh
 * @author Matthieu Bucchianeri
 * @brief Bitmask enums.
 *
 * Taken from:
 * http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
 */

#pragma once

#include <sphereplusplus/std.hh>

namespace SpherePlusPlus {

/**
 * @brief Type trait for an enum that does not support bitmask operators.
 * @tparam T The enum type.
 */
template<typename T>
struct EnableBitMaskOperators
{
    /**
     * Disable bitmask operators by default.
     */
    static const bool enable = false;
};

/**
 * @brief Helper macro to wrap traits to an enum to enable support for bitmask
 *        operators
 * @param[in] T The enum type.
 */
#define ENABLE_BITMASK_OPERATORS(T)  \
template<>                           \
struct EnableBitMaskOperators<T>     \
{                                    \
    static const bool enable = true; \
};

/**
 * @brief Bitwise OR operator for enum types.
 * @tparam T The enum type.
 * @param[in] lhs The lefthand-side value to OR.
 * @param[in] rhs The righthand-side value to OR.
 * @return The OR'ed value.
 */
template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator |(T lhs, T rhs)
{
    return static_cast<T>(static_cast<__underlying_type(T)>(lhs) |
                          static_cast<__underlying_type(T)>(rhs));
}

/**
 * @brief Bitwise AND operator for enum types.
 * @tparam T The enum type.
 * @param[in] lhs The lefthand-side value to AND.
 * @param[in] rhs The righthand-side value to AND.
 * @return The AND'ed value.
 */
template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator &(T lhs, T rhs)
{
    return static_cast<T>(static_cast<__underlying_type(T)>(lhs) &
                          static_cast<__underlying_type(T)>(rhs));
}

/**
 * @brief Bitwise XOR operator for enum types.
 * @tparam T The enum type.
 * @param[in] lhs The lefthand-side value to XOR.
 * @param[in] rhs The righthand-side value to XOR.
 * @return The XOR'ed value.
 */
template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator ^(T lhs, T rhs)
{
    return static_cast<T>(static_cast<__underlying_type(T)>(lhs) ^
                          static_cast<__underlying_type(T)>(rhs));
}

/**
 * @brief Bitwise NOT operator for enum types.
 * @tparam T The enum type.
 * @param[in] rhs The righthand-side value to NOT.
 * @return The bitwise negated (NOT) value.
 */
template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator ~(T rhs)
{
    return static_cast<T>(~static_cast<__underlying_type(T)>(rhs));
}

/**
 * @brief Test for a bit in enum types.
 * @tparam T The enum type.
 * @param[in] lhs The lefthand-side value to test.
 * @param[in] rhs The righthand-side value to test.
 * @return The result of the test.
 */
template<typename T,
         typename std::enable_if_t<EnableBitMaskOperators<T>::enable, T>* =
            nullptr>
bool
isSet(T lhs, T rhs)
{
    return !!(static_cast<__underlying_type(T)>(lhs) &
              static_cast<__underlying_type(T)>(rhs));
}

} /* namespace SpherePlusPlus */
