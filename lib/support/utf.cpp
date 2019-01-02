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
    } // namespace utf
} // namespace pstore
