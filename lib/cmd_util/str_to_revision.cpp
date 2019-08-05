//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===- lib/cmd_util/str_to_revision.cpp -----------------------------------===//
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
/// \file str_to_revision.cpp
/// \brief Provides str_to_revision() which is used by the utility applications to decode the user's
/// choice of store revision number.

#include "pstore/cmd_util/str_to_revision.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <type_traits>

#include "pstore/support/ctype.hpp"
#include "pstore/support/head_revision.hpp"
#include "pstore/support/maybe.hpp"

namespace {

    /// Returns a copy of the input string with any leading or trailing whitespace removed and all
    /// characters converted to lower-case.
    std::string trim_and_lowercase (std::string const & str) {
        auto is_ws = [](char c) { return pstore::isspace (c); };
        // Remove trailing whitespace.
        auto end = std::find_if_not (str.rbegin (), str.rend (), is_ws).base ();
        // Skip leading whitespace.
        auto begin = std::find_if_not (str.begin (), end, is_ws);

        std::string result;
        auto const new_length = std::distance (begin, end);
        assert (new_length >= 0);

        result.reserve (static_cast<std::make_unsigned<decltype (new_length)>::type> (new_length));
        std::transform (begin, end, std::back_inserter (result),
                        [](char c) { return static_cast<char> (std::tolower (c)); });
        return result;
    }

} // end anonymousnamespace

namespace pstore {

    maybe<unsigned> str_to_revision (std::string const & str) {
        std::string const arg = trim_and_lowercase (str);
        if (arg == "head") {
            return just (head_revision);
        }

        char const * cstr = arg.c_str ();
        char * endptr = nullptr;
        long revision = std::strtol (cstr, &endptr, 10);

        // strtol() returns LONG_MAX or LONG_MIN to indicate an overflow. endptr must point to
        // the terminating null character to ensure that the entire string was consumed. We also
        // check that revision lies within the range of numbers that we can safely represent.
        if (endptr != cstr && *endptr == '\0' && revision != LONG_MAX && revision != LONG_MIN &&
            revision >= 0 && revision != head_revision &&
            static_cast<unsigned long> (revision) < std::numeric_limits<unsigned>::max ()) {

            return just (static_cast<unsigned> (revision));
        }

        return nothing<unsigned> ();
    }

} // end namespace pstore
