//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===- lib/command_line/str_to_revision.cpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file str_to_revision.cpp
/// \brief Provides str_to_revision() which is used by the utility applications to decode the user's
/// choice of store revision number.

#include "pstore/command_line/str_to_revision.hpp"

#include <climits>

#include "pstore/support/ctype.hpp"
#include "pstore/support/head_revision.hpp"
#include "pstore/support/unsigned_cast.hpp"

namespace {

    /// Returns a copy of the input string with any leading or trailing whitespace removed and all
    /// characters converted to lower-case.
    std::string trim_and_lowercase (std::string const & str) {
        auto const is_ws = [] (char const c) { return pstore::isspace (c); };
        // Remove trailing whitespace.
        auto const end = std::find_if_not (str.rbegin (), str.rend (), is_ws).base ();
        // Skip leading whitespace.
        auto const begin = std::find_if_not (str.begin (), end, is_ws);

        std::string result;
        result.reserve (pstore::unsigned_cast (std::distance (begin, end)));
        std::transform (begin, end, std::back_inserter (result),
                        [] (char const c) { return static_cast<char> (std::tolower (c)); });
        return result;
    }

} // end anonymous namespace

namespace pstore {

    maybe<unsigned> str_to_revision (std::string const & str) {
        std::string const arg = trim_and_lowercase (str);
        if (arg == "head") {
            return just (head_revision);
        }

        gsl::czstring const cstr = arg.c_str ();
        char * endptr = nullptr;
        long const revision = std::strtol (cstr, &endptr, 10);

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
