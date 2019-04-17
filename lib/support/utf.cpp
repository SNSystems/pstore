//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===- lib/support/utf.cpp ------------------------------------------------===//
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
/// \file support/utf.cpp
/// \brief Implementation of functions for processing UTF-8 strings.

#include "pstore/support/utf.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <type_traits>

namespace pstore {
    namespace utf {

        std::size_t length (char const * str, std::size_t nbytes) {
            if (str == nullptr) {
                return 0;
            }
            return length (str, str + nbytes);
        }

        std::size_t length (::pstore::gsl::czstring str) {
            if (str == nullptr) {
                return 0;
            }
            return length (str, str + std::strlen (str));
        }


        // index
        // ~~~~~
        // returns a pointer to the beginning of the pos'th utf8 codepoint
        // in the buffer at s
        char const * index (::pstore::gsl::czstring str, std::size_t pos) {
            if (str == nullptr) {
                return nullptr;
            }
            char const * end = str + std::strlen (str);
            char const * result = index (str, end, pos);
            return result != end ? result : nullptr;
        }

        // slice
        // ~~~~~
        // converts codepoint indices start and end to byte offsets in the buffer at s
        std::pair<std::ptrdiff_t, std::ptrdiff_t> slice (::pstore::gsl::czstring s,
                                                         std::ptrdiff_t start, std::ptrdiff_t end) {
            if (s == nullptr) {
                return std::make_pair (-1, -1);
            }

            auto const first = s;
            auto const last = s + strlen (s);
            auto p1 =
                index (first, last, static_cast<std::size_t> (std::max (std::ptrdiff_t{0}, start)));
            auto p2 =
                index (first, last, static_cast<std::size_t> (std::max (std::ptrdiff_t{0}, end)));
            return std::make_pair ((p1 != last) ? p1 - s : -1, (p2 != last) ? p2 - s : -1);
        }


        //*       _    __ ___      _                _          *
        //*  _  _| |_ / _( _ )  __| |___ __ ___  __| |___ _ _  *
        //* | || |  _|  _/ _ \ / _` / -_) _/ _ \/ _` / -_) '_| *
        //*  \_,_|\__|_| \___/ \__,_\___\__\___/\__,_\___|_|   *
        //*                                                    *

        // Based on the "Flexible and Economical UTF-8 Decoder" by Bjoern Hoehrmann
        // <bjoern@hoehrmann.de> See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
        //
        // Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
        //
        // Permission is hereby granted, free of charge, to any person obtaining a copy of this
        // software and associated documentation files (the "Software"), to deal in the Software
        // without restriction, including without limitation the rights to use, copy, modify, merge,
        // publish, distribute, sublicense, and/or sell copies of the Software, and to permit
        // persons to whom the Software is furnished to do so, subject to the following conditions:
        //
        // The above copyright notice and this permission notice shall be included in all copies or
        // substantial portions of the Software.
        //
        // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
        // INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
        // PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
        // FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
        // OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
        // DEALINGS IN THE SOFTWARE.

        std::uint8_t const
            utf8_decoder::utf8d_[] =
                {
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 00..1f
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20..3f
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 40..5f
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                    0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 60..7f
                    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
                    1,   1,   1,   1,   1,   9,   9,   9,   9,   9,   9,
                    9,   9,   9,   9,   9,   9,   9,   9,   9,   9, // 80..9f
                    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
                    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
                    7,   7,   7,   7,   7,   7,   7,   7,   7,   7, // a0..bf
                    8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,
                    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
                    2,   2,   2,   2,   2,   2,   2,   2,   2,   2, // c0..df
                    0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
                    0x3, 0x3, 0x4, 0x3, 0x3, // e0..ef
                    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
                    0x8, 0x8, 0x8, 0x8, 0x8, // f0..ff
                    0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4,
                    0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
                    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
                    1,   1,   1,   1,   1,   1,   0,   1,   1,   1,   1,
                    1,   0,   1,   0,   1,   1,   1,   1,   1,   1, // s1..s2
                    1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,
                    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
                    1,   2,   1,   1,   1,   1,   1,   1,   1,   1, // s3..s4
                    1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,
                    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
                    1,   3,   1,   3,   1,   1,   1,   1,   1,   1, // s5..s6
                    1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,
                    1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,
                    1,   1,   1,   1,   1,   1,   1,   1,   1,   1, // s7..s8
        };

        std::uint8_t utf8_decoder::decode (gsl::not_null<std::uint8_t *> state,
                                           gsl::not_null<char32_t *> codep,
                                           std::uint32_t const byte) noexcept {
            auto const type = utf8d_[byte];
            *codep = (*state != accept) ? (byte & 0x3FU) | (*codep << 6U) : (0xFFU >> type) & byte;
            *state = utf8d_[256 + *state * 16 + type];
            return *state;
        }

        maybe<char32_t> utf8_decoder::get (std::uint8_t byte) noexcept {
            if (decode (&state_, &codepoint_, byte)) {
                well_formed_ = false;
                return nothing<char32_t> ();
            }
            auto const res = codepoint_;
            well_formed_ = true;
            codepoint_ = 0;
            return just (res);
        }

    } // namespace utf
} // namespace pstore
