//*            _ _ _    *
//*  ___ _ __ | (_) |_  *
//* / __| '_ \| | | __| *
//* \__ \ |_) | | | |_  *
//* |___/ .__/|_|_|\__| *
//*     |_|             *
//===- unittests/common/split.hpp -----------------------------------------===//
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
#ifndef SPLIT_HPP
#define SPLIT_HPP

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <iterator>
#include <string>
#include <vector>

#include "pstore/support/ctype.hpp"

template <typename StringType>
std::vector<StringType> split_lines (StringType const & s) {
    std::vector<StringType> result;
    typename StringType::size_type pos = 0;
    for (;;) {
        auto const cr_pos = s.find_first_of ('\n', pos);
        auto count = (cr_pos == StringType::npos) ? StringType::npos : cr_pos - pos;
        result.emplace_back (s, pos, count);
        if (count == StringType::npos) {
            break;
        }
        pos = cr_pos + 1;
    }
    return result;
}

template <typename StringType>
std::vector<StringType> split_tokens (StringType const & s) {
    std::vector<StringType> result;

    using char_type = typename StringType::value_type;
    auto is_space = [](char_type c) { return pstore::isspace (c); };

    auto it = std::begin (s);
    auto end = std::end (s);
    while (it != end) {
        it = std::find_if_not (it, end, is_space); // skip leading whitespace
        auto start = it;
        it = std::find_if (it, end, is_space);
        if (start != it) {
            result.emplace_back (start, it);
        }
    }
    return result;
}

#endif // SPLIT_HPP
