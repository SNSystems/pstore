//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===- lib/support/uint128.cpp --------------------------------------------===//
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
#include "pstore/support/uint128.hpp"

#include <cassert>

using pstore::just;
using pstore::maybe;
using pstore::nothing;

namespace {

    constexpr auto expected_length = std::size_t{32};

    constexpr char digit_to_hex (unsigned const v) noexcept {
        assert (v < 0x10);
        return static_cast<char> (v + ((v < 10) ? '0' : 'a' - 10));
    }

    maybe<unsigned> hex_to_digit (char const digit) noexcept {
        if (digit >= 'a' && digit <= 'f') {
            return just (static_cast<unsigned> (digit) - ('a' - 10));
        }
        if (digit >= 'A' && digit <= 'F') {
            return just (static_cast<unsigned> (digit) - ('A' - 10));
        }
        if (digit >= '0' && digit <= '9') {
            return just (static_cast<unsigned> (digit) - '0');
        }
        return nothing<unsigned> ();
    }

    maybe<std::uint64_t> get64 (std::string const & str, unsigned index) {
        assert (index < str.length ());
        auto result = std::uint64_t{0};
        for (auto shift = 60; shift >= 0; shift -= 4, ++index) {
            auto const digit = hex_to_digit (str[index]);
            if (!digit) {
                return nothing<std::uint64_t> ();
            }
            result |= (static_cast<std::uint64_t> (digit.value ()) << shift);
        }
        return just (result);
    }

} // end anonymous namespace

namespace pstore {

#ifdef PSTORE_HAVE_UINT128_T

    std::string uint128::to_hex_string () const {
        std::string str;
        str.reserve (expected_length);

        for (auto shift = 4U; shift <= 128U; shift += 4U) {
            str += digit_to_hex ((v_ >> (128U - shift)) & 0x0FU);
        }

        assert (str.length () == expected_length);
        return str;
    }

#else

    std::string uint128::to_hex_string () const {
        std::string str;
        str.reserve (expected_length);

        for (auto shift = 4U; shift <= 64; shift += 4U) {
            str += digit_to_hex ((high_ >> (64U - shift)) & 0x0FU);
        }
        for (auto shift = 4U; shift <= 64; shift += 4U) {
            str += digit_to_hex ((low_ >> (64U - shift)) & 0x0FU);
        }

        assert (str.length () == expected_length);
        return str;
    }

#endif // PSTORE_HAVE_UINT128_T

    maybe<uint128> uint128::from_hex_string (std::string const & str) {
        if (str.length () != 32) {
            return nothing<uint128> ();
        }
        return get64 (str, 0U) >>= [&] (std::uint64_t const high) {
            return get64 (str, 16U) >>= [&] (std::uint64_t const low) {
                return just (uint128{high, low});
            };
        };
    }

} // namespace pstore
