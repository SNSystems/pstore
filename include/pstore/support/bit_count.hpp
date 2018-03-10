//*  _     _ _                           _    *
//* | |__ (_) |_    ___ ___  _   _ _ __ | |_  *
//* | '_ \| | __|  / __/ _ \| | | | '_ \| __| *
//* | |_) | | |_  | (_| (_) | |_| | | | | |_  *
//* |_.__/|_|\__|  \___\___/ \__,_|_| |_|\__| *
//*                                           *
//===- include/pstore/support/bit_count.hpp -------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#ifndef PSTORE_BIT_COUNT_HPP
#define PSTORE_BIT_COUNT_HPP

#include <cassert>
#include <cstdint>
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace pstore {
    namespace bit_count {

#ifdef _MSC_VER
        /// Count the number of contiguous zero bits starting from the MSB.
        /// It is undefined behavior if x is 0.
        inline unsigned clz (std::uint64_t x) {
            assert (x != 0);
            unsigned long bit_position = 0;
            _BitScanReverse64 (&bit_position, x);
            assert (bit_position < 64);
            return 63 - bit_position;
        }

        /// Count the  number of contiguous zero bits starting from the LSB.
        /// It is undefined behavior if x is 0.
        inline unsigned ctz (std::uint64_t x) {
            assert (x != 0);
            unsigned long bit_position = 0;
            _BitScanForward64 (&bit_position, x);
            assert (bit_position < 64);
            return bit_position;
        }
#else
        /// Count the number of contiguous zero bits starting from the MSB.
        /// It is undefined behavior if x is 0.
        inline unsigned clz (unsigned long long x) {
            static_assert (sizeof (unsigned long long) == sizeof (std::uint64_t),
                           "use of clzll requires unsigned long long to be 64 bits");
            assert (x != 0);
            return static_cast<unsigned> (__builtin_clzll (x));
        }

        /// Count the  number of contiguous zero bits starting from the LSB.
        /// It is undefined behavior if x is 0.
        inline unsigned ctz (unsigned long long x) {
            static_assert (sizeof (unsigned long long) == sizeof (std::uint64_t),
                           "use of ctzll requires unsigned long long to be 64 bits");
            assert (x != 0);
            return static_cast<unsigned> (__builtin_ctzll (x));
        }
#endif


#ifdef _MSC_VER
        // TODO: VC2015RC won't allow pop_count to be constexpr. Sigh.
        inline unsigned pop_count (unsigned char x) {
            static_assert (sizeof (unsigned char) <= sizeof (unsigned __int16),
                           "unsigned char > unsigned __int16");
            return __popcnt16 (x);
        }
        inline unsigned pop_count (unsigned short x) {
            static_assert (sizeof (unsigned short) == sizeof (unsigned __int16),
                           "unsigned short != unsigned __int16");
            return __popcnt16 (x);
        }
        inline unsigned pop_count (unsigned x) { return __popcnt (x); }
        inline unsigned pop_count (unsigned long x) {
            static_assert (sizeof (unsigned long) == sizeof (unsigned int),
                           "unsigned long != unsigned int");
            return pop_count (static_cast<unsigned int> (x));
        }
        inline unsigned pop_count (unsigned long long x) {
            static_assert (sizeof (unsigned long long) == sizeof (unsigned __int64),
                           "unsigned long long != unsigned __int16");
            return static_cast<unsigned> (__popcnt64 (x));
        }
#else
        inline constexpr unsigned pop_count (unsigned char x) {
            return static_cast<unsigned> (__builtin_popcount (x));
        }
        inline constexpr unsigned pop_count (unsigned short x) {
            return static_cast<unsigned> (__builtin_popcount (x));
        }
        inline constexpr unsigned pop_count (unsigned x) {
            return static_cast<unsigned> (__builtin_popcount (x));
        }
        inline constexpr unsigned pop_count (unsigned long x) {
            return static_cast<unsigned> (__builtin_popcountl (x));
        }
        inline constexpr unsigned pop_count (unsigned long long x) {
            return static_cast<unsigned> (__builtin_popcountll (x));
        }
#endif

    } // namespace bit_count
} // namespace pstore
#endif // PSTORE_BIT_COUNT_HPP
// eof: include/pstore/support/bit_count.hpp
