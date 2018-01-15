//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===- include/pstore/uint128.hpp -----------------------------------------===//
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

#ifndef PSTORE_UINT128_HPP
#define PSTORE_UINT128_HPP

#include <array>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

namespace pstore {

    class uint128 {
    public:
        constexpr uint128 () noexcept = default;

        constexpr uint128 (std::uint64_t high, std::uint64_t low) noexcept
                : high_{high}
                , low_{low} {}

        /// Construct from an unsigned integer that's 64-bits wide or fewer.
        template <typename IntType,
                  typename = typename std::enable_if<std::is_unsigned<IntType>::value>::type>
        constexpr uint128 (IntType v) noexcept
                : low_{v} {}

        /// \param bytes Points to an array of 16 bytes whose contents represent a 128 bit
        /// value.
        explicit uint128 (std::uint8_t const * bytes) noexcept
                : high_{bytes_to_uint64 (&bytes[0])}
                , low_{bytes_to_uint64 (&bytes[8])} {}

        uint128 (std::array<std::uint8_t, 16> const & bytes) noexcept
                : uint128 (bytes.data ()) {}

        uint128 (uint128 const &) = default;
        uint128 (uint128 &&) noexcept = default;

        uint128 & operator= (uint128 const &) = default;
        uint128 & operator= (uint128 &&) noexcept = default;

        constexpr std::uint64_t high () const noexcept {
            return high_;
        }
        constexpr std::uint64_t low () const noexcept {
            return low_;
        }

        std::string to_hex_string () const;

    private:
        std::uint64_t high_ = 0U;
        std::uint64_t low_ = 0U;

        static std::uint64_t bytes_to_uint64 (std::uint8_t const * bytes) noexcept {
            return std::uint64_t{bytes[7]} << 56 | std::uint64_t{bytes[6]} << 48 |
                   std::uint64_t{bytes[5]} << 40 | std::uint64_t{bytes[4]} << 32 |
                   std::uint64_t{bytes[3]} << 24 | std::uint64_t{bytes[2]} << 16 |
                   std::uint64_t{bytes[1]} << 8 | std::uint64_t{bytes[0]};
        }
    };

    static_assert (sizeof (uint128) == 16, "uint128 must be 16 bytes");
    static_assert (std::is_standard_layout<uint128>::value, "uint128 must be standard-layout");

    inline std::ostream & operator<< (std::ostream & os, uint128 const & value) {
        return os << '{' << value.high () << ',' << value.low () << '}';
    }


    inline constexpr bool operator== (uint128 const & lhs, uint128 const & rhs) noexcept {
        return lhs.high () == rhs.high () && lhs.low () == rhs.low ();
    }
    inline constexpr bool operator!= (uint128 const & lhs, uint128 const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }

    inline constexpr bool operator< (uint128 const & lhs, uint128 const & rhs) noexcept {
        return lhs.high () < rhs.high () ||
               (!(rhs.high () < lhs.high ()) && lhs.low () < rhs.low ());
    }
    inline constexpr bool operator> (uint128 const & lhs, uint128 const & rhs) noexcept {
        return rhs < lhs;
    }
    inline constexpr bool operator>= (uint128 const & lhs, uint128 const & rhs) noexcept {
        return !(lhs < rhs);
    }
    inline constexpr bool operator<= (uint128 const & lhs, uint128 const & rhs) noexcept {
        return !(rhs < lhs);
    }

} // namespace pstore

#endif // PSTORE_UINT128_HPP
// eof: include/pstore/uint128.hpp
