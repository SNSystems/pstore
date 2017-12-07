//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- lib/pstore/sstring_view.cpp ----------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

#include "pstore/sstring_view.hpp"
#include "pstore/database.hpp"

namespace pstore {

    constexpr sstring_view::size_type sstring_view::npos;

    char const * sstring_view::search_substring (char const * first1, char const * last1,
                                                 char const * first2, char const * last2) {
        // Take advantage of knowing source and pattern lengths.
        // Stop short when source is smaller than pattern.
        std::ptrdiff_t const len2 = last2 - first2;
        if (len2 == 0) {
            return first1;
        }
        std::ptrdiff_t len1 = last1 - first1;
        if (len1 < len2) {
            return last1;
        }

        char f2 = *first2;
        while (true) {
            len1 = last1 - first1;
            // Check whether first1 still has at least len2 bytes.
            if (len1 < len2) {
                return last1;
            }

            // Find f2 the first byte matching in first1.
            first1 = traits::find (first1, len1 - len2 + 1, f2);
            if (first1 == nullptr) {
                return last1;
            }

            if (traits::compare (first1, first2, len2) == 0) {
                return first1;
            }

            ++first1;
        }
    }

    sstring_view::size_type sstring_view::str_find (char const * p, size_type sz, char c,
                                                    size_type pos) {
        if (pos >= sz) {
            return npos;
        }
        char const * r = traits::find (p + pos, sz - pos, c);
        if (r == nullptr) {
            return npos;
        }
        assert (r >= p);
        return static_cast<size_type> (r - p);
    }
    sstring_view::size_type sstring_view::str_find (char const * p, size_type sz, char const * s,
                                                    size_type pos, size_type n) {
        if (pos > sz) {
            return npos;
        }
        if (n == 0) {
            return pos;
        }
        char const * r = search_substring (p + pos, p + sz, s, s + n);
        if (r == p + sz) {
            return npos;
        }
        return static_cast<size_type> (r - p);
    }

} // namespace pstore
// eof: lib/pstore/sstring_view.cpp
