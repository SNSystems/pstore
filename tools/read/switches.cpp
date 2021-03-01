//===- tools/read/switches.cpp --------------------------------------------===//
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
#include "switches.hpp"

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/support/error.hpp"

using namespace pstore::command_line;

namespace {

    opt<pstore::command_line::revision_opt, parser<std::string>>
        revision ("revision", desc ("The starting revision number (or 'HEAD')"));
    alias revision2 ("r", desc ("Alias for --revision"), aliasopt (revision));

    opt<std::string> db_path (positional, usage ("repository"),
                              desc ("Path of the pstore repository to be read"), required);
    opt<std::string> key (positional, usage ("key"), required);
    opt<bool> string_mode ("strings", init (false),
                           desc ("Reads from the 'strings' index rather than the 'names' index."));
    alias string_mode2 ("s", desc ("Alias for --strings"), aliasopt (string_mode));

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    parse_command_line_options (argc, argv, "pstore read utility\n");

    switches result;
    result.revision = static_cast<unsigned> (revision.get ());
    result.db_path = db_path.get ();
    result.key = key.get ();
    result.string_mode = string_mode.get ();

    return {result, EXIT_SUCCESS};
}
