//*                  _                      _                 _    *
//*  _   _ _ __  ___(_) __ _ _ __   ___  __| |   ___ __ _ ___| |_  *
//* | | | | '_ \/ __| |/ _` | '_ \ / _ \/ _` |  / __/ _` / __| __| *
//* | |_| | | | \__ \ | (_| | | | |  __/ (_| | | (_| (_| \__ \ |_  *
//*  \__,_|_| |_|___/_|\__, |_| |_|\___|\__,_|  \___\__,_|___/\__| *
//*                    |___/                                       *
//===- include/pstore/support/unsigned_cast.hpp ---------------------------===//
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
/// \file unsigned_cast.hpp
/// \brief unsigned_cast() (and its runtime-checked version) allow for simple integral unsigned
///  casts.
#ifndef PSTORE_SUPPORT_UNSIGNED_CAST_HPP
#define PSTORE_SUPPORT_UNSIGNED_CAST_HPP

#include "pstore/support/error.hpp"

namespace pstore {

    template <typename SrcT, typename DestT = std::make_unsigned_t<typename std::remove_cv_t<SrcT>>,
              typename = typename std::enable_if<std::is_integral<SrcT>::value &&
                                                 std::is_unsigned<DestT>::value>::type>
    constexpr DestT unsigned_cast (SrcT const value) noexcept {
        static_assert (
            std::numeric_limits<DestT>::max () >=
                std::numeric_limits<std::make_unsigned_t<typename std::remove_cv_t<SrcT>>>::max (),
            "DestT cannot hold all of the values of SrcT");
        assert (value >= SrcT{0});
        return static_cast<DestT> (value);
    }

    template <typename SrcT, typename DestT = std::make_unsigned_t<typename std::remove_cv_t<SrcT>>,
              typename = typename std::enable_if<std::is_integral<SrcT>::value &&
                                                 std::is_unsigned<DestT>::value>::type>
    constexpr DestT checked_unsigned_cast (SrcT const value) {
        if (value < SrcT{0}) {
            raise (std::errc::invalid_argument, "bad cast to unsigned");
        }
        return unsigned_cast<SrcT, DestT> (value);
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_UNSIGNED_CAST_HPP
