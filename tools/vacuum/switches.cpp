//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/vacuum/switches.cpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "switches.hpp"

#include "pstore/command_line/command_line.hpp"

using namespace pstore::command_line;

namespace {

    opt<std::string> path (positional, usage ("repository"),
                           desc ("Path of the pstore repository to be vacuumed."));

} // end anonymous namespace

std::pair<vacuum::user_options, int> get_switches (int argc, tchar * argv[]) {
    parse_command_line_options (argc, argv, "pstore vacuum utility\n");

    vacuum::user_options opt;
    opt.src_path = path.get ();
    return {opt, EXIT_SUCCESS};
}
