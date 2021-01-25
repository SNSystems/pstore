//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/index_structure/switches.hpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_INDEX_STRUCTURE_SWITCHES_HPP
#define PSTORE_INDEX_STRUCTURE_SWITCHES_HPP

#include <bitset>
#include <string>

#include "pstore/command_line/tchar.hpp"
#include "pstore/core/database.hpp"
#include "pstore/config/config.hpp"

struct switches {
    std::bitset<static_cast<std::underlying_type<pstore::trailer::indices>::type> (
        pstore::trailer::indices::last)>
        selected;
    unsigned revision = pstore::head_revision;
    std::string db_path;

    bool test (pstore::trailer::indices idx) const {
        auto const position =
            static_cast<std::underlying_type<pstore::trailer::indices>::type> (idx);
        PSTORE_ASSERT (idx < pstore::trailer::indices::last);
        return selected.test (position);
    }
};

std::pair<switches, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // PSTORE_INDEX_STRUCTURE_SWITCHES_HPP
