//===- unittests/common/split.hpp -------------------------*- mode: C++ -*-===//
//*            _ _ _    *
//*  ___ _ __ | (_) |_  *
//* / __| '_ \| | | __| *
//* \__ \ |_) | | | |_  *
//* |___/ .__/|_|_|\__| *
//*     |_|             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
    auto is_space = [] (char_type c) { return pstore::isspace (c); };

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
