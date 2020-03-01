//*                        *
//*  _ __ ___   __ ___  __ *
//* | '_ ` _ \ / _` \ \/ / *
//* | | | | | | (_| |>  <  *
//* |_| |_| |_|\__,_/_/\_\ *
//*                        *
//===- include/pstore/support/max.hpp -------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
/// \brief A template implementation of helper types for determining the maximum size and alignment
/// of a collection of types.
#ifndef PSTORE_SUPPORT_MAX_HPP
#define PSTORE_SUPPORT_MAX_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace pstore {
    namespace details {

        // max_sizeof
        // ~~~~~~~~~~
        template <typename... T>
        struct max_sizeof;
        template <>
        struct max_sizeof<> {
            static constexpr auto value = std::size_t{1};
        };
        template <typename Head, typename... Tail>
        struct max_sizeof<Head, Tail...> {
            static constexpr auto value = std::max (sizeof (Head), max_sizeof<Tail...>::value);
        };

        // max_alignment
        // ~~~~~~~~~~~~~
        template <typename... T>
        struct max_alignment;
        template <>
        struct max_alignment<> {
            static constexpr auto value = std::size_t{1};
        };
        template <typename Head, typename... Tail>
        struct max_alignment<Head, Tail...> {
            static constexpr auto value = std::max (alignof (Head), max_alignment<Tail...>::value);
        };

    } // end namespace details

    // characteristics
    // ~~~~~~~~~~~~~~~
    /// Given a list of types, find the size of the largest and the alignment of the most aligned.
    template <typename... T>
    struct characteristics {
        static constexpr std::size_t size = details::max_sizeof<T...>::value;
        static constexpr std::size_t align = details::max_alignment<T...>::value;
    };

} // end namespace pstore

static_assert (pstore::details::max_sizeof<std::uint_least8_t>::value ==
                   sizeof (std::uint_least8_t),
               "max(sizeof(1)) != sizeof(1)");
static_assert (pstore::details::max_sizeof<std::uint_least8_t, std::uint_least16_t>::value >=
                   sizeof (std::uint_least16_t),
               "max(sizeof(1),sizeof(2) != 2");

#endif // PSTORE_SUPPORT_MAX_HPP
