//*                  _                      _                 _    *
//*  _   _ _ __  ___(_) __ _ _ __   ___  __| |   ___ __ _ ___| |_  *
//* | | | | '_ \/ __| |/ _` | '_ \ / _ \/ _` |  / __/ _` / __| __| *
//* | |_| | | | \__ \ | (_| | | | |  __/ (_| | | (_| (_| \__ \ |_  *
//*  \__,_|_| |_|___/_|\__, |_| |_|\___|\__,_|  \___\__,_|___/\__| *
//*                    |___/                                       *
//===- include/pstore/support/unsigned_cast.hpp ---------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_SUPPORT_UNSIGNED_CAST_HPP
#define PSTORE_SUPPORT_UNSIGNED_CAST_HPP

#include <cassert>
#include <type_traits>

#include "pstore/support/error.hpp"

namespace pstore {

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    struct unsigned_castable {
        using type = typename std::make_unsigned<typename std::remove_cv<T>::type>::type;
    };

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    constexpr auto unsigned_cast (T const value) noexcept -> typename unsigned_castable<T>::type {
        assert (value >= T{0});
        return static_cast <typename unsigned_castable<T>::type> (value);
    }

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    constexpr auto checked_unsigned_cast (T const value) -> typename unsigned_castable<T>::type {
        if (value < 0) {
            raise (std::errc::invalid_argument, "bad cast to unsigned");
        }
        return unsigned_cast(value);
    }

} // end namespace pstore

#endif //PSTORE_SUPPORT_UNSIGNED_CAST_HPP
