//*  _     _ _                           _    *
//* | |__ (_) |_    ___ ___  _   _ _ __ | |_  *
//* | '_ \| | __|  / __/ _ \| | | | '_ \| __| *
//* | |_) | | |_  | (_| (_) | |_| | | | | |_  *
//* |_.__/|_|\__|  \___\___/ \__,_|_| |_|\__| *
//*                                           *
//===- include/pstore/support/bit_count.hpp -------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
/// \file bit_count.hpp
/// \brief Implements portable functions for bit twiddling operations including counting leading and
/// trailing zero bits as well as population count.

#ifndef PSTORE_SUPPORT_BIT_COUNT_HPP
#define PSTORE_SUPPORT_BIT_COUNT_HPP

#ifdef _MSC_VER
#    include <intrin.h>
#endif

#include "pstore/support/uint128.hpp"

namespace pstore {
    namespace bit_count {

        //*         _        *
        //*   ___  | |  ____ *
        //*  / __| | | |_  / *
        //* | (__  | |  / /  *
        //*  \___| |_| /___| *
        //*                  *
        ///@{
        /// Count the number of contiguous zero bits starting from the MSB.
        /// It is undefined behavior if x is 0.

#ifdef _MSC_VER
        inline unsigned clz (std::uint32_t const x) noexcept {
            PSTORE_ASSERT (x != 0);
            unsigned long bit_position = 0;
            _BitScanReverse (&bit_position, x);
            PSTORE_ASSERT (bit_position < 32);
            return 31 - bit_position;
        }
        inline unsigned clz (std::uint64_t const x) noexcept {
            PSTORE_ASSERT (x != 0);
            unsigned long bit_position = 0;
            _BitScanReverse64 (&bit_position, x);
            PSTORE_ASSERT (bit_position < 64);
            return 63 - bit_position;
        }
        inline unsigned clz (std::uint8_t const x) noexcept {
            unsigned const r = clz (static_cast<std::uint32_t> (x));
            PSTORE_ASSERT (r >= 24U);
            return r - 24U;
        }
        inline unsigned clz (std::uint16_t const x) noexcept {
            unsigned const r = clz (static_cast<std::uint32_t> (x));
            PSTORE_ASSERT (r >= 16U);
            return r - 16U;
        }
        inline unsigned clz (uint128 const x) noexcept {
            return x.high () != 0U ? clz (x.high ()) : 64U + clz (x.low ());
        }
#else
        constexpr unsigned clz (unsigned const x) noexcept {
            PSTORE_STATIC_ASSERT (sizeof (x) == sizeof (std::uint32_t));
            PSTORE_ASSERT (x != 0);
            return static_cast<unsigned> (__builtin_clz (x));
        }
        constexpr unsigned clz (unsigned long const x) noexcept {
            PSTORE_STATIC_ASSERT (sizeof (x) == sizeof (std::uint64_t));
            PSTORE_ASSERT (x != 0);
            return static_cast<unsigned> (__builtin_clzl (x));
        }
        constexpr unsigned clz (unsigned long long const x) noexcept {
            PSTORE_STATIC_ASSERT (sizeof (x) == sizeof (std::uint64_t));
            PSTORE_ASSERT (x != 0);
            return static_cast<unsigned> (__builtin_clzll (x));
        }
        constexpr unsigned clz (std::uint8_t const x) noexcept {
            unsigned const r = clz (static_cast<std::uint32_t> (x));
            PSTORE_ASSERT (r >= 24U);
            return r - 24U;
        }
        constexpr unsigned clz (std::uint16_t const x) noexcept {
            unsigned const r = clz (static_cast<std::uint32_t> (x));
            PSTORE_ASSERT (r >= 16U);
            return r - 16U;
        }
        constexpr unsigned clz (uint128 const x) noexcept {
            return x.high () != 0U ? clz (x.high ()) : 64U + clz (x.low ());
        }
#endif //_MSC_VER
       ///@}

        //*         _          *
        //*   ___  | |_   ____ *
        //*  / __| | __| |_  / *
        //* | (__  | |_   / /  *
        //*  \___|  \__| /___| *
        //*                    *
        ///@{
        /// Count the  number of contiguous zero bits starting from the LSB.
        /// It is undefined behavior if x is 0.
#ifdef _MSC_VER
        inline unsigned ctz (std::uint64_t const x) noexcept {
            PSTORE_ASSERT (x != 0);
            unsigned long bit_position = 0;
            _BitScanForward64 (&bit_position, x);
            PSTORE_ASSERT (bit_position < 64);
            return bit_position;
        }
        inline unsigned ctz (uint128 const x) noexcept {
            PSTORE_ASSERT (x != 0U);
            return x.low () == 0U ? 64U + ctz (x.high ()) : ctz (x.low ());
        }
#else
        constexpr unsigned ctz (unsigned long long const x) noexcept {
            static_assert (sizeof (unsigned long long) == sizeof (std::uint64_t),
                           "use of ctzll requires unsigned long long to be 64 bits");
            PSTORE_ASSERT (x != 0);
            return static_cast<unsigned> (__builtin_ctzll (x));
        }
        constexpr unsigned ctz (uint128 const x) noexcept {
            PSTORE_ASSERT (x != 0U);
            return x.low () == 0U ? 64U + ctz (x.high ()) : ctz (x.low ());
        }
#endif //_MSC_VER
       ///@}


#ifdef _MSC_VER
        // Unfortunately VC2017 won't allow pop_count to be constexpr.
        inline unsigned pop_count (unsigned char const x) noexcept {
            static_assert (sizeof (unsigned char) <= sizeof (unsigned __int16),
                           "unsigned char > unsigned __int16");
            return __popcnt16 (x);
        }
        inline unsigned pop_count (unsigned short const x) noexcept {
            static_assert (sizeof (unsigned short) == sizeof (unsigned __int16),
                           "unsigned short != unsigned __int16");
            return __popcnt16 (x);
        }
        inline unsigned pop_count (unsigned const x) noexcept { return __popcnt (x); }
        inline unsigned pop_count (unsigned long const x) noexcept {
            static_assert (sizeof (unsigned long) == sizeof (unsigned int),
                           "unsigned long != unsigned int");
            return pop_count (static_cast<unsigned int> (x));
        }
        inline unsigned pop_count (unsigned long long const x) noexcept {
            static_assert (sizeof (unsigned long long) == sizeof (unsigned __int64),
                           "unsigned long long != unsigned __int16");
            return static_cast<unsigned> (__popcnt64 (x));
        }
        inline unsigned pop_count (uint128 const x) noexcept {
            return pop_count (x.high ()) + pop_count (x.low ());
        }
#else
        constexpr unsigned pop_count (unsigned char const x) noexcept {
            return static_cast<unsigned> (__builtin_popcount (x));
        }
        constexpr unsigned pop_count (unsigned short const x) noexcept {
            return static_cast<unsigned> (__builtin_popcount (x));
        }
        constexpr unsigned pop_count (unsigned const x) noexcept {
            return static_cast<unsigned> (__builtin_popcount (x));
        }
        constexpr unsigned pop_count (unsigned long const x) noexcept {
            return static_cast<unsigned> (__builtin_popcountl (x));
        }
        constexpr unsigned pop_count (unsigned long long const x) noexcept {
            return static_cast<unsigned> (__builtin_popcountll (x));
        }
        constexpr unsigned pop_count (uint128 const x) noexcept {
            return pop_count (x.high ()) + pop_count (x.low ());
        }
#endif //_MSC_VER

    } // namespace bit_count
} // namespace pstore
#endif // PSTORE_SUPPORT_BIT_COUNT_HPP
