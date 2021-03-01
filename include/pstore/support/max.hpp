//===- include/pstore/support/max.hpp ---------------------*- mode: C++ -*-===//
//*                        *
//*  _ __ ___   __ ___  __ *
//* | '_ ` _ \ / _` \ \/ / *
//* | | | | | | (_| |>  <  *
//* |_| |_| |_|\__,_/_/\_\ *
//*                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file max.hpp
/// \brief A template implementation of helper types for determining the maximum size and alignment
/// of a collection of types.
#ifndef PSTORE_SUPPORT_MAX_HPP
#define PSTORE_SUPPORT_MAX_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace pstore {
    namespace details {

        // size
        // ~~~~
        struct size {
            template <typename T>
            static constexpr std::size_t value () {
                return sizeof (T);
            }
        };

        // align
        // ~~~~~
        struct align {
            template <typename T>
            static constexpr std::size_t value () {
                return alignof (T);
            }
        };

    } // end namespace details

    // maxof
    // ~~~~~
    template <typename TypeValue, typename... T>
    struct maxof;
    template <typename TypeValue>
    struct maxof<TypeValue> {
        static constexpr auto value = std::size_t{1};
    };
    template <typename TypeValue, typename Head, typename... Tail>
    struct maxof<TypeValue, Head, Tail...> {
        static constexpr auto value =
            std::max (TypeValue::template value<Head> (), maxof<TypeValue, Tail...>::value);
    };

    // characteristics
    // ~~~~~~~~~~~~~~~
    /// Given a list of types, find the size of the largest and the alignment of the most aligned.
    template <typename... T>
    struct characteristics {
        static constexpr std::size_t size = maxof<details::size, T...>::value;
        static constexpr std::size_t align = maxof<details::align, T...>::value;
    };

} // end namespace pstore

static_assert (pstore::maxof<pstore::details::size, std::uint_least8_t>::value ==
                   sizeof (std::uint_least8_t),
               "max(sizeof(1)) != sizeof(1)");
static_assert (
    pstore::maxof<pstore::details::size, std::uint_least8_t, std::uint_least16_t>::value >=
        sizeof (std::uint_least16_t),
    "max(sizeof(1),sizeof(2) != 2");

#endif // PSTORE_SUPPORT_MAX_HPP
