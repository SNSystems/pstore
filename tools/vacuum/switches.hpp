//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/vacuum/switches.hpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_VACUUM_SWITCHES_HPP
#define PSTORE_VACUUM_SWITCHES_HPP

#include <utility>

#include "pstore/command_line/tchar.hpp"
#include "pstore/vacuum/user_options.hpp"

std::pair<vacuum::user_options, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // PSTORE_VACUUM_SWITCHES_HPP
