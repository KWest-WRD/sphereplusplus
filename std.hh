/**
 * @file application.hh
 * @author Matthieu Bucchianeri
 * @brief Standard library substitutions.
 */

#pragma once

namespace std
{
    /*
     * std::enable_if
     *
     * Taken from:
     * https://eli.thegreenplace.net/2014/sfinae-and-enable_if/
     */
    template <bool, typename T = void>
    struct enable_if
    {};

    template <typename T>
    struct enable_if<true, T> {
        typedef T type;
    };

    template <bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;
}
