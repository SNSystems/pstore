//===- tools/diff/switches.hpp ----------------------------*- mode: C++ -*-===//
//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DIFF_SWITCHES_HPP
#define PSTORE_DIFF_SWITCHES_HPP

#include <string>
#include <utility>

#include "pstore/command_line/tchar.hpp"
#include "pstore/diff_dump/diff_value.hpp"
#include "pstore/diff_dump/revision.hpp"

struct switches {
    std::string db_path;
    pstore::revision_number first_revision = pstore::head_revision;
    pstore::maybe<pstore::revision_number> second_revision;
    bool hex = false;
};

std::pair<switches, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // PSTORE_DIFF_SWITCHES_HPP
