//===- lib/command_line/csv.cpp -------------------------------------------===//
//*                  *
//*   ___ _____   __ *
//*  / __/ __\ \ / / *
//* | (__\__ \\ V /  *
//*  \___|___/ \_/   *
//*                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/csv.hpp"

#include "pstore/support/assert.hpp"

std::list<std::string> pstore::command_line::csv (std::string const & s) {
    std::list<std::string> result;
    auto spos = std::string::size_type{0};
    while (spos != std::string::npos) {
        auto const epos = s.find (',', spos);
        PSTORE_ASSERT (epos >= spos);
        result.emplace_back (s.substr (spos, epos - spos));
        spos = (epos == std::string::npos) ? epos : epos + 1;
    }
    return result;
}
