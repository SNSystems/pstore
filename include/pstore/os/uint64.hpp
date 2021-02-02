//*        _       _    __   _  _    *
//*  _   _(_)_ __ | |_ / /_ | || |   *
//* | | | | | '_ \| __| '_ \| || |_  *
//* | |_| | | | | | |_| (_) |__   _| *
//*  \__,_|_|_| |_|\__|\___/   |_|   *
//*                                  *
//===- include/pstore/os/uint64.hpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file uint64.hpp
/// \brief Convenience functions for breaking up a uint64_t value for use with Win32 API functions.

#ifndef PSTORE_OS_UINT64_HPP
#define PSTORE_OS_UINT64_HPP

#include <cstdint>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include "pstore/support/portab.hpp"

namespace pstore {

#ifdef _WIN32
    PSTORE_STATIC_ASSERT (sizeof (DWORD) == sizeof (std::uint32_t));
    PSTORE_STATIC_ASSERT (std::is_unsigned<DWORD>::value);
#endif

    constexpr auto uint64_high4 (std::uint64_t const v) noexcept -> std::uint32_t {
        return static_cast<std::uint32_t> (v >> 32U);
    }
    constexpr auto uint64_low4 (std::uint64_t const v) noexcept -> std::uint32_t {
        return static_cast<std::uint32_t> (v & ((std::uint64_t{1} << 32U) - 1U));
    }

} // end namespace pstore

#endif // PSTORE_OS_UINT64_HPP
