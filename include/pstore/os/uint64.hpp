//*        _       _    __   _  _    *
//*  _   _(_)_ __ | |_ / /_ | || |   *
//* | | | | | '_ \| __| '_ \| || |_  *
//* | |_| | | | | | |_| (_) |__   _| *
//*  \__,_|_|_| |_|\__|\___/   |_|   *
//*                                  *
//===- include/pstore/os/uint64.hpp ---------------------------------------===//
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
