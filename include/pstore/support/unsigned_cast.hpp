//===- include/pstore/support/unsigned_cast.hpp -----------*- mode: C++ -*-===//
//*                  _                      _                 _    *
//*  _   _ _ __  ___(_) __ _ _ __   ___  __| |   ___ __ _ ___| |_  *
//* | | | | '_ \/ __| |/ _` | '_ \ / _ \/ _` |  / __/ _` / __| __| *
//* | |_| | | | \__ \ | (_| | | | |  __/ (_| | | (_| (_| \__ \ |_  *
//*  \__,_|_| |_|___/_|\__, |_| |_|\___|\__,_|  \___\__,_|___/\__| *
//*                    |___/                                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
        PSTORE_ASSERT (value >= SrcT{0});
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
