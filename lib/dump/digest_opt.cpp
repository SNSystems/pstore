//*      _ _                 _                 _    *
//*   __| (_) __ _  ___  ___| |_    ___  _ __ | |_  *
//*  / _` | |/ _` |/ _ \/ __| __|  / _ \| '_ \| __| *
//* | (_| | | (_| |  __/\__ \ |_  | (_) | |_) | |_  *
//*  \__,_|_|\__, |\___||___/\__|  \___/| .__/ \__| *
//*          |___/                      |_|         *
//===- lib/dump/digest_opt.cpp --------------------------------------------===//
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
#include "pstore/dump/digest_opt.hpp"

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
    namespace dump {

        maybe<index::digest> digest_from_string (std::string const & str) {
            if (str.length () != 32) {
                return nothing<index::digest> ();
            }
            return get64 (str, 0U) >>= [&] (std::uint64_t high) {
                return get64 (str, 16U) >>= [&] (std::uint64_t low) {
                    return just (index::digest{high, low});
                };
            };
        }

    } // namespace dump
} // end namespace pstore
