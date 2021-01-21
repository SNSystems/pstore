//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/diff/switches.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "switches.hpp"

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/revision_opt.hpp"

using namespace pstore::command_line;

namespace {

    opt<std::string> db_path (positional, usage ("repository"),
                              desc ("Path of the pstore repository to be read."), required);

    opt<pstore::command_line::revision_opt, parser<std::string>>
        first_revision (positional, usage ("[1st-revision]"),
                        desc ("The first revision number (or 'HEAD')"), optional);

    opt<pstore::command_line::revision_opt, parser<std::string>>
        second_revision (positional, usage ("[2nd-revision]"),
                         desc ("The second revision number (or 'HEAD')"), optional);

    option_category how_cat ("Options controlling how fields are emitted");

    opt<bool> hex ("hex", desc ("Emit number values in hexadecimal notation"), cat (how_cat));
    alias hex2 ("x", desc ("Alias for --hex"), aliasopt (hex));

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    using namespace pstore;
    parse_command_line_options (argc, argv, "pstore diff utility\n");

    switches result;
    result.db_path = db_path.get ();
    result.first_revision = static_cast<unsigned> (first_revision.get ());
    result.second_revision = second_revision.get_num_occurrences () > 0
                                 ? just (static_cast<unsigned> (second_revision.get ()))
                                 : nothing<revision_number> ();
    result.hex = hex.get ();
    return {result, EXIT_SUCCESS};
}
