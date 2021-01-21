//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===- lib/support/uint128.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/support/uint128.hpp"

using pstore::just;
using pstore::maybe;
using pstore::nothing;

namespace {

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
        PSTORE_ASSERT (index < str.length ());
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

    // to hex string
    // ~~~~~~~~~~~~~
    std::string uint128::to_hex_string () const {
        std::string result;
        result.reserve (hex_string_length);
        this->to_hex (std::back_inserter (result));
        return result;
    }

    // from hex string
    // ~~~~~~~~~~~~~~~
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

} // end namespace pstore
