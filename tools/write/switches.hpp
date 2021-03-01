//===- tools/write/switches.hpp ---------------------------*- mode: C++ -*-===//
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
#ifndef PSTORE_WRITE_SWITCHES_HPP
#define PSTORE_WRITE_SWITCHES_HPP

#include <list>
#include <string>
#include <utility>

#include "pstore/command_line/tchar.hpp"
#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"

struct switches {
    std::string db_path;
    pstore::database::vacuum_mode vmode = pstore::database::vacuum_mode::disabled;
    std::list<std::pair<std::string, std::string>> add;
    std::list<std::string> strings;
    std::list<std::pair<std::string, std::string>> files;
};

std::pair<switches, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // PSTORE_WRITE_SWITCHES_HPP
